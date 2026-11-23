// Fuzz target `dascript`: compile an untrusted .das script (lexer/parser gen1+gen2, type
// inference, constant folding, optimizer, lint, compile-time macros). Replaces the legacy
// `daScript @@ -dry-run` CLI target with the same code path, in process.
#include "fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {
    if (size > 0x100000) return -1;
    das_fuzz_init();
    TextWriter tout;
    ModuleGroup libGroup;
    auto access = das_fuzz_access("fuzz.das", data, size);
    CodeOfPolicies policies;
    auto program = compileDaScript("fuzz.das", access, tout, libGroup, policies);
    program.reset();
    return 0;
}
