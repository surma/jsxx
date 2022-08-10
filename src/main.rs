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

fn cpp_to_binary<T: AsRef<str>>(code: T, flags: Vec<&str>) -> Result<()> {
    let mut tempfile = File::create("./generated.cpp")?;
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
                        "output",
                        "generated.cpp",
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
    Ok(())
}

fn main() -> Result<()> {
    let mut input: String = String::new();
    std::io::stdin().read_to_string(&mut input)?;

    let cpp_code = js_to_cpp(&input)?;
    cpp_to_binary(cpp_code, vec![])?;
    Ok(())
}
