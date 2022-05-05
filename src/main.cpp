#include "cdt.h"

int main(int argc, const char** argv) {
    OsApi os;
    Cdt cdt;
    cdt.os = &os;
    if (!InitCdt(argc, argv, cdt)) {
        return 1;
    }
    while (true) {
        ExecCdtSystems(cdt);
    }
}
