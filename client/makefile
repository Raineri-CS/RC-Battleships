C_FILES = $(wildcard include/*.c)
O_FILES = $(C_FILES:include/%.c=build/%.o)

.PHONY: all clean
.DEFAULT: all

all: client

client: $(O_FILES)
	gcc main.c -o $@ $^

build:
	@mkdir -p build

build/%.o: include/%.c | build
	gcc -c $< -o $@

clean:
	-rm -f $(O_FILES)
	-rm -f client
	-rm -rf build