all:	clear writer reader
clear:
	rm -f channel.fifo && pkill --signal KILL writer && pkill --signal KILL reader || true
reader: reader.cpp
	g++ reader.cpp -Wall -ggdb3 -o reader
writer:	writer.cpp
	g++ writer.cpp -Wall -ggdb3 -o writer