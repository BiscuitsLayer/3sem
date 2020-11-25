#include "GenLib.h"

int main (int argc, char **argv) {
	key_t key = ftok ("main", FTOK_ID);
    CHECK (key == -1);
	int semId = semget (key, SEM_ARRAY_SIZE, IPC_CREAT | IPC_EXCL | 0666);
	CHECK (semId == -1);

    //! SEM INIT
	for (int i = 0; i < SEM_ARRAY_SIZE; ++i) {
		semctl (semId, i, SETVAL, 0);
	}
	semctl (semId, SemType::empty, SETVAL, 1);
	//! SEM INIT

    fprintf (stdout, "Initialized:\n");
    for (int i = 0; i < SEM_ARRAY_SIZE; ++i)
		fprintf (stdout, "sem %d == %d\n", i, GetValue (semId, (SemType)i));

	while (true) {
		fprintf (stdout, "Enter when finished:\n");
		char temp = 0;
		scanf ("%c", &temp);
		if (temp != 'q') {
			fprintf (stdout, "Current  state:\n");
    		for (int i = 0; i < SEM_ARRAY_SIZE; ++i)
				fprintf (stdout, "sem %d == %d\n", i, GetValue (semId, (SemType)i));
		}
		else 
			break;
	}

	fprintf (stdout, "After work:\n");
    for (int i = 0; i < SEM_ARRAY_SIZE; ++i)
		fprintf (stdout, "sem %d == %d\n", i, GetValue (semId, (SemType)i));

	semctl (semId, 0, IPC_RMID);

	exit (EXIT_SUCCESS);
}
