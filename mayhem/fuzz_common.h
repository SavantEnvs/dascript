// Shared setup for the daScript libFuzzer harnesses (mayhem/ layer, not upstream code).
//
// Module set: the default builtin modules a host registers (NEED_ALL_DEFAULT_MODULES in
// include/daScript/daScriptModule.h) MINUS fio, dasbind (native FFI), network, jit and
// debugger. Script macros execute during compilation, so without this cut a fuzz input could
// write files, open sockets or call arbitrary native functions by design. Module registration
// is process-global and read-only after Module::Initialize(); every input gets its own
// FileAccess, Program and Context, which are released before the next input.
#pragma once
#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"
#include <cstring>

using namespace das;

static inline void das_fuzz_init() {
    static bool done = false;
    if (done) return;
    done = true;
    NEED_MODULE(Module_BuiltIn);
    NEED_MODULE(Module_Math);
    NEED_MODULE(Module_Strings);
    NEED_MODULE(Module_Rtti);
    NEED_MODULE(Module_Ast);
    NEED_FUSION;
    Module::Initialize();
}

// In-memory script file, exactly `size` bytes (no NUL terminator), owned by the FileAccess —
// the same thing the C API's das_fileaccess_introduce_file_n(..., owns=1) does.
static inline smart_ptr<FsFileAccess> das_fuzz_access(const char * name, const uint8_t * data, size_t size) {
    auto access = make_smart<FsFileAccess>();
    uint32_t len = (uint32_t)size;
    char * copy = (char *) das_aligned_alloc16(len ? len : 1);
    if (len) memcpy(copy, data, len);
    access->setFileInfo(name, make_unique<TextFileInfo>(copy, len, true));
    return access;
}
