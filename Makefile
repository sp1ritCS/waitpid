#
# waitpid - waits for a foreign process to exit.
# Copyright (c) 2023 Florian "sp1rit" <sp1rit@national.shitposting.agency>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

TARGET = waitpid
BUILDDIR := build

VERSION := 1.0

CC = gcc
CFLAGS = -Wall -Wextra -O2 '-DWAITPID_VERSION="$(VERSION)"'
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
