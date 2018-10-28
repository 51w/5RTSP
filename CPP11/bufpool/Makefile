##
CC = gcc
AR = ar

OBJS := bufpool.o
TARGET = bufpool

%.o:%.c
	$(CC) -I. -o $@ -c $^
	
$(TARGET):$(OBJS)
	$(AR) rc libbufpool.a $^
	
all:$(TARGET)

clean:
	rm *.a *.o


