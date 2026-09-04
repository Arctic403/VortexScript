use crate::ir::{CompareCode, Op, Plan, Target, Value};

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ResolvedValue<'a> {
    Number(f32),
    String(&'a str),
    Bool(bool),
    Identifier(&'a str),
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ResolvedTarget<'a> {
    Android,
    Web,
    Desktop,
    Named(&'a str),
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ResolvedOp<'a> {
    BeginModel(&'a str),
    EndModel,
    Import(&'a str),
    BeginOptimize(ResolvedTarget<'a>),
    EndOptimize,
    BeginMaterial(&'a str),
    EndMaterial,
    Parameter {
        key: &'a str,
        op: CompareCode,
        value: ResolvedValue<'a>,
    },
}

pub trait Backend {
    type Error;

    fn submit(&mut self, op: ResolvedOp<'_>) -> Result<(), Self::Error>;
}

pub fn execute<B: Backend>(plan: &Plan, backend: &mut B) -> Result<(), B::Error> {
    for op in &plan.ops {
        backend.submit(resolve_op(plan, op))?;
    }
    Ok(())
}

fn resolve_op<'a>(plan: &'a Plan, op: &'a Op) -> ResolvedOp<'a> {
    match op {
        Op::BeginModel(name) => ResolvedOp::BeginModel(resolve_string(plan, *name)),
        Op::EndModel => ResolvedOp::EndModel,
        Op::Import(path) => ResolvedOp::Import(resolve_string(plan, *path)),
        Op::BeginOptimize(target) => ResolvedOp::BeginOptimize(resolve_target(plan, target)),
        Op::EndOptimize => ResolvedOp::EndOptimize,
        Op::BeginMaterial(name) => ResolvedOp::BeginMaterial(resolve_string(plan, *name)),
        Op::EndMaterial => ResolvedOp::EndMaterial,
        Op::Parameter { key, op, value } => ResolvedOp::Parameter {
            key: resolve_string(plan, *key),
            op: *op,
            value: resolve_value(plan, value),
        },
    }
}

fn resolve_target<'a>(plan: &'a Plan, target: &'a Target) -> ResolvedTarget<'a> {
    match target {
        Target::Android => ResolvedTarget::Android,
        Target::Web => ResolvedTarget::Web,
        Target::Desktop => ResolvedTarget::Desktop,
        Target::Named(name) => ResolvedTarget::Named(resolve_string(plan, *name)),
    }
}

fn resolve_value<'a>(plan: &'a Plan, value: &'a Value) -> ResolvedValue<'a> {
    match value {
        Value::Number(value) => ResolvedValue::Number(*value),
        Value::String(value) => ResolvedValue::String(resolve_string(plan, *value)),
        Value::Bool(value) => ResolvedValue::Bool(*value),
        Value::Identifier(value) => ResolvedValue::Identifier(resolve_string(plan, *value)),
    }
}

fn resolve_string(plan: &Plan, index: u32) -> &str {
    &plan.strings[index as usize]
}
