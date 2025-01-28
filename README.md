😸

my first analysis pass

## Install dependencies

```bash
sudo apt-get install -y llvm-20 llvm-20-dev llvm-20-tools clang-20
LLVM_DIR=/usr/lib/llvm-20
```

## Build

```bash
mkdir build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..
make -j$(nproc)
```

## Run

```bash
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR
$LLVM_DIR/bin/clang -O1 -S -emit-llvm ./inputs/hello.c -o hello.ll
$LLVM_DIR/bin/opt -load-pass-plugin ./build/lib/libhelloworld.so -passes=hello-world -disable-output hello.ll
```
