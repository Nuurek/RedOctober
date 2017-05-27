#include <signal.h>
#include "time.h"
#include "instance.h"
#include "common.h"


Instance instance;
volatile bool keepRunning = true;

void interuptHandler(int dummy) {
	keepRunning = false;
}

void seedRandomEngine() {
	int seed;
	time_t now;
	seed = time(&now);
	srand(seed);
}

int getSlavesNumber(int argc, const char *args[]) {
	int slavesNumber = DEFAULT_SLAVES_NUMBER;
	if (argc > 1) {
		slavesNumber = atoi(args[1]);
		slavesNumber = (slavesNumber <= MAX_SLAVES_NUMBER ? slavesNumber : MAX_SLAVES_NUMBER);
	}

	printf("Slaves number: %d\n", slavesNumber);

	return slavesNumber;
}

int getCanalsNumber(int argc, const char *args[]) {
	int canalsNumber = DEFAULT_CANALS_NUMBER;
	if (argc > 2) {
		canalsNumber = atoi(args[2]);
		canalsNumber = (canalsNumber <= MAX_CANALS_NUMBER ? canalsNumber : MAX_CANALS_NUMBER);
	}

	printf("Canals number: %d\n", canalsNumber);

	return canalsNumber;
}

void setCanalSizes(int argc, const char *args[], Instance& instance) {
	int i = 0, canalSize;
	while (i < instance.canalsNumber) {
		// Canal capacity in program input
		if (i < argc - 3) {
			canalSize = atoi(args[i + 3]);
			// if input size is larger than max set it to max
			instance.canalSizes[i] = (canalSize <= MAX_CANAL_SIZE ? canalSize : MAX_CANAL_SIZE);
		// Otherwise, draw random size <1, MAX_CANAL_SIZE>
		} else {
			instance.canalSizes[i] = rand() % MAX_CANAL_SIZE + 1;
		}
		printf("Canal[%d]: %d\n", i, instance.canalSizes[i]);
		++i;
	}
}

void spawnSlaves(Instance& instance) {
	int numberOfSpawned = pvm_spawn(SLAVE_NAME, NULL, PvmTaskDefault, NULL, instance.slavesNumber, instance.slaveTIds);

	if (numberOfSpawned != instance.slavesNumber) {
		printf("Not enough slaves spawned! Only %d spawned\n", numberOfSpawned);
		printf("Before exit\n");
		pvm_exit();
		printf("After exit\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Spawned %d slaves\n", numberOfSpawned);
	}
}

void initializeSlaves(Instance& instance) {
	pvm_initsend(PvmDataDefault);
	packInstance(instance);
	pvm_mcast(instance.slaveTIds, instance.slavesNumber, INITIAL_DATA_TAG);
}

void collectReportMessage() {
	char buffer[REPORT_MESSAGE_SIZE];
	while(keepRunning) {
		pvm_recv(-1, REPORT_TAG);
		pvm_upkstr(buffer);
		printf("%s\n", buffer);
	}
}

void killSlavesAndExit() {
	int i;
	for (i = 0; i < instance.slavesNumber; i++) {
		pvm_kill(instance.slaveTIds[i]);
	}

	pvm_exit();
}

int main(int argc, const char *args[]) {
	signal(SIGINT, interuptHandler);
	seedRandomEngine();

	instance.slavesNumber = getSlavesNumber(argc, args);
	instance.canalsNumber = getCanalsNumber(argc, args);
	setCanalSizes(argc, args, instance);

	instance.masterTId = pvm_mytid();

	spawnSlaves(instance);
	initializeSlaves(instance);

	collectReportMessage();

	killSlavesAndExit();
}
