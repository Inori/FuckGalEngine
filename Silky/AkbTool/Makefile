NAME = akbtool
INC_DIR += .
BUILD_DIR ?= ./build
OBJ_DIR ?= $(BUILD_DIR)/obj
BINARY ?= $(BUILD_DIR)/$(NAME)

.DEFAULT_GOAL = app

# Compilation flags
CC = g++
LD = g++
INCLUDES  = $(addprefix -I, $(INC_DIR))
CFLAGS   += -O2 -std=c++14 $(INCLUDES)

# Files to be compiled
SRCS = $(shell find . -name "*.cc")
OBJS = $(SRCS:./%.cc=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: ./%.cc
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<

# Depencies
-include $(OBJS:.o=.d)

# Some convinient rules

.PHONY: app run submit clean
app: $(BINARY)

# Command to execute NEMU
NEMU_EXEC := $(BINARY) $(ARGS)

$(BINARY): $(OBJS)
	@echo + LD $@
	@$(LD) -O2 -o $@ $^ -lpng

clean: 
	rm -rf $(BUILD_DIR)
