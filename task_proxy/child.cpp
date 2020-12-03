#include <cstdlib>

#include "GenLib.hpp"

void ChildInit (ConnectionData* connections, int Id, int n, int* fileFd, char* filepath) {
	if (Id == 0) {	//* Самый первый ребенок читает из файла
		*fileFd = open (filepath, O_RDONLY);
		if (*fileFd < 0) {
			fprintf (stderr, "Error opening file \n");
			ClearBuffers (connections, n);
    		exit (EXIT_FAILURE);
		}
	}
    else {
		CLOSE (connections[Id - 1].P2CPipeFds[FD::WRITE]); // т.к. этот pipe относится к предыдущему соединению
    }

	CLOSE (connections[Id].C2PPipeFds[FD::READ]);

	// эти два файловых дескриптора просто не связаны с ребёнком
	CLOSE (connections[Id].P2CPipeFds[FD::READ]);
	CLOSE (connections[Id].P2CPipeFds[FD::WRITE]);

	for (unsigned j = 0; j < Id; ++j) {
		//* Так как после fork получаем полную копию процесса - родителя, нужно закрыть лишние файловые дескрипторы
		CLOSE (connections[j].P2CPipeFds[FD::WRITE]);
		CLOSE (connections[j].C2PPipeFds[FD::READ]);
	}

	prctl (PR_SET_PDEATHSIG, SIGKILL);
    if (getppid () != connections[Id].parentPid) {
        fprintf (stderr, "Parent died: old ppid %d, new ppid %d\n", connections[Id].parentPid, getppid ());
        exit (EXIT_FAILURE);
    }
}

void ChildWork (ConnectionData* connections, int Id, int fileFd) {
    int retVal = 0;

	while (true) {

#ifdef DEBUG
ChildDebug (connections, Id, fileFd);
#endif

		if (Id == 0) {
			retVal = splice (fileFd,  NULL, \
							 connections[Id].C2PPipeFds[FD::WRITE], NULL, MAX_MSG_SIZE, SPLICE_F_MOVE);
		}
		else {
        	retVal = splice (connections[Id - 1].P2CPipeFds[FD::READ],  NULL, \
							 connections[Id].C2PPipeFds[FD::WRITE], NULL, MAX_MSG_SIZE, SPLICE_F_MOVE);
		}

		if (retVal == 0) {

#ifdef CHILDDEBUG
fprintf (stderr, "Child %d leaving and closing fds %d (write) and %d (read)\n", Id, connections[Id].C2PPipeFds[FD::WRITE], \
(Id == 0 ? fileFd : connections[Id - 1].P2CPipeFds[FD::READ]));
#endif

            CLOSE (connections[Id].C2PPipeFds[FD::WRITE]);
			if (Id == 0) {
				CLOSE (fileFd);
			}
			else {				
            	CLOSE (connections[Id - 1].P2CPipeFds[FD::READ]);
			}

#ifdef DEBUG
ChildDebug (connections, Id, fileFd);
#endif

            exit (EXIT_SUCCESS);
        }

        if (retVal < 0) {
            fprintf (stderr, "Splice failed in child #%d\n", Id);

#ifdef DEBUG
ChildDebug (connections, Id, fileFd);
#endif

			perror ("Splice");
			exit (EXIT_FAILURE);
        }
    }
}