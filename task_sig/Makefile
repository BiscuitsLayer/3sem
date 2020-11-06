all:	c b r
c:
		pkill --signal KILL main || true
b:
		g++ main.cpp -ggdb3 -o main -ggdb3
r:
		./main input1.txt
d:
		diff input.txt output.txt