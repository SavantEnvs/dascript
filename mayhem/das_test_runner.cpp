// Functional-test host for mayhem/test.sh: compile a .das file from disk with the default
// module set, simulate it and run `main`. Script output goes to stdout; exits 1 on compile,
// simulate or runtime error (printing the diagnostics), 0 otherwise.
#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"
#include <cstdio>

using namespace das;

int main(int argc, char ** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s file.das\n", argv[0]); return 2; }
    NEED_MODULE(Module_BuiltIn);
    NEED_MODULE(Module_Math);
    NEED_MODULE(Module_Strings);
    NEED_MODULE(Module_Rtti);
    NEED_MODULE(Module_Ast);
    NEED_FUSION;
    Module::Initialize();
    int rc = 0;
    {
        TextPrinter tout;
        ModuleGroup libGroup;
        auto access = make_smart<FsFileAccess>();
        auto program = compileDaScript(argv[1], access, tout, libGroup);
        if (program->failed()) {
            for (auto & err : program->errors)
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            rc = 1;
        } else {
            Context ctx(program->getContextStackSize());
            if (!simulateProgram(program, ctx, tout)) {
                for (auto & err : program->errors)
                    tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                rc = 1;
            } else if (auto fn = ctx.findFunction("main")) {
                ctx.evalWithCatch(fn, nullptr);
                if (auto ex = ctx.getException()) { tout << "exception: " << ex << "\n"; rc = 1; }
            } else { tout << "no main\n"; rc = 1; }
        }
    }
    Module::Shutdown();
    return rc;
}
