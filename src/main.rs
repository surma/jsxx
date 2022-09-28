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

#[cfg(test)]
mod test;

#[derive(Parser)]
#[clap(author, version, about)]
struct Args {
    /// Path to clang++
    #[clap(long = "clang-path", default_value = "clang++", value_parser)]
    clang_path: String,

    /// Emit cpp code to stdout rather than compiling it
    #[clap(long = "emit-cpp", default_value_t = false, value_parser)]
    emit_cpp: bool,

    /// Target WebAssembly
    #[clap(long = "wasm", default_value_t = false, value_parser)]
    wasm: bool,

    /// Extra flags to path to clang++
    extra_flags: Vec<String>,
}

fn js_to_cpp<T: AsRef<str>>(mut transpiler: transpiler::Transpiler, input: T) -> Result<String> {
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

    transpiler.globals.push(globals::io::io_global());
    transpiler.globals.push(globals::json::json_global());
    transpiler.globals.push(globals::symbol::symbol_global());
    transpiler.transpile_module(&module)
}

fn cpp_to_binary(
    code: String,
    outputname: String,
    clang_path: String,
    flags: &[String],
) -> Result<()> {
    let cpp_file_name = format!("./{}.cpp", outputname);
    let mut tempfile = File::create(&cpp_file_name)?;
    tempfile.write_all(code.as_bytes())?;
    drop(tempfile);

    let args = flags
        .into_iter()
        .map(|i| i.as_ref())
        .chain(
            [
                "--std=c++20",
                "-o",
                outputname.as_ref(),
                cpp_file_name.as_ref(),
                "runtime/global_json.cpp",
                "runtime/global_symbol.cpp",
                "runtime/global_io.cpp",
                "runtime/js_primitives.cpp",
                "runtime/js_value.cpp",
                "runtime/exceptions.cpp",
            ]
            .into_iter(),
        )
        .collect::<Vec<&str>>();

    let mut child = Command::new(&clang_path)
        .stdin(Stdio::inherit())
        .stdout(Stdio::inherit())
        .stderr(Stdio::inherit())
        .args(args)
        .spawn()?;

    child.wait()?;
    std::fs::remove_file(cpp_file_name)?;
    Ok(())
}

fn main() -> Result<()> {
    let args = Args::parse();

    let mut input: String = String::new();
    std::io::stdin().read_to_string(&mut input)?;

    let mut transpiler = transpiler::Transpiler::new();
    transpiler.feature_exceptions = !args.wasm;
    let cpp_code = js_to_cpp(transpiler, &input)?;

    if args.emit_cpp {
        let (_status, stdout, _stderr) =
            command_utils::pipe_through_shell::<String>("clang-format", &[], cpp_code.as_bytes())?;
        println!("{}", String::from_utf8(stdout)?);
    } else {
        let mut flags = args.extra_flags.clone();
        let mut extension = "".to_string();
        if args.wasm {
            flags.push("-fno-exceptions".to_string());
            flags.push("--target=wasm32-wasi".to_string());
            if let Ok(wasi_sdk_prefix) = std::env::var("WASI_SDK_PREFIX") {
                flags.push(format!("--sysroot={}/share/wasi-sysroot", wasi_sdk_prefix));
            }
            extension = ".wasm".to_string();
        } else {
            flags.push("-DFEATURE_EXCEPTIONS".to_string());
        }
        cpp_to_binary(
            cpp_code,
            format!("output{}", extension),
            args.clang_path,
            &flags,
        )?;
    }
    Ok(())
}
