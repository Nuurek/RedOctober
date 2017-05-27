#include <stdio.h>
#include <stdlib.h>
#include <pvm3.h>

#ifndef DEF_H
#define DEF_H

#define SLAVE_NAME "slave"
#define SLAVENUM 'z' - 'a' + 1
#define PASSSIZE 5
#define SALT "aa"
#define TARGET_HASH "aa7uckMQPpN46"

#define NAMESIZE   64

#define MSG_MSTR 1
#define MSG_SLV  2

#define GROUPNAME "dbxlab"

#define MAX_SLAVES_NUMBER 32
#define DEFAULT_SLAVES_NUMBER 4
#define MAX_CANALS_NUMBER 256
#define DEFAULT_CANALS_NUMBER 2
#define MAX_CANAL_SIZE 4

#define INITIAL_DATA_TAG 0
#define REPORT_TAG 1
#define REPORT_MESSAGE_SIZE 128
#define SLAVE_GROUP "slave_group"

#endif
