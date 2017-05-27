#include <stdarg.h>
#include <string.h>
#include <unistd.h>
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
	int myTId = pvm_mytid();
	for (i = 0; i < instance.slavesNumber; i++) {
		localState = localStates[i];
		localState.id = i;
		localState.section = LOCAL;
		localState.position = BASE;
		localState.canal = -1;
		localState.timestamp = 0;

		if (instance.slaveTIds[i] == myTId) {
			myId = i;
			state = localStates[myId];
		}
	}
}

void localSection() {
	unsigned int period;
	switch (state.position) {
		case BASE:
			period = rand() % MAX_BASE_TIME + 1;
			reportToMaster("Replenishing stocks in the BASE for %d seconds", period);
			break;

		case MISSION:
			period = rand() % MAX_MISSION_TIME + 1;
			reportToMaster("Carrying out the MISSION for %d seconds", period);
			break;
	}
	sleep(period);

	state.section = REQUEST;
}

void requestSection() {
	state.canal = rand() % instance.canalsNumber;

	state.section = CRITICAL;
}

void criticalSection() {
	unsigned int period = rand() % MAX_CRUISE_TIME + 1;
	switch (state.position) {
		case BASE:
			reportToMaster("Starting cruise from BASE to MISSION through canal %d for %d seconds", state.canal, period);
			state.position = MISSION;
			break;

		case MISSION:
			reportToMaster("Starting cruise from MISSION to BASE through canal %d for %d seconds", state.canal, period);
			state.position = BASE;
			break;
	}
	sleep(period);

	state.section = LOCAL;
}

main(int argc, char const *args[])
{
	srand(pvm_mytid());

	pvm_recv(-1, INITIAL_DATA_TAG);
	unpackInstance(instance);

	initializeLocalStates();
	reportToMaster("Initialized");

	pvm_joingroup(SLAVE_GROUP);
	pvm_barrier(SLAVE_GROUP, instance.slavesNumber);
	reportToMaster("Group created");

	while (true) {
		switch (state.section) {
			case REQUEST:
				requestSection();
				break;

			case CRITICAL:
				criticalSection();
				break;

			case LOCAL:
				localSection();
				break;
		}
		++state.timestamp;
	}
}
