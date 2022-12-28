
include Config

CC = gcc
RM = rm -rf
CFLAGS = -Wall -Wextra -Werror -g3 -O0
TARGET_EXECUTABLE_NAME=test

LIBS := -lpthread -lsipc

C_SRCS = \
main.c

OBJS += \
./main.o

.PHONY: all clean

all:
	$(CC) -o ./$(TARGET_EXECUTABLE_NAME) $(C_SRCS) $(CFLAGS) $(LIBS) -L$(LIBRARY_PATH)

clean:
	$(RM) $(OBJS) ./$(TARGET_EXECUTABLE_NAME)
