include Config

ifeq (${OPEN_DEBUG},y)
CFLAGS += -DOPEN_DEBUG
endif

CC = gcc
RM = rm -rf
CFLAGS = -Wall -Wextra -Werror -g3 -O2 -fPIC
LDFLAGS = -lpthread

COMMON_INCDIR="$(shell echo ${PWD}/${COMMON_PATH}/include)"
LIB_INCDIR="$(shell echo ${PWD}/${LIBSIPCC_PATH}/include)"
LIB_DIR="$(shell echo ${PWD}/${LIBSIPCC_PATH})"
SO_NAME=lib${LIBRARY_NAME}.so
LIB_NAME=${LIBRARY_NAME}

SUBDIRS = common \
	libsipcc \
	daemon \
	test

.PHONY: all clean

all:
	echo "_-_-_-_- make start _-_-_-_-"
	for dir in $(SUBDIRS); do \
		make -C $$dir all; \
	done
	echo "_-_-_-_- make end _-_-_-_-"

clean:
	echo "_-_-_-_- clean start _-_-_-_-"
	for dir in $(SUBDIRS); do \
		make -C $$dir clean; \
	done
	echo "_-_-_-_- clean end _-_-_-_-"

.EXPORT_ALL_VARIABLES:
