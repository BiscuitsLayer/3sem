all:	c b
c:
		pkill --signal KILL main || true
b:
		g++ main.cpp -ggdb3 -o main -ggdb3
r:
		./main input.txt > output.txt
d:
		diff input.txt output.txt