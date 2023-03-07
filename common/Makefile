COMMON_INCDIR=./include

C_SRCS = \
sipc_common.c

OBJS += \
./sipc_common.o

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(COMMON_INCDIR) -c $(C_SRCS)

clean:
	$(RM) $(OBJS)
