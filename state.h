#include "def.h"


#ifndef STATE_H
#define STATE_H

enum Section {
    LOCAL,
    REQUEST,
    CRITICAL
};

enum Position {
    BASE,
    MISSION
};

struct State {
    int id;
    enum Section section;
    enum Position position;
    int canal;
    unsigned long timestamp;
};
typedef struct State State;

#endif
