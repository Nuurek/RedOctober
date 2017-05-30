#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "instance.h"
#include "state.h"
#include "common.h"


Instance instance;
int myId = -1;
State localStates[MAX_SLAVES_NUMBER];


void reportToMaster(const char* message, ...) {
	va_list argp;
	va_start(argp, message);
	char messageBuffer[REPORT_MESSAGE_SIZE];
	vsprintf(messageBuffer, message, argp);
	va_end(argp);

	char buffer[REPORT_MESSAGE_SIZE];
	sprintf(buffer, "[%d@%d]: ", myId, localStates[myId].timestamp);

	strcat(buffer, messageBuffer);

	pvm_initsend(PvmDataDefault);
	pvm_pkstr(buffer);
	pvm_send(instance.masterTId, REPORT_TAG);
}

void reportStateToMaster(State otherState) {
	reportToMaster("[%d], id: %d, ts: %d, canal: %d, REQts: %d",
	localStates[myId].id == otherState.id, otherState.id, otherState.timestamp, otherState.canal, otherState.requestTimestamp);
}

void initializeLocalStates() {
	int i;
	int myTId = pvm_mytid();
	for (i = 0; i < instance.slavesNumber; i++) {
		State& localState = localStates[i];
		localState.id = i;
		localState.section = LOCAL;
		localState.position = BASE;
		localState.canal = -1;
		localState.timestamp = 0;
		localState.requestTimestamp = -1;

		if (instance.slaveTIds[i] == myTId) {
			myId = i;
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
	++(localStates[myId].timestamp);
}

void inline updateTimeStamp(long timestamp) {
	if (timestamp > localStates[myId].timestamp) {
		localStates[myId].timestamp = timestamp + 1;
	} else {
		incrementTimeStamp();
	}
}

int receiveMessage() {
	State slaveState;
	unpackState(slaveState);

	updateTimeStamp(slaveState.timestamp);

	localStates[slaveState.id] = slaveState;
	reportToMaster("Received state:");
	return slaveState.id;
}

void prepareToSendState() {
	incrementTimeStamp();
	pvm_initsend(PvmDataDefault);
	packState(localStates[myId]);
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
	switch (localStates[myId].position) {
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

	localStates[myId].section = REQUEST;
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

		if (otherState.timestamp <= localStates[myId].requestTimestamp) {
			return false;
		}

		if (otherState.canal == localStates[myId].canal) {
			if (otherState.requestTimestamp < localStates[myId].requestTimestamp ||
				otherState.requestTimestamp == localStates[myId].requestTimestamp && otherState.id < localStates[myId].id) {
					++sameCanalRequests;
				}
		}
	}

	if (sameCanalRequests <= instance.canalSizes[localStates[myId].canal]) {
		return true;
	} else {
		return false;
	}
}

void requestSection() {
	localStates[myId].canal = rand() % instance.canalsNumber;

	broadcastStateMessage();
	reportToMaster("Requested canal no %d", localStates[myId].canal);
	localStates[myId].requestTimestamp = localStates[myId].timestamp;

	int requestResponses = 0;
	while (!requestCondition()) {
		int bufferId = pvm_recv(-1, -1);
		int tag;
		pvm_bufinfo(bufferId, NULL, &tag, NULL);
		int requestSlaveId;
		switch (tag) {
			case REQUEST_TAG:
				requestSlaveId = receiveRequestAndResponse();
				break;
			case STATE_TAG:
				receiveMessage();
				++requestResponses;
				break;
		}
	}

	localStates[myId].section = CRITICAL;
}

void criticalSection() {
	unsigned int period = rand() % MAX_CRUISE_TIME + 1;
	switch (localStates[myId].position) {
		case BASE:
			reportToMaster("Starting cruise from BASE to MISSION through canal %d for %d seconds", localStates[myId].canal, period);
			localStates[myId].position = MISSION;
			break;

		case MISSION:
			reportToMaster("Starting cruise from MISSION to BASE through canal %d for %d seconds", localStates[myId].canal, period);
			localStates[myId].position = BASE;
			break;
	}
	sleepAndResponse(period);

	localStates[myId].section = LOCAL;
	localStates[myId].canal = -1;

	prepareToSendState();

	pvm_bcast(SLAVE_GROUP, REQUEST_TAG);
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
		switch (localStates[myId].section) {
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
