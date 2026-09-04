use crate::ast::{CompareOp, Literal, ModelStatement, Parameter, Program};
use crate::ir::{CompareCode, Op, Plan, Target, Value};

pub fn compile(program: &Program) -> Plan {
    let mut compiler = Compiler::default();
    compiler.compile_program(program);
    compiler.finish()
}

#[derive(Default)]
struct Compiler {
    strings: Vec<String>,
    ops: Vec<Op>,
}

impl Compiler {
    fn compile_program(&mut self, program: &Program) {
        for model in &program.models {
            let name = self.intern(&model.name);
            self.ops.push(Op::BeginModel(name));

            for statement in &model.statements {
                match statement {
                    ModelStatement::Import(path) => {
                        let path = self.intern(path);
                        self.ops.push(Op::Import(path));
                    }
                    ModelStatement::Optimize(block) => {
                        let target = self.compile_target(&block.target);
                        self.ops.push(Op::BeginOptimize(target));
                        for parameter in &block.parameters {
                            self.compile_parameter(parameter);
                        }
                        self.ops.push(Op::EndOptimize);
                    }
                    ModelStatement::Material(material) => {
                        let name = self.intern(&material.name);
                        self.ops.push(Op::BeginMaterial(name));
                        for parameter in &material.parameters {
                            self.compile_parameter(parameter);
                        }
                        self.ops.push(Op::EndMaterial);
                    }
                }
            }

            self.ops.push(Op::EndModel);
        }
    }

    fn compile_parameter(&mut self, parameter: &Parameter) {
        let key = self.intern(&parameter.key);
        let op = match parameter.op {
            CompareOp::Set => CompareCode::Set,
            CompareOp::Equal => CompareCode::Equal,
            CompareOp::LessEqual => CompareCode::LessEqual,
        };
        let value = self.compile_literal(&parameter.value);
        self.ops.push(Op::Parameter { key, op, value });
    }

    fn compile_literal(&mut self, literal: &Literal) -> Value {
        match literal {
            Literal::Number(value) => Value::Number(*value as f32),
            Literal::String(value) => Value::String(self.intern(value)),
            Literal::Bool(value) => Value::Bool(*value),
            Literal::Identifier(value) => Value::Identifier(self.intern(value)),
        }
    }

    fn compile_target(&mut self, target: &str) -> Target {
        match target {
            "android" => Target::Android,
            "web" => Target::Web,
            "desktop" => Target::Desktop,
            other => Target::Named(self.intern(other)),
        }
    }

    fn intern(&mut self, value: &str) -> u32 {
        if let Some(index) = self.strings.iter().position(|existing| existing == value) {
            return index as u32;
        }
        let index = self.strings.len() as u32;
        self.strings.push(value.to_owned());
        index
    }

    fn finish(self) -> Plan {
        Plan {
            strings: self.strings,
            ops: self.ops,
        }
    }
}
