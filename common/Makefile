C_SRCS = \
smp_common.c

OBJS += \
./smp_common.o

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -I./include -c $(C_SRCS)

clean:
	$(RM) $(OBJS)
