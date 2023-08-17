#!/bin/sh
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
case "$os_name" in
Windows* | MINGW*)
    stack_size="4294967296" # "8388608"
    $compiler -std=c++20 -O3 $source -o $name-$version-$machine-strint.exe -Wl,--stack,$stack_size -DSTACK_MAX=$stack_size
    $compiler -std=c++20 -O3 $source -o $name-$version-$machine-gmpint.exe -Wl,--stack,$stack_size -DSTACK_MAX=$stack_size -DUSE_GMP -lgmp
    ;;
Linux*)
    stack_size="8388608" # "4294967296"
    $compiler -std=c++20 -O3 $source -o $name-$version-$machine-strint -Wl,-z,stack-size=$stack_size -DSTACK_MAX=$stack_size
    $compiler -std=c++20 -O3 $source -o $name-$version-$machine-gmpint -Wl,-z,stack-size=$stack_size -DSTACK_MAX=$stack_size -DUSE_GMP -lgmp
    ;;
*)
    echo "Unsupported OS: $os_name"
    ;;
esac
