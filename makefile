CC = clang
DIR = build
# CFLAGS = -w -O0
CFLAGS_NOTWIN32 = -g -O0 -w -ldl -fsanitize=address -D_GNU_SOURCE -lasan
CFLAGS_WIN32 = -g -O0 -w -ldbghelp 


OBJECTS = ./examples/main.c

UNAME_S := $(shell uname -s 2>/dev/null)


EXECUTABLE = a.exe

# TODO
# gcc -fdump-tree-original printexample.c
# add later
make: $(DIR) $(OBJECTS)
	$(CC) -o $(DIR)/$(EXECUTABLE) $(OBJECTS) $(CFLAGS_WIN32)

$(DIR):
	mkdir -p $(DIR)

clean:
	rm -rf $(DIR)
run: make
	./$(DIR)/$(EXECUTABLE) 
debug: 
	gcc -o $(DIR)/$(EXECUTABLE) $(OBJECTS) -g $(CFLAGS_NOTWIN32)
	gdb --tui ./$(DIR)/$(EXECUTABLE)
profile: make
	@echo "perf report to view"
	@sleep 2
	perf record ./$(DIR)/$(EXECUTABLE)
