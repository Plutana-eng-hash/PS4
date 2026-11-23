TARGET            := ps4_overlay_plugin
OO_PS4_TOOLCHAIN  ?= $(HOME)/OpenOrbis/PS4Toolchain

CC      := clang
LD      := ld.lld
PRX_GEN ?= $(OO_PS4_TOOLCHAIN)/bin/linux/create-fself

SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
DIST_DIR  := dist

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
ELF  := $(BUILD_DIR)/$(TARGET).elf
SPRX := $(BUILD_DIR)/$(TARGET).sprx

CFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -fvisibility=hidden \
          -fno-stack-protector -fno-builtin -funwind-tables -fuse-init-array -O2 -Wall -Wextra -std=c11 \
          -I$(INC_DIR) -I$(OO_PS4_TOOLCHAIN)/include

LIBS := -L$(OO_PS4_TOOLCHAIN)/lib -lc -lkernel
LINK_SCRIPT := $(OO_PS4_TOOLCHAIN)/link.x
CRTLIB := $(wildcard $(OO_PS4_TOOLCHAIN)/lib/crtlib.o)

.PHONY: all clean package check-toolchain

check-toolchain:
	@test -d "$(OO_PS4_TOOLCHAIN)/include" || { echo "OO_PS4_TOOLCHAIN is not a valid toolchain: $(OO_PS4_TOOLCHAIN)"; exit 1; }

all: check-toolchain $(SPRX)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJS)
	@test -f "$(LINK_SCRIPT)" || { echo "Linker script not found: $(LINK_SCRIPT)"; exit 1; }
	$(LD) -m elf_x86_64 -pie --script $(LINK_SCRIPT) --eh-frame-hdr -e module_start -o $@ $(OBJS) $(CRTLIB) $(LIBS)
	@test -n "$(CRTLIB)" || { echo "WARNING: $(OO_PS4_TOOLCHAIN)/lib/crtlib.o not found; PRX will lack OpenOrbis module startup/segments"; }
	@echo "--- linked output ---"; head -c 16 $@ | od -An -tx1; file $@ || true; readelf -lW $@ || true

OELF := $(BUILD_DIR)/$(TARGET).oelf
PRX  := $(BUILD_DIR)/$(TARGET).prx

$(SPRX): $(ELF)
	@test -x "$(PRX_GEN)" || { echo "SPRX converter not found: $(PRX_GEN)"; exit 1; }
	$(PRX_GEN) -in $< -out $(OELF) -lib $(PRX) -libname $(TARGET) -paid 0x3800000000000011 -ptype fake -library-path $(OO_PS4_TOOLCHAIN)/lib
	@test -s $(PRX) || { echo "create-fself produced no $(PRX)"; exit 1; }
	cp $(PRX) $@

package: all
	rm -rf $(DIST_DIR)/pkg
	mkdir -p $(DIST_DIR)/pkg
	cp $(SPRX) $(DIST_DIR)/pkg/
	cp config/*.ini $(DIST_DIR)/pkg/
	cp install/INSTALL.txt README.md $(DIST_DIR)/pkg/
	cd $(DIST_DIR)/pkg && zip -r ../$(TARGET).zip .
	@echo "Packaged $(DIST_DIR)/$(TARGET).zip"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
