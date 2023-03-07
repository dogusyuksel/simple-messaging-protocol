CFLAGS += -Wno-varargs

C_SRCS = \
sipc_lib.c \
../common/sipc_common.o

LIBSIPCC_INCDIR=-I ./include -I ../common/include

OBJS += \
./sipc_lib.o

.PHONY: all clean

all:
	$(CC) $(C_SRCS) $(CFLAGS) $(LDFLAGS) $(LIBSIPCC_INCDIR) -o ./$(SO_NAME) -shared

clean:
	$(RM) $(OBJS) ./$(SO_NAME)
