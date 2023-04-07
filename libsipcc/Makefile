CFLAGS += -Wno-varargs

C_SRCS = \
sipc_lib.c \
../common/sipc_common.o

CFLAGS += -I$(JSON_C_DIR)/include/json-c
CFLAGS += -I$(JSON_C_DIR)/include
LDFLAGS+= -L$(JSON_C_DIR)/lib -ljson-c

LIBSIPCC_INCDIR=-I ./include -I ../common/include

OBJS += \
./sipc_lib.o

.PHONY: all clean

all:
	$(CC) $(C_SRCS) $(CFLAGS) $(LDFLAGS) $(LIBSIPCC_INCDIR) -o ./$(SO_NAME) -shared

clean:
	$(RM) $(OBJS) ./$(SO_NAME)
