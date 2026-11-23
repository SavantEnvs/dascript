// Ported from the legacy mayhemheroes/dascript target `isutf8text`: das::isUtf8Text
// (src/ast/ast_parse.cpp) is the BOM/UTF-8 sniff run on every script source before parsing.
#include <stdint.h>
#include <stddef.h>

namespace das {
    extern bool isUtf8Text(const char * src, uint32_t length);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {
    if (size > 0xffffffffu) return -1;
    das::isUtf8Text((const char *)data, (uint32_t)size);
    return 0;
}
