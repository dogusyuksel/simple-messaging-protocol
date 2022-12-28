include Config


CC = gcc
RM = rm -rf
CFLAGS = -Wall -Wextra -Werror -g3 -O0 -fPIC -DOPEN_DEBUG
TARGET_EXECUTABLE_NAME=sipcd
TARGET_LIBRARY_NAME=libsipc.so

ifeq (${OPEN_DEBUG},y)
CFLAGS += -DOPEN_DEBUG
endif

LIBS := -lpthread

C_SRCS = \
sipc_lib.c     \
sipcd.c

OBJS += \
./sipc_lib.o  \
./sipcd.o

.PHONY: all clean

all:
	$(CC) $(C_SRCS) $(CFLAGS) $(LIBS) -o ./$(TARGET_EXECUTABLE_NAME)
	$(CC) $(C_SRCS) $(CFLAGS) $(LIBS) -o ./$(TARGET_LIBRARY_NAME) -fPIC -shared
	@echo ' Finish all'

clean:
	$(RM) $(OBJS) ./$(TARGET_EXECUTABLE_NAME) ./$(TARGET_LIBRARY_NAME)
