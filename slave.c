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
		localState.requestTimestamp = -1;

		if (instance.slaveTIds[i] == myTId) {
			myId = i;
			state = localStates[myId];
		}
	}
}

struct timeval timeDifference(struct timeval& start, struct timeval& stop) {
	struct timeval result;

	if ((stop.tv_usec - start.tv_usec) < 0) {
        result.tv_sec = stop.tv_sec - start.tv_sec - 1;
        result.tv_usec = stop.tv_usec - start.tv_usec + 1000000;
    } else {
        result.tv_sec = stop.tv_sec - start.tv_sec;
        result.tv_usec = stop.tv_usec - start.tv_usec;
    }

    return result;
}

void inline incrementTimeStamp() {
	++(state.timestamp);
}

void inline updateTimeStamp(long timestamp) {
	if (timestamp > state.timestamp) {
		state.timestamp = timestamp + 1;
	} else {
		incrementTimeStamp();
	}
}

int receiveMessage() {
	State slaveState;
	unpackState(slaveState);

	updateTimeStamp(slaveState.timestamp);

	localStates[slaveState.id] = slaveState;
	reportToMaster("Received message from %d, its timestamp: %d", slaveState.id, localStates[slaveState.id].timestamp);
	return slaveState.id;
}

void prepareToSendState() {
	incrementTimeStamp();
	pvm_initsend(PvmDataDefault);
	packState(state);
}

void sendStateMessage(int slaveId) {
	prepareToSendState();

	pvm_send(slaveId, STATE_TAG);
}

int receiveRequestAndResponse() {
	int requestSlaveId = receiveMessage();
	int tId = instance.slaveTIds[requestSlaveId];
	sendStateMessage(tId);
	return requestSlaveId;
}

void sleepAndResponse(unsigned int secondsToSleep) {
	struct timeval period;
	period.tv_sec = secondsToSleep;
	period.tv_usec = 0;

	struct timeval  start, now, difference;
	gettimeofday(&start, NULL);
	int requestSlaveId;
	while(pvm_trecv(-1, REQUEST_TAG, &period)) {
		receiveRequestAndResponse();

		gettimeofday (&now, NULL);
		// period -= (now - start)
		difference = timeDifference(start, now);
		period = timeDifference(difference, period);
		start = now;
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
	sleepAndResponse(period);

	state.section = REQUEST;
}

void broadcastStateMessage() {
	prepareToSendState();

	pvm_bcast(SLAVE_GROUP, REQUEST_TAG);
}

bool requestCondition() {
	int sameCanalRequests = 1;
	for (int i = 0; i < instance.slavesNumber; i++) {
		if (i == myId) {
			continue;
		}

		State& otherState = localStates[i];

		reportToMaster("%d timestamp: %d", i, otherState.timestamp);
		if (otherState.timestamp <= state.requestTimestamp) {
			reportToMaster("Not gathered fresh states");
			return false;
		}
		if (otherState.canal == state.canal && otherState.requestTimestamp <= state.requestTimestamp) {
			reportToMaster("Another request from %d with %d", i, otherState.requestTimestamp);
			++sameCanalRequests;
		}
	}

	if (sameCanalRequests <= instance.canalSizes[state.canal]) {
		reportToMaster("Canal has space");
		return true;
	} else {
		reportToMaster("Canal has no space");
		return false;
	}
}

void requestSection() {
	state.canal = rand() % instance.canalsNumber;

	broadcastStateMessage();
	state.requestTimestamp = state.timestamp;
	reportToMaster("Broadcasted request message on %d", state.requestTimestamp);

	int requestResponses = 0;
	while (!requestCondition()) {
		int bufferId = pvm_recv(-1, -1);
		int tag;
		pvm_bufinfo(bufferId, NULL, &tag, NULL);
		int requestSlaveId;
		switch (tag) {
			case REQUEST_TAG:
				requestSlaveId = receiveRequestAndResponse();
				reportToMaster("Received request from %d", requestSlaveId);
				break;
			case STATE_TAG:
				receiveMessage();
				++requestResponses;
				reportToMaster("Received response to request no. %d", requestResponses);
				break;
		}
	}

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
	sleepAndResponse(period);

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
	}
}
