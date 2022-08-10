use std::{
    fs::File,
    io::{Read, Write},
    process::{Command, Stdio},
};

use anyhow::{anyhow, Result};
use swc_common::BytePos;
use swc_ecma_parser::{lexer::Lexer, EsConfig, Parser, StringInput, Syntax};

mod globals;
mod transpiler;

fn js_to_cpp<T: AsRef<str>>(input: T) -> Result<String> {
    let syntax = Syntax::Es(EsConfig::default());
    let lexer = Lexer::new(
        syntax,
        swc_ecma_visit::swc_ecma_ast::EsVersion::Es2022,
        StringInput::new(
            input.as_ref(),
            swc_common::BytePos(0),
            BytePos(input.as_ref().as_bytes().len().try_into().unwrap()),
        ),
        None,
    );
    let mut parser = Parser::new_from(lexer);
    let module = parser
        .parse_module()
        .map_err(|err| anyhow!(format!("{:?}", err)))?;

    let mut transpiler = transpiler::Transpiler {
        globals: vec![globals::wasi::WASIGlobal(), globals::json::JSONGlobal()],
    };
    transpiler.transpile_module(&module)
}

fn cpp_to_binary<T: AsRef<str>, S: AsRef<str>>(
    code: T,
    outputname: S,
    flags: Vec<&str>,
) -> Result<()> {
    let cpp_file_name = format!("./{}.cpp", outputname.as_ref());
    let mut tempfile = File::create(&cpp_file_name)?;
    tempfile.write_all(code.as_ref().as_bytes())?;
    tempfile.flush();
    drop(tempfile);

    let mut child = Command::new("clang++")
        .stdin(Stdio::inherit())
        .stdout(Stdio::inherit())
        .stderr(Stdio::inherit())
        .args(
            flags
                .into_iter()
                .chain(
                    [
                        "--std=c++17",
                        "-o",
                        outputname.as_ref(),
                        cpp_file_name.as_ref(),
                        "runtime/global_json.cpp",
                        "runtime/global_wasi.cpp",
                        "runtime/js_primitives.cpp",
                        "runtime/js_value_binding.cpp",
                        "runtime/js_value.cpp",
                    ]
                    .into_iter(),
                )
                .collect::<Vec<&str>>(),
        )
        .spawn()?;

    child.wait()?;
    std::fs::remove_file(cpp_file_name)?;
    Ok(())
}

fn main() -> Result<()> {
    let mut input: String = String::new();
    std::io::stdin().read_to_string(&mut input)?;

    let cpp_code = js_to_cpp(&input)?;
    cpp_to_binary(cpp_code, "output", vec![])?;
    Ok(())
}

#[cfg(test)]
mod test {
    use super::*;
    use anyhow::Result;

    #[test]
    fn basic_program() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout("hello");
            "#,
            "basic_program",
        )?;
        assert_eq!(output, "hello");
        Ok(())
    }

    #[test]
    fn number_coalesc() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout("" + 123);
            "#,
            "number_coalesc",
        )?;
        assert!(output.starts_with("123."));
        Ok(())
    }

    fn compile_and_run<T: AsRef<str>, S: AsRef<str>>(code: T, name: S) -> Result<String> {
        let cpp = js_to_cpp(code)?;
        cpp_to_binary(cpp, name.as_ref(), vec![])?;
        let mut child = Command::new(format!("./{}", name.as_ref()))
            .stdout(Stdio::piped())
            .spawn()?;
        let output = child.wait_with_output()?;
        Ok(String::from_utf8(output.stdout)?)
    }
}
