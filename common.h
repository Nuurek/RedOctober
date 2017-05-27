#include <time.h>
#include "instance.h"
#include "state.h"

#ifndef COMMON_H
#define COMMON_H

void packInstance(Instance& instance) {
    pvm_pkbyte((char*)&instance, sizeof(instance), 1);
}

void unpackInstance(Instance& instance) {
    pvm_upkbyte((char*)&instance, sizeof(instance), 1);
}

void packState(State& state) {
    pvm_pkbyte((char*)&state, sizeof(state), 1);
}

void unpackState(State& state) {
    pvm_pkbyte((char*)&state, sizeof(state), 1);
}

void seedRandomEngine() {
	int seed;
	time_t now;
	seed = time(&now);
	srand(seed);
}
#endif
