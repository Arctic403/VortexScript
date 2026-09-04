use vortex_script_core::compile_source;
use vortex_script_core::ir::Op;

const SAMPLE: &str = include_str!("../../../examples/android_asset.vxs");

#[test]
fn compiles_android_asset_plan() {
    let plan = compile_source(SAMPLE).expect("sample should compile");

    assert!(!plan.strings.is_empty());
    assert!(plan.ops.iter().any(|op| matches!(op, Op::Import(_))));
    assert!(plan.ops.iter().any(|op| matches!(op, Op::BeginOptimize(_))));
    assert!(plan.ops.iter().any(|op| matches!(op, Op::BeginMaterial(_))));

    let encoded = plan.encode();
    assert_eq!(&encoded[0..4], b"VXS1");
}

#[test]
fn reports_invalid_source() {
    let diagnostics = compile_source("model { import 42 }").expect_err("source is invalid");
    assert!(!diagnostics.is_empty());
}
