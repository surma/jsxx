# jsxx

`jsxx` is a transpiler that compiles JavaScript to C++.

More details can be found in the [blog post].

## Usage

`jsxx` reads JavaScript from stdin and compiles it to C++ and then uses `clang++` to create a binary.

```
$ cat testprog.js | cargo run
$ ./output
1.000000,2.000000,3.000000
``` 

If you have [WASI-SDK] installed, you can also compile straight to WebAssembly:

```
$ cat testprog.js | \
    cargo run -- \ 
    --wasm \
    --clang-path $HOME/Downloads/wasi-sdk-16.0/bin/clang++ \
    -- -Oz -flto -Wl,--lto-O3
$ wasmtime output.wasm
1.000000,2.000000,3.000000
```

If you want to inspect the generated C++ code, use `--emit-cpp`.


---
Apache 2.0

[blog post]: https://surma.dev/things/compile-js
[WASI-SDK]: https://github.com/WebAssembly/wasi-sdk
