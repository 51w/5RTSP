﻿TARGET = rtsp

OBJS_PATH = objs

CROSS_COMPILE =
CXX   = $(CROSS_COMPILE)g++
CC    = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

INC  += -I$(shell pwd)/live555/ubuntu16/include/BasicUsageEnvironment
INC  += -I$(shell pwd)/live555/ubuntu16/include/groupsock
INC  += -I$(shell pwd)/live555/ubuntu16/include/liveMedia
INC  += -I$(shell pwd)/live555/ubuntu16/include/UsageEnvironment
LIB   = -L$(shell pwd)/live555/ubuntu16/lib
LD_FLAGS  = -lpthread -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock

O_FLAG   = -O2

SRC1  = $(notdir $(wildcard ./src/*.cpp))
OBJS1 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC1))

all: BUILD_DIR $(TARGET)

BUILD_DIR:
	@-mkdir -p $(OBJS_PATH)

$(TARGET) : $(OBJS1)
	$(CXX) $^ -o $@ $(CFLAGS) $(LIB) $(LD_FLAGS) $(CXX_FLAGS)
 
$(OBJS_PATH)/%.o : ./src/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)


clean:
	-rm -rf $(OBJS_PATH) $(TARGET)
