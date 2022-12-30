
include Config

CC = gcc
RM = rm -rf
CFLAGS = -Wall -Wextra -Werror -g3 -O0
TARGET_EXECUTABLE_NAME_A=app_A

LIBS := -lpthread -lsipc

C_SRCS_A = \
application_A.c

OBJS_A += \
./application_A.o

.PHONY: all clean

all:
	$(CC) -o ./$(TARGET_EXECUTABLE_NAME_A) $(C_SRCS_A) $(CFLAGS) $(LIBS) -L$(LIBRARY_PATH) -I$(INCLUDE_PATH)

clean:
	$(RM) $(OBJS_A) ./$(TARGET_EXECUTABLE_NAME_A)
