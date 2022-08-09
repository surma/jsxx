use std::io::Read;

use anyhow::{anyhow, Result};
use swc_common::BytePos;
use swc_ecma_parser::{lexer::Lexer, EsConfig, Parser, StringInput, Syntax};

mod transpiler;

fn main() -> Result<()> {
    let mut input: String = String::new();
    std::io::stdin().read_to_string(&mut input)?;

    let syntax = Syntax::Es(EsConfig::default());
    let lexer = Lexer::new(
        syntax,
        swc_ecma_visit::swc_ecma_ast::EsVersion::Es2022,
        StringInput::new(
            &input,
            swc_common::BytePos(0),
            BytePos(input.bytes().len().try_into().unwrap()),
        ),
        None,
    );
    let mut parser = Parser::new_from(lexer);
    let module = parser
        .parse_module()
        .map_err(|err| anyhow!(format!("{:?}", err)))?;

    let mut transpiler = transpiler::Transpiler {};
    let result = transpiler.transpile_module(&module)?;
    println!("{}", result);
    Ok(())
}
