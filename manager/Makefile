EXECUTABLE_NAME=smp_manager

CFLAGS += -Wno-stringop-truncation

CFLAGS += -I$(JSON_C_DIR)/include/json-c
CFLAGS += -I$(JSON_C_DIR)/include
LDFLAGS+= -L$(JSON_C_DIR)/lib -ljson-c

C_SRCS = \
smp_manager.c \
../common/smp_common.o

OBJS += \
./smp_manager.o

.PHONY: all clean

all:
	$(CC) -o ./$(EXECUTABLE_NAME) $(C_SRCS) $(CFLAGS) $(LDFLAGS) -I$(COMMON_INCDIR)

clean:
	$(RM) $(OBJS) ./$(EXECUTABLE_NAME)
