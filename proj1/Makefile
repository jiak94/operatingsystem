all: webserver_part1 webserver_part2 htmlget

webserver_part1: webserver_part1.c
	gcc webserver_part1.c -g -o webserver_part1 -lpthread

webserver_part2: webserver_part2.c
	gcc webserver_part2.c -g -o webserver_part2 -lpthread

htmlget: htmlget.c
	gcc htmlget.c -g -o htmlget

clean:
	rm -f webserver_part1 webserver_part2 htmlget
