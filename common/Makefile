COMMON_INCDIR=./include

C_SRCS = \
smp_common.c

OBJS += \
./smp_common.o

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(COMMON_INCDIR) -c $(C_SRCS)

clean:
	$(RM) $(OBJS)
