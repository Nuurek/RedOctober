#include <stdarg.h>
#include <string.h>
#include "instance.h"
#include "common.h"


Instance instance;
int myId = -1;

void reportToMaster(const char* message, ...) {
	va_list argp;
	va_start(argp, message);
	char messageBuffer[REPORT_MESSAGE_SIZE];
	vsprintf(messageBuffer, message, argp);
	va_end(argp);

	char buffer[REPORT_MESSAGE_SIZE];
	sprintf(buffer, "[%d]: ", myId);

	strcat(buffer, messageBuffer);

	pvm_initsend(PvmDataDefault);
	pvm_pkstr(buffer);
	pvm_send(instance.masterTId, REPORT_TAG);
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
		}
	}

	reportToMaster("Initiated.");

	pvm_exit();
}
