use anyhow::{anyhow, Result};
use swc_common::BytePos;
use swc_ecma_parser::{lexer::Lexer, EsConfig, Parser, StringInput, Syntax};

fn main() -> Result<()> {
    let input = r#"
		const a = 4;
		const b = [1, 2, 3];
		b.map(x => x + a);
	"#;
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
    println!("{:?}", module);
    Ok(())
}
