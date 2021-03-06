PVMINC=$(PVM_ROOT)/include
PVMLIB=$(PVM_ROOT)/lib/$(PVM_ARCH)

all:	$(PVM_HOME)/master $(PVM_HOME)/slave

run:
	$(PVM_HOME)/master

$(PVM_HOME)/master:	master.c def.h instance.h common.h state.h
	c++ -g master.c -Wno-write-strings -o $(PVM_HOME)/master -L$(PVMLIB) -I$(PVMINC) -lpvm3 -lgpvm3

$(PVM_HOME)/slave:	slave.c def.h instance.h common.h state.h
	c++ -g slave.c -Wno-write-strings -o $(PVM_HOME)/slave -L$(PVMLIB) -lcrypt -I$(PVMINC) -lpvm3 -lgpvm3

clean:
	rm $(PVM_HOME)/master $(PVM_HOME)/slave
	rm -f *.o
