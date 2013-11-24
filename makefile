DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)


#test_sql connection variables
OBJS = test_sql.o
LIBS_1 = `mysql_config --libs`
COMP_1 = g++
CFLAGS_1 = -Wall -c $(DEBUG) `mysql_config --cflags`
TARGET_1 = test_sql

.PHONY: all
.DEFAULT: all

all : $(TARGET_1)

$(TARGET_1) : $(OBJS)
	$(COMP_1) -o $(TARGET_1) $(LFLAGS) $(OBJS) $(LIBS_1)

test_sql.o : test_sql.cpp
	$(COMP_1) $(CFLAGS_1) test_sql.cpp
