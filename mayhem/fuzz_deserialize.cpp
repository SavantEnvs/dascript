// Fuzz target `dascript-deserialize`: the serialized-program (AST stream) reader — the format of
// the on-disk module cache (.dascache records) and of the public C API's
// das_program_serialize/das_program_deserialize_n blobs. Upstream hardens this reader against
// corrupt records (AstSerializer::verifyLength cites a module-cache corruption crash).
//
// Entry: exactly the module-cache read in compileDaScript (src/ast/ast_parse.cpp, the
// serializer_read->trySerialize block): an AstSerializer reading the bytes, with thisModuleGroup
// and fileAccess bound, through the noexcept AstSerializer::serializeProgram rail, which turns a
// corrupt stream into ser.failed. (das_program_deserialize_n itself is not used as the entry: it
// leaves thisModuleGroup null, so EVERY valid blob — even an empty script's — hits a UBSan
// "reference binding to null pointer of type 'ModuleGroup'" in FinAnnotationVisitor
// (src/ast/ast_annotations.cpp:44) and every input would stop there; see the PR findings.)
//
// State kept across inputs: only the module registry (read-only after das_fuzz_init). Each input
// gets its own storage, serializer, FileAccess, ModuleGroup and Program, all released before the
// next input (the program first: its LineInfos point at FileInfos the serializer owns).
// Seeds: das_program_serialize blobs of small compiled scripts.
#include "fuzz_common.h"
#include "daScript/ast/ast_serializer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {
    if (size > 0x400000) return -1;
    das_fuzz_init();
    SerializationStorageVector storage;
    storage.buffer.assign(data, data + size);
    auto access = make_smart<FsFileAccess>();
    ModuleGroup libGroup;
    {
        AstSerializer deser(&storage, false);
        deser.quietCache = true;
        deser.thisModuleGroup = &libGroup;
        deser.fileAccess = access.get();
        ProgramPtr program = make_smart<Program>();
        deser.serializeProgram(program, libGroup);
        program.reset();
        deser.moduleLibrary = nullptr;
    }
    return 0;
}
