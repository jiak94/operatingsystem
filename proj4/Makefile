all: bof bof2

bof: bof.c
	gcc -g -m64 bof.c -o bof -z execstack -fno-stack-protector

bof2: bof2.c
	gcc -g -m64 bof2.c -o bof2 -z execstack -fno-stack-protector

clean:
	rm bof bof2
