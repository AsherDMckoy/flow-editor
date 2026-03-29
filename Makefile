# ─────────────────────────────────────────────
#  Build Configuration
# ─────────────────────────────────────────────
CC      := gcc
TARGET  := flow

SRC_DIR   := src
DBG_DIR   := debug
REL_DIR   := release

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(notdir $(SRCS:.c=.o))

# ─────────────────────────────────────────────
#  Flags
# ─────────────────────────────────────────────
COMMON_FLAGS := -Wall -Wextra -std=c23

DBG_FLAGS := $(COMMON_FLAGS) -g -O0 -DDEBUG
REL_FLAGS := $(COMMON_FLAGS) -O2 -DNDEBUG -march=native

LIBS :=

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
$(DBG_DIR)/%.o: $(SRC_DIR)/%.c | $(DBG_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(REL_DIR)/%.o: $(SRC_DIR)/%.c | $(REL_DIR)
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
