fn main() {
    let bridge_file = "src/ffi/mod.rs";

    cxx_build::bridge(bridge_file)
        .flag_if_supported("-std=c++17")
        .compile("neko_core_cxx");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed={bridge_file}");
}
