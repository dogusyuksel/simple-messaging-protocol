CFLAGS += -Wno-varargs

C_SRCS = \
libsmp.c \
../common/smp_common.o

SO_NAME=lib${LIBRARY_NAME}.so

CFLAGS += -I$(JSON_C_DIR)/include/json-c
CFLAGS += -I$(JSON_C_DIR)/include
LDFLAGS+= -L$(JSON_C_DIR)/lib -ljson-c

LIBSMP_INCDIR=-I ./include -I ../common/include

OBJS += \
./libsmp.o

.PHONY: all clean

all:
	$(CC) $(C_SRCS) $(CFLAGS) $(LDFLAGS) $(LIBSMP_INCDIR) -o ./$(SO_NAME) -shared  -Wl,-rpath=$(JSON_C_BUILD)

clean:
	$(RM) $(OBJS) ./$(SO_NAME)
