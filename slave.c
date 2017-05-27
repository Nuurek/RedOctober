#include <stdarg.h>
#include <string.h>
#include "instance.h"
#include "state.h"
#include "common.h"


Instance instance;
int myId = -1;
State localStates[MAX_SLAVES_NUMBER];
State& state = localStates[0];

void reportToMaster(const char* message, ...) {
	va_list argp;
	va_start(argp, message);
	char messageBuffer[REPORT_MESSAGE_SIZE];
	vsprintf(messageBuffer, message, argp);
	va_end(argp);

	char buffer[REPORT_MESSAGE_SIZE];
	sprintf(buffer, "[%d@%d]: ", myId, state.timestamp);

	strcat(buffer, messageBuffer);

	pvm_initsend(PvmDataDefault);
	pvm_pkstr(buffer);
	pvm_send(instance.masterTId, REPORT_TAG);
}

void initializeLocalStates() {
	int i;
	State& localState = localStates[0];
	for (i = 0; i < instance.slavesNumber; i++) {
		localState = localStates[i];
		localState.section = LOCAL;
		localState.position = BASE;
		localState.canal = -1;
		localState.timestamp = 0;
	}
}

main(int argc, char const *args[])
{
	int myTId = pvm_mytid();

	pvm_recv(-1, INITIAL_DATA_TAG);
	unpackInstance(instance);

	int i = 0;
	for (i = 0; i < instance.slavesNumber; i++) {
		if (instance.slaveTIds[i] == myTId) {
			myId = i;
			state = localStates[myId];
		}
	}

	reportToMaster("Initiated.");

	pvm_exit();
}
