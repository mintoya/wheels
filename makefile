# Compiler settings
CXX      := clang++
CC       := clang
EXE      := wheels.exe

# Files
# Use 'make FILE=yourfile.c' to override
FILE     ?= bigint.c
C_SRC    := $(FILE)

# -w: like 79 vla warnings
CFLAGS   := -g -w -fno-sanitize=vla-bound
CXXFLAGS := $(CFLAGS) -std=c++17
LDFLAGS  := -rdynamic

ifeq ($(OS),Windows_NT)
    LIBS := -ldbghelp
else
    LIBS := 
endif

# Build Rules
.PHONY: all run clean

all: $(EXE)

$(EXE): $(C_SRC)
	$(CXX) $(CXXFLAGS) $(C_SRC) -o $(EXE) $(LDFLAGS) $(LIBS)

run: all
	./$(EXE)

clean:
	rm -f $(EXE)
