all:	clear kill_writer kill_reader writer reader
clear:
	rm -f *.fifo
kill_writer:
	pkill --signal KILL writer || true
kill_reader:
	pkill --signal KILL reader || true
reader: reader.cpp
	g++ reader.cpp -Wall -ggdb3 -o reader
writer:	writer.cpp
	g++ writer.cpp -Wall -ggdb3 -o writer