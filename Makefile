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
          -fno-stack-protector -O2 -Wall -Wextra -std=c11 \
          -I$(INC_DIR) -I$(OO_PS4_TOOLCHAIN)/include

LIBS := -L$(OO_PS4_TOOLCHAIN)/lib -lkernel -lSceLibcInternal
LINK_SCRIPT := $(OO_PS4_TOOLCHAIN)/link.x

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
	$(LD) -m elf_x86_64 -shared -Bsymbolic -T $(LINK_SCRIPT) -e module_start -o $@ $(OBJS) $(LIBS)

$(SPRX): $(ELF)
	@test -x "$(PRX_GEN)" || { echo "SPRX converter not found: $(PRX_GEN)"; echo "Contents of $(OO_PS4_TOOLCHAIN)/bin/linux:"; ls -la "$(OO_PS4_TOOLCHAIN)/bin/linux" 2>/dev/null; echo "Contents of $(OO_PS4_TOOLCHAIN)/scripts:"; ls -la "$(OO_PS4_TOOLCHAIN)/scripts" 2>/dev/null; echo "Set PRX_GEN to the converter path to continue."; exit 1; }
	$(PRX_GEN) -in $< -out $@ -lib $(BUILD_DIR)/$(TARGET).stub.prx -libname $(TARGET) -ptype fake || { echo "create-fself failed. Usage output follows:"; "$(PRX_GEN)" --help; exit 1; }
	@test -s $@ || { echo "create-fself reported success but $@ is empty"; exit 1; }

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
