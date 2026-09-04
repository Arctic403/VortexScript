#[derive(Debug, Clone, PartialEq)]
pub struct Plan {
    pub strings: Vec<String>,
    pub ops: Vec<Op>,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Op {
    BeginModel(u32),
    EndModel,
    Import(u32),
    BeginOptimize(Target),
    EndOptimize,
    BeginMaterial(u32),
    EndMaterial,
    Parameter {
        key: u32,
        op: CompareCode,
        value: Value,
    },
}

#[derive(Debug, Clone, PartialEq)]
pub enum Target {
    Android,
    Web,
    Desktop,
    Named(u32),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum CompareCode {
    Set = 0,
    Equal = 1,
    LessEqual = 2,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Number(f32),
    String(u32),
    Bool(bool),
    Identifier(u32),
}

impl Plan {
    pub fn encode(&self) -> Vec<u8> {
        let mut output = Vec::new();
        output.extend_from_slice(b"VXS1");
        output.extend_from_slice(&1u16.to_le_bytes());

        write_u32(&mut output, self.strings.len() as u32);
        for value in &self.strings {
            write_u32(&mut output, value.len() as u32);
            output.extend_from_slice(value.as_bytes());
        }

        write_u32(&mut output, self.ops.len() as u32);
        for op in &self.ops {
            encode_op(&mut output, op);
        }

        output
    }
}

fn encode_op(output: &mut Vec<u8>, op: &Op) {
    match op {
        Op::BeginModel(name) => {
            output.push(0x01);
            write_u32(output, *name);
        }
        Op::EndModel => output.push(0x02),
        Op::Import(path) => {
            output.push(0x10);
            write_u32(output, *path);
        }
        Op::BeginOptimize(target) => {
            output.push(0x20);
            encode_target(output, target);
        }
        Op::EndOptimize => output.push(0x21),
        Op::BeginMaterial(name) => {
            output.push(0x30);
            write_u32(output, *name);
        }
        Op::EndMaterial => output.push(0x31),
        Op::Parameter { key, op, value } => {
            output.push(0x40);
            write_u32(output, *key);
            output.push(*op as u8);
            encode_value(output, value);
        }
    }
}

fn encode_target(output: &mut Vec<u8>, target: &Target) {
    match target {
        Target::Android => output.push(0),
        Target::Web => output.push(1),
        Target::Desktop => output.push(2),
        Target::Named(name) => {
            output.push(3);
            write_u32(output, *name);
        }
    }
}

fn encode_value(output: &mut Vec<u8>, value: &Value) {
    match value {
        Value::Number(value) => {
            output.push(0);
            output.extend_from_slice(&value.to_le_bytes());
        }
        Value::String(value) => {
            output.push(1);
            write_u32(output, *value);
        }
        Value::Bool(value) => {
            output.push(2);
            output.push(u8::from(*value));
        }
        Value::Identifier(value) => {
            output.push(3);
            write_u32(output, *value);
        }
    }
}

fn write_u32(output: &mut Vec<u8>, value: u32) {
    output.extend_from_slice(&value.to_le_bytes());
}
