#include "cdt.h"

int main(int argc, const char** argv) {
    os_api os;
    cdt cdt;
    cdt.os = &os;
    if (!init_cdt(argc, argv, cdt)) {
        return 1;
    }
    while (true) {
        exec_cdt_systems(cdt);
    }
}
