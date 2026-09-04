pub mod ast;
pub mod compiler;
pub mod diagnostic;
pub mod ir;
pub mod lexer;
pub mod parser;
pub mod runtime;

pub use diagnostic::{Diagnostic, Span};
pub use ir::Plan;

pub fn compile_source(source: &str) -> Result<Plan, Vec<Diagnostic>> {
    let tokens = lexer::lex(source).map_err(|diagnostic| vec![diagnostic])?;
    let program = parser::Parser::new(tokens)
        .parse_program()
        .map_err(|diagnostic| vec![diagnostic])?;
    Ok(compiler::compile(&program))
}
