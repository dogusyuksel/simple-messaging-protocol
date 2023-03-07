EXECUTABLE_NAME=sipcd

CFLAGS += -Wno-stringop-truncation

C_SRCS = \
daemon.c \
../common/sipc_common.o

OBJS += \
./daemon.o

.PHONY: all clean

all:
	$(CC) -o ./$(EXECUTABLE_NAME) $(C_SRCS) $(CFLAGS) $(LDFLAGS) -I$(COMMON_INCDIR)

clean:
	$(RM) $(OBJS) ./$(EXECUTABLE_NAME)
