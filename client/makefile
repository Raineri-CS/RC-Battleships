C_FILES = $(wildcard include/*.c)
O_FILES = $(C_FILES:include/%.c=build/%.o)

.PHONY: all clean
.DEFAULT: all

all: client

client: build/main.o $(O_FILES) 
	gcc -o $@ $^

build:
	@mkdir -p build

build/main.o: build
	gcc -c main.c -o build/main.o

build/%.o: include/%.c | build
	gcc -c $< -o $@

clean:
	-rm -f $(O_FILES)
	-rm -f client
	-rm -rf build