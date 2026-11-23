// Fuzz target `dascript-json`: the runtime JSON-to-value scanner behind the `sscan_json` builtin
// (src/simulate/json_scan.cpp), reached the way a script reaches it. A fixed script is compiled
// and simulated once; each input is handed as a string to its exported `test`, which scans it
// into a value whose type covers every shape the scanner walks: scalars of each width, strings,
// fixed arrays, dynamic arrays, tables, pointers, variants, tuples, a recursive struct and a
// class pointer.
//
// State kept across inputs: the compiled program and its Context (read-only code). The context
// heaps and globals are reset before every input, so each result depends only on that input.
#include "fuzz_common.h"
#include <string>

static const char * kScript = R"DAS(options gen2
options rtti

struct Scalars {
    b : bool
    i : int
    u : uint
    i8 : int8
    u8 : uint8
    i16 : int16
    u16 : uint16
    i64 : int64
    u64 : uint64
    f : float
    d : double
    s : string
    v2 : float2
    v4 : int4
}

struct Layer {
    gain : float = 1.0
    enabled : bool = true
    name : string = "layer"
}

variant Slot {
    bits : uint4
    layer : Layer
    n : int
    txt : string
}

struct Tree {
    value : int = 3
    kids : array<Tree>
}

class Node {
    value : int = 5
    tag : string
}

struct Doc {
    sc : Scalars
    fixed : int[4]
    names : string[2]
    layers : array<Layer>
    byName : table<string; Layer>
    counts : table<string; int>
    keys : table<int; string>
    solo : Layer?
    slots : array<Slot>
    slot : Slot = Slot(n = 0)
    pairs : array<tuple<Layer; int>>
    tree : Tree = Tree()
    nodes : array<Node?>
    nested : array<array<float>>
}

[export]
def test(s : string) : bool {
    var doc = Doc()
    return sscan_json(s, doc)
}
)DAS";

static smart_ptr<Program> g_program;
static Context * g_ctx = nullptr;
static SimFunction * g_test = nullptr;

static void setup() {
    das_fuzz_init();
    TextWriter tout;
    ModuleGroup libGroup;
    auto access = das_fuzz_access("json.das", (const uint8_t *)kScript, strlen(kScript));
    g_program = compileDaScript("json.das", access, tout, libGroup);
    if (g_program->failed()) {
        for (auto & err : g_program->errors)
            fprintf(stderr, "%s", reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
        abort();
    }
    g_ctx = new Context(g_program->getContextStackSize());
    if (!simulateProgram(g_program, *g_ctx, tout)) abort();
    g_test = g_ctx->findFunction("test");
    if (!g_test) abort();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {
    if (size > 0x100000) return -1;
    // A daScript string is NUL-terminated and cannot hold a NUL byte (the scanner is handed a
    // `char *`), so an input with an embedded NUL is not a string a script could pass.
    if (memchr(data, 0, size)) return -1;
    if (!g_ctx) setup();
    std::string s((const char *)data, size);
    g_ctx->restart();
    g_ctx->restartHeaps();
    vec4f args[1];
    args[0] = cast<char *>::from((char *)s.c_str());
    g_ctx->evalWithCatch(g_test, args);
    return 0;
}
