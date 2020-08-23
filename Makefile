# -----------------------------------------------
# OPTIONS
# -----------------------------------------------

DEBUG ?= 0
MARCH ?= native
MTUNE ?= native
# -----------------------------------------------
# SYSTEM DETECTION
# -----------------------------------------------

OS_TYPE := $(shell $(CC) -dumpmachine)
WINDRES ?= windres

ENTRYPOINT := _entryPoint
ifneq (,$(findstring i686-,$(OS_TYPE)))
	ENTRYPOINT := $(addprefix _,$(ENTRYPOINT))
endif

# -----------------------------------------------
# PATHS
# -----------------------------------------------

CLI_PATH := frontend
LIB_PATH := libreplace

OUT_PATH := bin/$(OS_TYPE)
OBJ_PATH := obj/$(OS_TYPE)
OUT_FILE := $(OUT_PATH)/replace.exe

SRCFILES := $(wildcard $(LIB_PATH)/src/*.c) $(wildcard $(CLI_PATH)/src/*.c)
RESFILES := $(wildcard frontend/*.rc)
OBJFILES := $(addprefix $(OBJ_PATH)/,$(patsubst %.rc,%.res.o,$(notdir $(RESFILES))))

# -----------------------------------------------
# FLAGS
# -----------------------------------------------

CFLAGS = -Wno-incompatible-pointer-types -I$(LIB_PATH)/include -e $(ENTRYPOINT) -nostdlib
LDFLAGS = -lkernel32 -luser32 -lshell32

ifeq ($(DEBUG),0)
  CFLAGS += -O3 -DNDEBUG
else
  CFLAGS += -g
endif

CFLAGS += -march=$(MARCH) -mtune=$(MTUNE)

# -----------------------------------------------
# MAKE RULES
# -----------------------------------------------

.PHONY: all mkdirs clean

all: mkdirs $(SRCFILES) $(OBJFILES)
	$(CC) $(CFLAGS) -o $(OUT_FILE) $(SRCFILES) $(OBJFILES) $(LDFLAGS)
ifeq ($(DEBUG),0)
	strip $(OUT_FILE)
endif

mkdirs:
	mkdir -p $(OUT_PATH)
	mkdir -p $(OBJ_PATH)

$(OBJ_PATH)/%.res.o: $(CLI_PATH)/%.rc
	$(WINDRES) -i $< -o $@

clean:
	rm -f $(OBJFILES) $(OUT_FILE)
