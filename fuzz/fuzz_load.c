#include <stddef.h>
#include <stdint.h>
#include "src/bindings/default/qsp_default.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int initialized = 0;
    if (!initialized) { QSPInit(); initialized = 1; }
    QSPLoadGameWorldFromData(data, (int)size, QSP_FALSE);
    return 0;
}
