#!/usr/bin/env bash
set -e

cd rust
trap 'cd ..' EXIT

echo "Building..."
cargo rustc --release -- --emit=asm
echo

echo "Running..."
cargo run --release -- 1
echo

ASM_COUNT=$(find target/release/deps -type f -name '*.s' | wc -l)
if [ "$ASM_COUNT" -ne 1 ]; then
    echo "Expected exactly one assembly file, found $ASM_COUNT" >&2
    exit 1
fi
ASM=$(find target/release/deps -type f -name '*.s' | head -n 1)

echo "Found assembly file: $ASM"
for kernel in comptime runtime; do
    sed -n "/^[[:space:]]*\\.section.*${kernel}/,/^[[:space:]]*\\.size.*${kernel}/p" "$ASM" > "target/${kernel}_rs.s"
done
echo

echo "SIMD instructions: compile-time stride"
grep -E '\b(mulps|addps)\b' target/comptime_rs.s || true
echo "SIMD instructions: runtime stride"
grep -E '\b(mulps|addps)\b' target/runtime_rs.s || true
echo

echo "Scalar instructions: compile-time stride"
grep -E '\b(mulss|addss)\b' target/comptime_rs.s || true
echo "Scalar instructions: runtime stride"
grep -E '\b(mulss|addss)\b' target/runtime_rs.s || true
echo

