all:	build link test
build:
		g++ main.cpp -ggdb3 -c
		g++ parent.cpp -ggdb3 -c
		g++ child.cpp -ggdb3 -c
		g++ GenLib.cpp -ggdb3 -c
link:
		g++ main.o parent.o child.o GenLib.o -o main
run:
		./main
test:
		./main 14 test0.txt
big_test:
		./main 1 test1.txt > test2.txt
		diff test1.txt test2.txt
main_test:
		./main 1 /bin/bash > xx
		diff /bin/bash xx
