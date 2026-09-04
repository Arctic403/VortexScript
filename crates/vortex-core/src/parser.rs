use crate::ast::{
    CompareOp, Literal, MaterialDecl, ModelDecl, ModelStatement, OptimizeBlock, Parameter, Program,
};
use crate::diagnostic::Diagnostic;
use crate::lexer::{Token, TokenKind};

pub struct Parser {
    tokens: Vec<Token>,
    current: usize,
}

impl Parser {
    pub fn new(tokens: Vec<Token>) -> Self {
        Self { tokens, current: 0 }
    }

    pub fn parse_program(mut self) -> Result<Program, Diagnostic> {
        let mut models = Vec::new();
        while !self.is_eof() {
            models.push(self.parse_model()?);
        }
        Ok(Program { models })
    }

    fn parse_model(&mut self) -> Result<ModelDecl, Diagnostic> {
        self.expect_keyword("model")?;
        let name = self.expect_ident()?;
        self.expect_lbrace()?;

        let mut statements = Vec::new();
        while !self.at_rbrace() {
            if self.is_eof() {
                return Err(self.error_here("expected '}' to close model"));
            }
            statements.push(self.parse_model_statement()?);
        }
        self.bump();

        Ok(ModelDecl { name, statements })
    }

    fn parse_model_statement(&mut self) -> Result<ModelStatement, Diagnostic> {
        if self.peek_keyword("import") {
            self.bump();
            let path = self.expect_string()?;
            self.consume_semicolon();
            return Ok(ModelStatement::Import(path));
        }

        if self.peek_keyword("optimize") {
            self.bump();
            let target = self.expect_ident()?;
            let parameters = self.parse_parameter_block()?;
            return Ok(ModelStatement::Optimize(OptimizeBlock { target, parameters }));
        }

        if self.peek_keyword("material") {
            self.bump();
            let name = self.expect_ident()?;
            let parameters = self.parse_parameter_block()?;
            return Ok(ModelStatement::Material(MaterialDecl { name, parameters }));
        }

        Err(self.error_here("expected import, optimize, or material statement"))
    }

    fn parse_parameter_block(&mut self) -> Result<Vec<Parameter>, Diagnostic> {
        self.expect_lbrace()?;
        let mut parameters = Vec::new();
        while !self.at_rbrace() {
            if self.is_eof() {
                return Err(self.error_here("expected '}' to close block"));
            }
            parameters.push(self.parse_parameter()?);
        }
        self.bump();
        Ok(parameters)
    }

    fn parse_parameter(&mut self) -> Result<Parameter, Diagnostic> {
        let key = self.expect_ident()?;
        let op = match self.current().kind {
            TokenKind::LessEqual => {
                self.bump();
                CompareOp::LessEqual
            }
            TokenKind::Equal => {
                self.bump();
                CompareOp::Equal
            }
            _ => CompareOp::Set,
        };
        let value = self.parse_literal()?;
        self.consume_semicolon();
        Ok(Parameter { key, op, value })
    }

    fn parse_literal(&mut self) -> Result<Literal, Diagnostic> {
        let token = self.bump();
        match token.kind {
            TokenKind::Number(value) => Ok(Literal::Number(value)),
            TokenKind::String(value) => Ok(Literal::String(value)),
            TokenKind::Ident(value) if value == "true" => Ok(Literal::Bool(true)),
            TokenKind::Ident(value) if value == "false" => Ok(Literal::Bool(false)),
            TokenKind::Ident(value) => Ok(Literal::Identifier(value)),
            _ => Err(Diagnostic::new("expected a value", token.span)),
        }
    }

    fn expect_keyword(&mut self, keyword: &str) -> Result<(), Diagnostic> {
        if self.peek_keyword(keyword) {
            self.bump();
            Ok(())
        } else {
            Err(self.error_here(format!("expected '{keyword}'")))
        }
    }

    fn peek_keyword(&self, keyword: &str) -> bool {
        matches!(&self.current().kind, TokenKind::Ident(value) if value == keyword)
    }

    fn expect_ident(&mut self) -> Result<String, Diagnostic> {
        let token = self.bump();
        match token.kind {
            TokenKind::Ident(value) => Ok(value),
            _ => Err(Diagnostic::new("expected identifier", token.span)),
        }
    }

    fn expect_string(&mut self) -> Result<String, Diagnostic> {
        let token = self.bump();
        match token.kind {
            TokenKind::String(value) => Ok(value),
            _ => Err(Diagnostic::new("expected string literal", token.span)),
        }
    }

    fn expect_lbrace(&mut self) -> Result<(), Diagnostic> {
        let token = self.bump();
        match token.kind {
            TokenKind::LBrace => Ok(()),
            _ => Err(Diagnostic::new("expected '{'", token.span)),
        }
    }

    fn consume_semicolon(&mut self) {
        if matches!(self.current().kind, TokenKind::Semicolon) {
            self.bump();
        }
    }

    fn at_rbrace(&self) -> bool {
        matches!(self.current().kind, TokenKind::RBrace)
    }

    fn is_eof(&self) -> bool {
        matches!(self.current().kind, TokenKind::Eof)
    }

    fn current(&self) -> &Token {
        &self.tokens[self.current]
    }

    fn bump(&mut self) -> Token {
        let token = self.tokens[self.current].clone();
        if !matches!(token.kind, TokenKind::Eof) {
            self.current += 1;
        }
        token
    }

    fn error_here(&self, message: impl Into<String>) -> Diagnostic {
        Diagnostic::new(message, self.current().span)
    }
}
