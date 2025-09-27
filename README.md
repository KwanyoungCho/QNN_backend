# QNN Backend Context Loader

This repository provides a minimal helper for reconstructing a QNN backend context and graph from a serialized context binary that was exported from an ExecuTorch `.pte` bundle. The helper mirrors the flow used in `executorch/examples/qualcomm/oss_scripts/llama/qnn_llama_runner.cpp`, but is condensed so that it can be integrated into other frameworks or Android test harnesses.

## Building

The code is standard C++17 and only depends on the QNN SDK headers/libraries and `dlopen`. To build for Android (aarch64), cross-compile the sources with the Android NDK and link against the QNN backend shared objects (`libQnnHtp.so`, etc.).

```bash
# Example: building a standalone binary with the Android NDK toolchain
ANDROID_NDK_HOME=/path/to/android/ndk \
$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang++ \
    -Iinclude \
    examples/qnn_context_main.cpp src/qnn_context_loader.cpp \
    -ldl -o qnn_context_main
```

Push the resulting binary to the Android device and invoke it under `adb shell`:

```bash
adb push qnn_context_main /data/local/tmp/
adb push your_context.bin /data/local/tmp/
adb push libQnnHtp.so /data/local/tmp/
adb shell "cd /data/local/tmp && LD_LIBRARY_PATH=. ./qnn_context_main ./libQnnHtp.so ./your_context.bin"
```

## API Overview

* `qnn::BackendLoader` wraps `dlopen`/`dlsym` and instantiates a QNN backend using the exported interface entry point (`QnnInterface`).
* `qnn::load_context_and_graph` reads a serialized context binary, deserializes it via `contextCreateFromBinary`, and materializes a graph handle via `graphCreateFromContext`.
* `examples/qnn_context_main.cpp` demonstrates how to wire these utilities into a simple CLI suitable for iterative testing on Android devices.

When linking against the real QNN SDK, include the appropriate headers **before** `qnn_context_loader.h` to ensure that the full `QnnInterface` definition (with the actual function pointer names) is visible to the compiler.

