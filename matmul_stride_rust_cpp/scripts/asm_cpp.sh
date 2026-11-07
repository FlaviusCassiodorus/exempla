#!/usr/bin/env bash
set -e

cd cpp
trap 'cd ..' EXIT

echo "Building..."
mkdir -p build
g++ -O3 -std=c++20 -masm=intel -save-temps=obj \
    simple.cpp -o build/simple
echo

echo "Running.."
build/simple 1
echo

ASM="build/simple.s"
echo "Assembly file: $ASM"
echo

# Extract the two functions
sed -n '/^_Z8comptime/,/^\.size.*_Z8comptime/p' "$ASM" > build/comptime_cpp.s
sed -n '/^_Z7runtime/,/^\.size.*_Z7runtime/p' "$ASM" > build/runtime_cpp.s

echo "SIMD instructions: compile-time stride"
grep -E '\b(mulps|addps)\b' build/comptime_cpp.s || true
echo "SIMD instructions: runtime stride"
grep -E '\b(mulps|addps)\b' build/runtime_cpp.s || true
echo

echo "Scalar instructions: compile-time stride"
grep -E '\b(mulss|addss)\b' build/comptime_cpp.s || true
echo "Scalar instructions: runtime stride"
grep -E '\b(mulss|addss)\b' build/runtime_cpp.s || true
echo

