# -----------------------------------------------
# OPTIONS
# -----------------------------------------------

DEBUG ?= 0
MARCH ?= native
MTUNE ?= native

# -----------------------------------------------
# SYSTEM DETECTION
# -----------------------------------------------

OS_TYPE := $(shell $(CXX) -dumpmachine)
ENTRYPOINT := _entryPoint

ifeq (,$(findstring x86_64,$(OS_TYPE)))
	ENTRYPOINT := $(addprefix _,$(ENTRYPOINT))
endif

# -----------------------------------------------
# PATHS
# -----------------------------------------------

OUT_PATH := bin/$(OS_TYPE)
OUT_FILE := $(OUT_PATH)/replace.exe
RES_FILE := frontend/replace.rc
OBJ_PATH := obj/$(OS_TYPE)
OBJ_FILE := $(RCC_PATH)/replace.rc.o

SRCFILES := $(wildcard libreplace/src/*.c) $(wildcard frontend/src/*.c)

# -----------------------------------------------
# FLAGS
# -----------------------------------------------

CFLAGS = -Wno-incompatible-pointer-types -e $(ENTRYPOINT) -nostdlib
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

.PHONY: all clean

all:
	mkdir -p "$(OUT_PATH)" "$(OBJ_PATH)"
	windres -i "$(RES_FILE)" -o "$(OBJ_FILE)"
	$(CC) -I libreplace/include $(CFLAGS) -o "$(OUT_FILE)" $(SRCFILES) "$(OBJ_FILE)" $(LDFLAGS)
ifeq ($(DEBUG),0)
	strip "$(OUT_FILE)"
endif

clean:
	rm -f "$(RES_FILE)" "$(OUT_FILE)"
