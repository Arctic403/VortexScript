#[derive(Debug, Clone, PartialEq)]
pub struct Program {
    pub models: Vec<ModelDecl>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ModelDecl {
    pub name: String,
    pub statements: Vec<ModelStatement>,
}

#[derive(Debug, Clone, PartialEq)]
pub enum ModelStatement {
    Import(String),
    Optimize(OptimizeBlock),
    Material(MaterialDecl),
}

#[derive(Debug, Clone, PartialEq)]
pub struct OptimizeBlock {
    pub target: String,
    pub parameters: Vec<Parameter>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct MaterialDecl {
    pub name: String,
    pub parameters: Vec<Parameter>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CompareOp {
    Set,
    Equal,
    LessEqual,
}

#[derive(Debug, Clone, PartialEq)]
pub struct Parameter {
    pub key: String,
    pub op: CompareOp,
    pub value: Literal,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Literal {
    Number(f64),
    String(String),
    Bool(bool),
    Identifier(String),
}
