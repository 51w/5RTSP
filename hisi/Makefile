﻿TARGET = main

OBJS_PATH = objs

CROSS_COMPILE = arm-hisiv300-linux-
CXX   = $(CROSS_COMPILE)g++
CC    = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

INC  += -I$(shell pwd)/include
LIB   = -L$(shell pwd)/lib

LD_FLAGS += ./lib/libmpi.a ./lib/libVoiceEngine.a ./lib/libupvqe.a ./lib/libdnvqe.a ./lib/libisp.a ./lib/libsns_ov4689.a 
LD_FLAGS += ./lib/lib_cmoscfg.a ./lib/lib_iniparser.a ./lib/lib_hiae.a ./lib/lib_hiawb.a ./lib/lib_hiaf.a ./lib/lib_hidefog.a
LD_FLAGS += ./lib/libtde.a
LD_FLAGS += -lm -ldl -lpthread 

#OMNIVISION_OV4689_MIPI_4M_30FPS
#OMNIVISION_OV4689_MIPI_1080P_30FPS
XX_FLAGS += -Dhi3516a -DHICHIP=0x3516A100 -DSENSOR_TYPE=OMNIVISION_OV4689_MIPI_1080P_30FPS
XX_FLAGS += -DISP_V2 -DHI_MIPI_ENABLE
XX_FLAGS += -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -ffunction-sections

O_FLAG   = -O2

SRC1  = $(notdir $(wildcard ./osd/*.c))
OBJS1 = $(patsubst %.c,$(OBJS_PATH)/%.o,$(SRC1))

all: BUILD_DIR $(TARGET)

BUILD_DIR:
	@-mkdir -p $(OBJS_PATH)

$(TARGET) : $(OBJS1)
	$(CC) $^ -o $@ $(CFLAGS) $(LIB) $(LD_FLAGS) $(CXX_FLAGS) $(XX_FLAGS)
 
$(OBJS_PATH)/%.o : ./osd/%.c
	$(CC) -c  $< -o  $@  $(CXX_FLAGS) $(INC) $(XX_FLAGS)


clean:
	-rm -rf $(OBJS_PATH) $(TARGET)
