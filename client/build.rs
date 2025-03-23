use std::io::{Result, Error};

fn main() -> Result<()> {
    println!("cargo:rerun-if-changed=../proto/windowcaster.proto");
    
    protobuf_codegen::Codegen::new()
        .pure()
        .includes(&["../proto"])
        .input("../proto/windowcaster.proto")
        .cargo_out_dir("protos")
        .run()
        .map_err(|e| Error::new(std::io::ErrorKind::Other, e.to_string()))?;

    Ok(())
} 