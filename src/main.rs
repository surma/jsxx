use std::{
    fs::File,
    io::{Read, Write},
    process::{Command, Stdio},
};

use anyhow::{anyhow, Result};
use clap::Parser;
use swc_common::BytePos;
use swc_ecma_parser::{lexer::Lexer, EsConfig, Parser as ESParser, StringInput, Syntax};

mod command_utils;
mod globals;
mod transpiler;

#[derive(Parser)]
#[clap(author, version, about)]
struct Args {
    /// Emit cpp code to stdout rather than compiling it.
    #[clap(long = "emit-cpp", default_value_t = false, value_parser)]
    emit_cpp: bool,
}

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
    let mut parser = ESParser::new_from(lexer);
    let module = parser
        .parse_module()
        .map_err(|err| anyhow!(format!("{:?}", err)))?;

    let mut transpiler = transpiler::Transpiler::new();
    transpiler.globals.push(globals::wasi::WASIGlobal());
    transpiler.globals.push(globals::json::JSONGlobal());
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
    let args = Args::parse();

    let mut input: String = String::new();
    std::io::stdin().read_to_string(&mut input)?;

    let cpp_code = js_to_cpp(&input)?;

    if args.emit_cpp {
        let (status, stdout, stderr) =
            command_utils::pipe_through_shell::<String>("clang-format", &[], cpp_code.as_bytes())?;
        println!("{}", String::from_utf8(stdout)?);
    } else {
        cpp_to_binary(cpp_code, "output", vec![])?;
    }
    Ok(())
}

#[cfg(test)]
mod test {
    use super::*;
    use anyhow::Result;
    use uuid::Uuid;

    #[test]
    fn basic_program() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout("hello");
            "#,
        )?;
        assert_eq!(output, "hello");
        Ok(())
    }

    #[test]
    fn variable() -> Result<()> {
        let output = compile_and_run(
            r#"
                let a = "hello";
                WASI.write_to_stdout(a);
            "#,
        )?;
        assert_eq!(output, "hello");
        Ok(())
    }

    #[test]
    fn variable_assign() -> Result<()> {
        let output = compile_and_run(
            r#"
                let a = "hi";
                a = "hello";
                WASI.write_to_stdout(a);
            "#,
        )?;
        assert_eq!(output, "hello");
        Ok(())
    }

    #[test]
    fn ternary() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout(2 == 3 ? "yes" : "no");
            "#,
        )?;
        assert_eq!(output, "no");
        Ok(())
    }

    #[test]
    fn compare() -> Result<()> {
        let output = compile_and_run(
            r#"
                let b = (2 == 2) && (3 != 4) && (1 < 2) && (2<=2) && (3>=3) && (4 > 3);
                WASI.write_to_stdout(b ? "yes" : "no");
            "#,
        )?;
        assert_eq!(output, "yes");
        Ok(())
    }

    #[test]
    fn arrow_func() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout("" + (() => "test")());
            "#,
        )?;
        assert!(output.starts_with("test"));
        Ok(())
    }

    #[test]
    fn arrow_func_with_body() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout("" + (() => { 1 + 1; return "test";})());
            "#,
        )?;
        assert!(output.starts_with("test"));
        Ok(())
    }

    #[test]
    fn func_decl() -> Result<()> {
        let output = compile_and_run(
            r#"
                function a() {
                    return "test";
                }

                WASI.write_to_stdout("" + a());
            "#,
        )?;
        assert!(output.starts_with("test"));
        Ok(())
    }

    #[test]
    fn full_func() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout("" + (function () { return "test";})());
            "#,
        )?;
        assert!(output.starts_with("test"));
        Ok(())
    }

    #[test]
    fn if_else() -> Result<()> {
        let output = compile_and_run(
            r#"
                let a;
                if(1 == 1) {
                    a = "y";
                } else {
                    a = "n";
                }
                let b;
                if(1 == 2) {
                    b = "y";
                } else {
                    b = "n";
                }
                WASI.write_to_stdout(a + b);
            "#,
        )?;
        assert_eq!(output, "yn");
        Ok(())
    }
    #[test]
    fn number_coalesc() -> Result<()> {
        let output = compile_and_run(
            r#"
                WASI.write_to_stdout("" + 123);
            "#,
        )?;
        assert!(output.starts_with("123."));
        Ok(())
    }

    #[test]
    fn array_literals() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = ["a", "b", "c"]
                WASI.write_to_stdout(v.join(","));
            "#,
        )?;
        assert_eq!(output, "a,b,c");
        Ok(())
    }

    #[test]
    fn array_access() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = ["a", "b"];
                WASI.write_to_stdout(v[0] + v[1]);
            "#,
        )?;
        assert_eq!(output, "ab");
        Ok(())
    }

    #[test]
    fn array_push() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = ["a", "b"];
                v.push("c");
                WASI.write_to_stdout(v.join(","));
            "#,
        )?;
        assert_eq!(output, "a,b,c");
        Ok(())
    }

    #[test]
    fn array_map() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = ["a", "b", "c"];
                WASI.write_to_stdout(v.map(v => v + "!").join(","));
            "#,
        )?;
        assert_eq!(output, "a!,b!,c!");
        Ok(())
    }

    #[test]
    fn array_filter() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = [1, 2, 3, 4, 5];
                WASI.write_to_stdout(v.filter(v => v % 2 == 0).length == 2 ? "yes" : "no");
            "#,
        )?;
        assert_eq!(output, "yes");
        Ok(())
    }

    #[test]
    fn array_reduce() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = ["a", "b", "c"].reduce((acc, c) => acc + c, "X");
                let v2 = ["a", "b", "c"].reduce((acc, c) => acc + c);
                WASI.write_to_stdout(v + v2);
            "#,
        )?;
        assert_eq!(output, "Xabcabc");
        Ok(())
    }

    #[test]
    fn object_lit() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = {a: "v"};
                WASI.write_to_stdout(v.a);
            "#,
        )?;
        assert_eq!(output, "v");
        Ok(())
    }

    #[test]
    fn object_func() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = {a: () => "hi"};
                WASI.write_to_stdout(v.a());
            "#,
        )?;
        assert_eq!(output, "hi");
        Ok(())
    }

    #[test]
    fn object_func_this() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = {marker: "flag", a: function() { return this.marker; }};
                WASI.write_to_stdout(v.a());
            "#,
        )?;
        assert_eq!(output, "flag");
        Ok(())
    }

    #[test]
    fn object_assign() -> Result<()> {
        let output = compile_and_run(
            r#"
                let v = {marker: "flag"};
                v.marker = "hi";
                WASI.write_to_stdout(v.marker);
            "#,
        )?;
        assert_eq!(output, "hi");
        Ok(())
    }

    #[test]
    fn object_shorthand() -> Result<()> {
        let output = compile_and_run(
            r#"
                let a = "hi";
                let v = {a};
                WASI.write_to_stdout(v.a);
            "#,
        )?;
        assert_eq!(output, "hi");
        Ok(())
    }

    fn compile_and_run<T: AsRef<str>>(code: T) -> Result<String> {
        let name = Uuid::new_v4().to_string();
        let cpp = js_to_cpp(code)?;
        cpp_to_binary(cpp, &name, vec![])?;
        let mut child = Command::new(format!("./{}", &name))
            .stdout(Stdio::piped())
            .spawn()?;
        let output = child.wait_with_output()?;
        std::fs::remove_file(&name)?;
        Ok(String::from_utf8(output.stdout)?)
    }
}
