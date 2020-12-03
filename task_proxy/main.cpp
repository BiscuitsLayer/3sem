#include "GenLib.hpp"

extern const int MAX_CHILDREN_COUNT;
extern const int MAX_MSG_SIZE;

int main (int argc, char **argv) {
	unsigned n = 0; // количество детей
	int myId = -1;	// в случае ребёнка = порядковый номер, в случае родителя = -1

	if (argc != 3) {
		fprintf  (stderr, "Usage: ./main [number of children] [filename]\n");
		exit (EXIT_FAILURE);
	}

	char *endptr = nullptr;
	n = strtoll (argv[1], &endptr, 10);

	if ((errno == ERANGE && (n == LLONG_MAX || n == LLONG_MIN)) || (errno != 0 && n == 0) \
		|| n <= 0 || n > MAX_CHILDREN_COUNT) {
        fprintf (stderr, "Wrong argument\n");
        exit (EXIT_FAILURE);
    }
	
	ConnectionData* connections = (ConnectionData*) calloc (n, sizeof (ConnectionData));

	int fileFd = POISON;

	// INIT
	for (unsigned i = 0; i < n; ++i) { 
		pipe (connections[i].C2PPipeFds);
		pipe (connections[i].P2CPipeFds);
		connections[i].parentPid = getpid (); // пока мы находимся в родителе

		pid_t childPid = fork ();
		if (childPid != 0) { 	//	PARENT
			ParentInit (connections, i, n, childPid);
		}
		else {					//	CHILD
			myId = i;
			ChildInit (connections, i, n, &fileFd, argv[2]);
			break;
		}
	}

	// WORK
	if (myId == -1) {	//	PARENT
		ParentWork (connections, n);
	}
	else { 				// CHILD
		ChildWork (connections, myId, fileFd);
	}

	return 0;
}