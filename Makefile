# ─────────────────────────────────────────────
#  Build Configuration
# ─────────────────────────────────────────────
CC      := gcc
TARGET  := flow

SRC_DIR   := src
DBG_DIR   := debug
REL_DIR   := release

HEADERS := $(wildcard $(SRC_DIR)/*.h)
SRCS    := $(wildcard  $(SRC_DIR)/*.c)
OBJS    := $(notdir $(SRCS:.c=.o))

# ─────────────────────────────────────────────
#  Flags
# ─────────────────────────────────────────────
COMMON_FLAGS := -Wall -Wextra -std=c23 $(shell pkg-config --cflags --libs sdl3 sdl3-image sdl3-ttf)

DBG_FLAGS := $(COMMON_FLAGS) -ggdb -O0 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -Wno-unused-parameter
REL_FLAGS := $(COMMON_FLAGS) -O3 -DNDEBUG -march=native

LIBS := $(shell pkg-config --libs sdl3 sdl3-image)

# ─────────────────────────────────────────────
#  Default: Debug
# ─────────────────────────────────────────────
.PHONY: all debug release clean

all: debug

debug: CFLAGS   := $(DBG_FLAGS)
debug: OUT_DIR  := $(DBG_DIR)
debug: $(DBG_DIR)/$(TARGET)

release: CFLAGS  := $(REL_FLAGS)
release: OUT_DIR := $(REL_DIR)
release: $(REL_DIR)/$(TARGET)

# ─────────────────────────────────────────────
#  Link
# ─────────────────────────────────────────────
$(DBG_DIR)/$(TARGET): $(addprefix $(DBG_DIR)/, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo "  → debug build: $@"

$(REL_DIR)/$(TARGET): $(addprefix $(REL_DIR)/, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo "  → release build: $@"

# ─────────────────────────────────────────────
#  Compile
# ─────────────────────────────────────────────
$(DBG_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(DBG_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(REL_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(REL_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ─────────────────────────────────────────────
#  Ensure output dirs exist
# ─────────────────────────────────────────────
$(DBG_DIR) $(REL_DIR):
	mkdir -p $@

# ─────────────────────────────────────────────
#  Clean
# ─────────────────────────────────────────────
clean:
	rm -rf $(DBG_DIR) $(REL_DIR)
	@echo "  cleaned."

# ─────────────────────────────────────────────
#  Run
# ─────────────────────────────────────────────
run: debug
	@echo "  → running debug build..."
	ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=$(CURDIR)/lsan.supp ./$(DBG_DIR)/$(TARGET)

run-release: release
	@echo "  → running release build..."
	./$(REL_DIR)/$(TARGET)

