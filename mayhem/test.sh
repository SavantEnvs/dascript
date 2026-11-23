#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Golden-output suite: each mayhem/tests/<name>.das is compiled and run by bin/das_test_runner
# (built by build.sh against the same libDaScript) and its stdout must equal <name>.expected.
# compile_error.das must be REJECTED by the compiler (exit 1 + an "error" diagnostic).
RUNNER="$SRC/bin/das_test_runner"
[ -x "$RUNNER" ] || { echo "missing $RUNNER (build.sh should have built it)" >&2; emit_ctrf das-golden 0 1; exit 1; }
passed=0; failed=0
for exp in "$SRC"/mayhem/tests/*.expected; do
  das="${exp%.expected}.das"
  if out=$("$RUNNER" "$das" 2>&1) && [ "$out" == "$(cat "$exp")" ]; then
    passed=$((passed+1)); echo "PASS $(basename "$das")"
  else
    failed=$((failed+1)); echo "FAIL $(basename "$das")"; echo "$out" | head -20
  fi
done
if out=$("$RUNNER" "$SRC/mayhem/tests/compile_error.das" 2>&1); then
  failed=$((failed+1)); echo "FAIL compile_error.das (compiled)"
elif grep -qi "error" <<<"$out" && grep -q "compile_error.das" <<<"$out"; then
  passed=$((passed+1)); echo "PASS compile_error.das"
else
  failed=$((failed+1)); echo "FAIL compile_error.das (no diagnostic)"; echo "$out" | head -20
fi
emit_ctrf das-golden "$passed" "$failed"
