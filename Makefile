TARGET = waitpid
BUILDDIR := build

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS := 

SRCS = $(wildcard *.c)
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.c=.o))

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
