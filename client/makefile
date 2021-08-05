release:
	cd include;\
	make clean;\
	make;\
	cd ..;\
	gcc main.c -I$(PWD)/include -L$(PWD)/include -o ./bin/battleship -Wall -Wextra -pedantic -Werror;\

debug:
	gcc main.c -I$(PWD)/include -L$(PWD)/include -g -o ./bin/battleship -Wall -Wextra -pedantic -Werror

clean:
	rm -R build
