all:	k b_c b c
k:
		pkill --signal KILL main || true
b_c:
		g++ cleaner.cpp -ggdb3 -o cleaner 
b:
		g++ main.cpp -ggdb3 -o main
c:
		./ipc_cleaner.sh
