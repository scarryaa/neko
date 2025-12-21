fn main() {
    cxx_build::bridge("src/ffi/bridge.rs")
        .flag_if_supported("-std=c++17")
        .compile("neko-core_cxx");

    println!("cargo:rerun-if-changed=src/ffi/bridge.rs");
    println!("cargo:rerun-if-changed=src/ffi/conversions.rs");
    println!("cargo:rerun-if-changed=src/ffi/wrappers.rs");
}
