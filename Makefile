CC := gcc
CFLAGS := -O3 -pedantic-errors -std=c18
CFLAGS_DEBUG := -Og -g -fsanitize=address -std=c18 -pedantic-errors -Wall
DEPS := main huffman bitHandler
BUILDDIR := build/
BINDIR := /usr/local/bin/
TARGET := huffman
TARGET_DEBUG := $(TARGET)
# TARGET_DEBUG := huffman_debug

OBJS := $(addprefix $(BUILDDIR), $(addsuffix .o, $(DEPS)))
OBJS_DEBUG := $(addprefix $(BUILDDIR)DEBUG_, $(addsuffix .o, $(DEPS)))
SOURCE := $(addsuffix .c, $(DEPS))

default: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Debug is noticeably slower
debug: $(OBJS_DEBUG)
	$(CC) $(CFLAGS_DEBUG) $(OBJS_DEBUG) -o $(TARGET_DEBUG)

clean:
	rm -rf $(BUILDDIR) $(TARGET_DEBUG)

# Most likely needs admin rights
install: default
	install -m 755 $(TARGET) $(BINDIR)$(TARGET)

uninstall:
	rm -f $(BINDIR)$(TARGET)

$(BUILDDIR)%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)DEBUG_%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@


.PHONY: default debug clean install