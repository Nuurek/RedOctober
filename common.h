#include "instance.h"

#ifndef COMMON_H
#define COMMON_H

void packInstance(Instance &instance) {
    pvm_pkbyte((char*)&instance, sizeof(instance), 1);
}

void unpackInstance(Instance &instance) {
    pvm_upkbyte((char*)&instance, sizeof(instance), 1);
}

#endif
