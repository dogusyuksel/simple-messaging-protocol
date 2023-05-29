include Config

CC = gcc
RM = rm -rf
CFLAGS += -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -g3 -O2 -fPIC
LDFLAGS = -lpthread

ifeq (${OPEN_DEBUG},y)
CFLAGS += -DOPEN_DEBUG
endif

ROOT_DIR="$(shell echo ${PWD})"
COMMON_INCDIR=${ROOT_DIR}/${COMMON_PATH}/include
JSON_C=${ROOT_DIR}/${JSON_C_DIRECTORY}
JSON_C_BUILD=${ROOT_DIR}/${JSON_C_BUILD_DIR}
JSON_C_DIR=${ROOT_DIR}/${JSON_C_DIRECTORY}/install

SUBDIRS = common \
	libsmp \
	manager \
	test

.ONESHELL: # Applies to every targets in the file!

.PHONY: all clean json-prep

all: clean json-prep
	echo "_-_-_-_- make starts _-_-_-_-"
	for dir in $(SUBDIRS); do \
		make -C $$dir all; \
	done
	echo "_-_-_-_- make ends _-_-_-_-"

clean:
	rm -rf $(JSON_C_BUILD)
	rm -rf $(JSON_C)
	rm -rf $(JSON_C_TEST_DIRECTORY)
	rm -rf $(JSON_C__CMAKE_FILES_DIRECTORY)
	rm -rf $(JSON_C_APPS)
	echo "_-_-_-_- clean starts _-_-_-_-"
	for dir in $(SUBDIRS); do \
		make -C $$dir clean; \
	done
	echo "_-_-_-_- clean ends _-_-_-_-"

json-prep:
	git clone $(JSON_GITHUB_LINK)
	mkdir $(JSON_C_BUILD)
	cd $(JSON_C_BUILD)
	cmake $(JSON_C) >/dev/null
	make >/dev/null
	cd $(ROOT_DIR)

.EXPORT_ALL_VARIABLES: # Export all variables defined in this doc to all sub-makefiles
