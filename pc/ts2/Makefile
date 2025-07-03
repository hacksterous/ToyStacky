PROJECT_SOURCE_DIR := .
CC := gcc
DEFS := _UNIX
CFLAGS := -Os -Wall -std=c18 -fmax-errors=10 -Wno-unused-variable -Wno-unused-function -D$(DEFS)

TS2_SRC_FILES := \
     $(PROJECT_SOURCE_DIR)/ts2/ts2.c \
     $(PROJECT_SOURCE_DIR)/ts2/day.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-process.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-stack.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-numString.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-fnOrOpVec2Param.c \
     $(PROJECT_SOURCE_DIR)/ts2/yasML.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-llist.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-math.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-fnOrOp2Param.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-processPop.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-fn1Param.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-processDefaultPush.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-fnOrOpMat2Param.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-fnOrOpMat1Param.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-fnOrOpVec1Param.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-tokenize.c \
     $(PROJECT_SOURCE_DIR)/ts2/TS-core-ledger.c \
     $(PROJECT_SOURCE_DIR)/ts2/bigint.c \


TERM_SRC_FILES := \
		$(PROJECT_SOURCE_DIR)/microrl/microrl.c 

ifeq ($(DEFS),_UNIX)
TS2_SRC_FILES += $(PROJECT_SOURCE_DIR)/microrl/unix.c
endif

ifneq ($(DEFS),_UNIX)
TS2_SRC_FILES += $(PROJECT_SOURCE_DIR)/microrl/stm32.c
endif

SRC_FILES := $(TS2_SRC_FILES) $(TERM_SRC_FILES)
OBJ_FILES := $(SRC_FILES:.c=.o)

TARGET := ts2.exe

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES)

cleanall:
	rm -f $(OBJ_FILES) $(TARGET)

.PHONY: all clean



