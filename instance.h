#include "def.h"

#ifndef INSTANCE_H
#define INSTANCE_H

struct Instance {
    int masterTId;
    int slavesNumber;
    int slaveTIds[MAX_SLAVES_NUMBER];
    int canalsNumber;
    int canalSizes[MAX_CANALS_NUMBER];
};
typedef struct Instance Instance;

#endif
