use std::env;
use std::fs;
use std::path::{Path, PathBuf};

use vortex_script_core::compile_source;

fn main() {
    if let Err(error) = run() {
        eprintln!("vortex-script: {error}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let mut args = env::args().skip(1);
    let command = args.next().ok_or_else(usage)?;
    let input = args.next().ok_or_else(usage)?;

    match command.as_str() {
        "check" => check(Path::new(&input)),
        "compile" => {
            let output = args.next().map(PathBuf::from).unwrap_or_else(|| {
                let mut path = PathBuf::from(&input);
                path.set_extension("vxb");
                path
            });
            compile(Path::new(&input), &output)
        }
        _ => Err(usage()),
    }
}

fn check(input: &Path) -> Result<(), String> {
    let source = read_source(input)?;
    let plan = compile_source(&source).map_err(render_diagnostics)?;
    println!(
        "ok: {} interned strings, {} operations",
        plan.strings.len(),
        plan.ops.len()
    );
    Ok(())
}

fn compile(input: &Path, output: &Path) -> Result<(), String> {
    let source = read_source(input)?;
    let plan = compile_source(&source).map_err(render_diagnostics)?;
    let bytes = plan.encode();
    fs::write(output, &bytes).map_err(|error| format!("failed to write {}: {error}", output.display()))?;
    println!("wrote {} bytes to {}", bytes.len(), output.display());
    Ok(())
}

fn read_source(path: &Path) -> Result<String, String> {
    fs::read_to_string(path).map_err(|error| format!("failed to read {}: {error}", path.display()))
}

fn render_diagnostics(diagnostics: Vec<vortex_script_core::Diagnostic>) -> String {
    diagnostics
        .into_iter()
        .map(|diagnostic| diagnostic.to_string())
        .collect::<Vec<_>>()
        .join("\n")
}

fn usage() -> String {
    "usage: vortex-script <check|compile> <input.vxs> [output.vxb]".to_owned()
}
