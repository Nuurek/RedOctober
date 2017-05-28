#include <stdio.h>
#include <stdlib.h>
#include <pvm3.h>

#ifndef DEF_H
#define DEF_H


#define SLAVE_NAME "slave"
#define MAX_SLAVES_NUMBER 32
#define DEFAULT_SLAVES_NUMBER 4
#define MAX_CANALS_NUMBER 256
#define DEFAULT_CANALS_NUMBER 2
#define MAX_CANAL_SIZE 4

#define INITIAL_DATA_TAG 0
#define REPORT_TAG 1
#define EXIT_TAG 2
#define STATE_TAG 3
#define REQUEST_TAG 4
#define REPORT_MESSAGE_SIZE 128
#define SLAVE_GROUP "slave_group"

#define MAX_BASE_TIME 2
#define MAX_CRUISE_TIME 5
#define MAX_MISSION_TIME 2

#endif
