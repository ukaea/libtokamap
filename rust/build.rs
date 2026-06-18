use std::env;
use std::path::PathBuf;

fn main() {
    // Get the path to the libtokamap source directory
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let libtokamap_root = PathBuf::from(&manifest_dir).parent().unwrap().to_path_buf();

    // Include directories for libtokamap
    let include_dirs = vec![
        libtokamap_root.join("src"),
        libtokamap_root.join("ext_include"),
        libtokamap_root.join("build").join("include"),
        libtokamap_root.join("include"),
    ];

    // Build the C++ bridge
    cxx_build::bridge("src/lib.rs")
        .file("src/bridge.cpp")
        .includes(&include_dirs)
        .std("c++20")
        .flag_if_supported("-Wall")
        .flag_if_supported("-Wextra")
        .flag_if_supported("-Werror")
        .compile("libtokamap_rust_bridge");

    // Link against libtokamap
    println!("cargo:rustc-link-lib=tokamap");

    // Add library search path
    let lib_dir = libtokamap_root.join("build").join("lib");
    if lib_dir.exists() {
        println!("cargo:rustc-link-search=native={}", lib_dir.display());
    }

    // Also check install directory
    let install_lib_dir = libtokamap_root.join("install").join("lib");
    if install_lib_dir.exists() {
        println!(
            "cargo:rustc-link-search=native={}",
            install_lib_dir.display()
        );
    }

    // Rerun if any of these files change
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=../src/handlers/mapping_handler.hpp");
    println!("cargo:rerun-if-changed=../src/handlers/mapping_handler.cpp");
}
