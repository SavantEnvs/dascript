// Fuzz target `dascript-run`: compile an untrusted .das script, simulate it and run its
// `main` in the interpreter (ast_simulate, SimNodes, fusion, heap, runtime arrays/tables/strings).
//
// Policies:
//  - no_unsafe: the language contract puts `unsafe` blocks (raw pointers, reinterpret) outside
//    its memory-safety guarantee, so they are rejected with the library's own switch.
//  - max_fast_call_depth: the interpreter's opt-in guard (upstream 1b0510c2f1) that turns
//    unbounded recursion into a recoverable panic instead of a native stack overflow.
//  - heap limits keep a runaway allocation an in-script panic rather than an OOM of the host.
#include "fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {
    if (size > 0x100000) return -1;
    das_fuzz_init();
    TextWriter tout;
    ModuleGroup libGroup;
    auto access = das_fuzz_access("fuzz.das", data, size);
    CodeOfPolicies policies;
    policies.no_unsafe = true;
    policies.max_fast_call_depth = 4096;
    policies.max_heap_allocated = 256u << 20;
    policies.max_string_heap_allocated = 256u << 20;
    auto program = compileDaScript("fuzz.das", access, tout, libGroup, policies);
    if (!program->failed()) {
        Context ctx(program->getContextStackSize());
        if (simulateProgram(program, ctx, tout)) {
            if (auto fn = ctx.findFunction("main")) {
                if (fn->debugInfo->count == 0) {
                    ctx.evalWithCatch(fn, nullptr);
                }
            }
        }
    }
    program.reset();
    return 0;
}
