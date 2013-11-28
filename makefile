DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)


#rpmd variables
OBJS = rpmd.o
TARGET_1 = rpmd
LIBS_1 = `mysql_config --libs`
COMP_1 = g++ -std=c++0x
CFLAGS_1 = -I. -Wall -c $(DEBUG) `mysql_config --cflags`


.PHONY: all
.DEFAULT: all

all : $(TARGET_1)

$(TARGET_1) : $(OBJS)
	$(COMP_1) -o $(TARGET_1) $(LFLAGS) $(OBJS) $(LIBS_1)

rpmd.o : Rpmd.hpp rpmd.cpp
	$(COMP_1) $(CFLAGS_1) rpmd.cpp
