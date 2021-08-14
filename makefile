# Criar o cliente e o servidor e copia-los pra pasta root desse make com os nomes cliente e servidor, respectivamente

.PHONY: all clean
.DEFAULT: all

all: cliente | servidor

cliente:
	make -C client clean 
	make -C client
	mv client/client cliente

servidor:
	make -C server clean
	make -C server
	mv server/server servidor

clean:
	rm -f cliente
	rm -f servidor