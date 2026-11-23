#!/usr/bin/env bash
# mayhem/build.sh — build the daScript fuzz harnesses and the functional-test runner.
#
# One CMake build of the static compiler+runtime (libDaScript, libDaScript_runtime and the bundled
# uriparser) with $SANITIZER_FLAGS + SanCov + $DEBUG_FLAGS, then each harness linked twice
# (libFuzzer + standalone reproducer) into /mayhem/bin, plus mayhem/das_test_runner (a tiny
# compile-and-run host used by test.sh). Binaries live in bin/ because daScript resolves its root
# (daslib/) as the parent of the executable's directory.
#
# UBSan runs with every check on. Upstream's ubsan lane also turns off signed-integer-overflow,
# shift and enum, but none of them fires here: CMakeCommon.txt builds every library TU with
# -fwrapv (script ints wrap by definition), the interpreter masks shift counts (sim_policy.h
# BinShl), and script enums are plain ints, not C++ enum loads.
# (vecmath auto-detects ASan and selects its sanitizer-safe loads.)
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export CC CXX
cd "$SRC"

FLAGS="-O1 $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS"
B=build/mayhem-fuzz   # build/ is gitignored upstream

cmake -S . -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=MayhemFuzz \
  -DCMAKE_C_FLAGS="$FLAGS" -DCMAKE_CXX_FLAGS="$FLAGS" \
  -DDAS_FLEX_BISON_DISABLED=ON -DDAS_TESTS_DISABLED=ON -DDAS_TUTORIAL_DISABLED=ON \
  -DDAS_AOT_EXAMPLES_DISABLED=ON -DDAS_HV_DISABLED=ON -DDAS_GLFW_DISABLED=ON -DDAS_IMGUI_DISABLED=ON \
  -DDAS_VULKAN_DISABLED=ON -DDAS_AUDIO_DISABLED=ON -DDAS_STDDLG_DISABLED=ON \
  -DDAS_MESHOPTIMIZER_DISABLED=ON -DDAS_STBIMAGE_DISABLED=ON -DDAS_CLIPBOARD_DISABLED=ON \
  -DDAS_PUGIXML_DISABLED=ON -DDAS_CUDA_DISABLED=ON -DDAS_TREE_SITTER_DISABLED=ON \
  -DDAS_TERMINAL_DISABLED=ON
cmake --build "$B" -j"$MAYHEM_JOBS" --target libDaScript

LIBS=(lib/liblibDaScript.a lib/liblibDaScript_runtime.a)
for l in lib/liblibUriParser.a lib/libUriParser.a; do [ -f "$l" ] && LIBS+=("$l"); done
LINK=(-Wl,--start-group "${LIBS[@]}" -Wl,--end-group -lpthread -ldl -lm)
INC=(-Iinclude -I3rdparty)
CXXF="-std=c++17 -O1 $SANITIZER_FLAGS $DEBUG_FLAGS"

O=build/mayhem-obj; mkdir -p "$O" bin
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$O/standalone_main.o"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$O/lsan_off.o"

for h in fuzz_dascript fuzz_dascript_run fuzz_isUtf8Text fuzz_sscan_json fuzz_deserialize; do
  $CXX $CXXF "${INC[@]}" -c "mayhem/$h.cpp" -o "$O/$h.o"
  $CXX $CXXF $LIB_FUZZING_ENGINE "$O/$h.o" "$O/lsan_off.o" "${LINK[@]}" -o "bin/$h"
  $CXX $CXXF "$O/standalone_main.o" "$O/$h.o" "$O/lsan_off.o" "${LINK[@]}" -o "bin/$h-standalone"
done

# Functional-test host (test.sh): same library build, plain run-and-print driver.
$CXX $CXXF $COVERAGE_FLAGS "${INC[@]}" mayhem/das_test_runner.cpp "$O/lsan_off.o" "${LINK[@]}" -o bin/das_test_runner
