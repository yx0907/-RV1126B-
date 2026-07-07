#
# Makefile
#
CC 		=/usr/bin/aarch64-linux-gnu-gcc
LVGL_DIR_NAME 	?= lvgl
LVGL_DIR 	?= .

WARNINGS		:= -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
					-fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits \
					-Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security \
					-Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body \
					-Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -std=gnu99
CFLAGS 			?= -O3 -g0 -I$(LVGL_DIR)/ $(WARNINGS)
CFLAGS			+= -I/usr/aarch64-linux-gnu/include
CFLAGS			+= -I/usr/include/
CFLAGS			+= -I/usr/include/libdrm/
LDFLAGS 		?= -lm -ldrm
BIN 			= main
BUILD_DIR 		= ./build
BUILD_OBJ_DIR 	= $(BUILD_DIR)/obj
BUILD_BIN_DIR 	= $(BUILD_DIR)/bin

CJSON_DIR = external/cjson

prefix 			?= /usr
bindir 			?= $(prefix)/bin

#Collect the files to compile
MAINSRC          = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk

CSRCS 			+=$(LVGL_DIR)/mouse_cursor_icon.c 
CSRCS += ./ui/ui_manager.c
CSRCS += ./ui/ui_main.c
CSRCS += ./ui/ui_common.c
CSRCS += ./noto_sans_font.c
CSRCS += ./core/state_machine.c
CSRCS += ./ui/ui_state.c
CSRCS += ./read.c
CSRCS += ./ui/ui_read.c
CSRCS += ./ui/ui_ocr_page.c
CSRCS += ./ocr/ocr_config.c
CSRCS += ./ocr/ocr_manager.c
CSRCS += ./ocr/ocr_storage.c
CSRCS += ./receive/ai_comm.c
CSRCS += ./$(CJSON_DIR)/cJSON.c
CSRCS += ./receive/ai_parser.c
CSRCS += ./ui/ui_history.c
CSRCS += ./ocr/ocr_history.c
CSRCS += ./receive/readpipe.c
CSRCS += ./receive/fifot.c

OBJEXT 			?= .o

AOBJS 			= $(ASRCS:.S=$(OBJEXT))
COBJS 			= $(CSRCS:.c=$(OBJEXT))

MAINOBJ 		= $(MAINSRC:.c=$(OBJEXT))

SRCS 			= $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS 			= $(AOBJS) $(COBJS) $(MAINOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

## MAINOBJ -> OBJFILES


all: default

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) -static $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC) -static $(CFLAGS) -c $< -o $@
	@echo "CC $<"


default: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CC) -static -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

clean: 
	rm -rf $(BUILD_DIR)

install:
	install -d $(DESTDIR)$(bindir)
	install $(BUILD_BIN_DIR)/$(BIN) $(DESTDIR)$(bindir)

uninstall:
	$(RM) -r $(addprefix $(DESTDIR)$(bindir)/,$(BIN))
