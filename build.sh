#!/bin/sh
# ./build.sh lambda_lite.cpp
# ./build.sh lambda_full.cpp
if [ $# -ne 1 ]; then
    echo "Usage: $0 <source>"
    exit 1
fi
source=$1
name=${source%.*}
compiler="g++"
machine=$($compiler -dumpmachine)
version=$(git describe --tags --always --dirty="-dev" 2>/dev/null || echo "unknown")
os_name=$(uname -s)
mkdir -p build
case "$os_name" in
Windows* | MINGW*)
    stack_size="4294967296" # "8388608"
    $compiler -std=c++20 -O3 $source -o build/$name-$version-$machine-strint.exe -DSTACK_MAX=$stack_size -Wl,--stack,$stack_size || exit 1
    $compiler -std=c++20 -O3 $source -o build/$name-$version-$machine-gmpint.exe -DSTACK_MAX=$stack_size -Wl,--stack,$stack_size -DUSE_GMP -lgmp || exit 1
    ;;
Linux*)
    stack_size="8388608" # "4294967296"
    $compiler -std=c++20 -O3 $source -o build/$name-$version-$machine-strint -DSTACK_MAX=$stack_size -Wl,-z,stack-size=$stack_size -DSTACK_MAX=$stack_size || exit 1
    $compiler -std=c++20 -O3 $source -o build/$name-$version-$machine-gmpint -DSTACK_MAX=$stack_size -Wl,-z,stack-size=$stack_size -DSTACK_MAX=$stack_size -DUSE_GMP -lgmp || exit 1
    ;;
*)
    echo "Unsupported OS: $os_name" >&2
    exit 1
    ;;
esac
