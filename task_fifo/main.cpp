#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cmath>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sys/wait.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <errno.h>
#include <limits.h>
#include <cstring>
#include <poll.h>

#define EXIT exit (EXIT_FAILURE);

const int MAX_NAME_SIZE = 100;
const int MAX_MSG_SIZE = 100;
const int MAX_WAIT_TIME_SEC = 5;

int OpenFifo (const char* fifoName, mode_t mode) {
	if (mkfifo (fifoName, 0666) < 0) {
		if (errno != EEXIST) {
			fprintf (stderr, "Error creating fifo\n");
			exit (EXIT_FAILURE);
		}
		errno = 0;
	}
	return open (fifoName, mode);
}

int main (int argc, char** argv) {
	int retVal = -1;
	if (argc > 2) {
		fprintf  (stderr, "Usage:\n\tWriter: ./main [filename]\n\tReader: .main\n");
		exit (EXIT_FAILURE);
	}

	if (argc == 2) {	// WRITER
		pid_t readerPid = -1;
		int pidFifo = OpenFifo ("pidFifo.fifo", O_RDONLY);
		if (pidFifo < 0) {
			fprintf (stderr, "Error opening pidFifo\n");
			exit (EXIT_FAILURE);
		}
		
		//	CriticalStart (3) - writer-ы борятся между собой за pid reader-a
		// 	CriticalStart (4) - парный код
		retVal = read (pidFifo, &readerPid, sizeof (readerPid));
		//	CriticalEnd (3)

		if (retVal < 0) {
			fprintf (stderr, "Reading from pidFifo error\n");
			exit (EXIT_FAILURE);
		} 
		else if (retVal == 0) {
			fprintf (stderr, "Reader missing\n");
			exit (EXIT_FAILURE);
		}

		char dataFifoName [MAX_NAME_SIZE];
		sprintf (dataFifoName, "dataFifo_%d.fifo", readerPid);
		int dataFifo = OpenFifo (dataFifoName, O_WRONLY | O_NONBLOCK);
		//	CriticalEnd (4)

		if (dataFifo < 0) {
			fprintf (stderr, "Error opening dataFifo\n");
			exit (EXIT_FAILURE);
		}

		retVal = fcntl (dataFifo, F_SETFL, O_WRONLY);

		if (retVal < 0) {
			fprintf (stderr, "Fcntl error\n");
			exit (EXIT_FAILURE);
		}
		
		int fileFd = open (argv[1], O_RDONLY);
		if (fileFd < 0) {
			fprintf (stderr, "Error opening file\n");
			exit (EXIT_FAILURE);
		}
	
		while (true) {
			retVal = splice (fileFd, NULL, dataFifo, NULL, MAX_MSG_SIZE, SPLICE_F_MOVE);
			if (retVal == 0) {
				break;
			}
			if (retVal < 0) {
				fprintf (stderr, "Splice error\n");
				exit (EXIT_FAILURE);
			}
		}

		close (dataFifo);
		close (pidFifo);
		exit (EXIT_SUCCESS);
	}
	else {				// READER
		int pidFifo = OpenFifo ("pidFifo.fifo", O_WRONLY);
		if (pidFifo < 0) {
			fprintf (stderr, "Error opening pidFifo\n");
			exit (EXIT_FAILURE);
		}

		pid_t readerPid = getpid ();

		char dataFifoName [MAX_NAME_SIZE];
		sprintf (dataFifoName, "dataFifo_%d.fifo", readerPid);
		int dataFifo = OpenFifo (dataFifoName, O_RDONLY | O_NONBLOCK);

		if (dataFifo < 0) {
			fprintf (stderr, "Error opening dataFifo\n");
			exit (EXIT_FAILURE);
		}

		//	CriticalStart (1) - reader-ы борятся между собой за возможность отправить pid
		//	CriticalStart (2) - reader с writer за fd на
		retVal = write (pidFifo, &readerPid, sizeof (readerPid));
		//	CriticalEnd (1)
		
		if (retVal < 0) {
			fprintf (stderr, "Writing to pidFifo error\n");
			exit (EXIT_FAILURE);
		} 
		

		fd_set dataFifoSet {};
		FD_ZERO (&dataFifoSet);
		FD_SET (dataFifo, &dataFifoSet);

		timeval timeVal {};
		timeVal.tv_sec = MAX_WAIT_TIME_SEC;
		retVal = select (dataFifo + 1, &dataFifoSet, NULL, NULL, &timeVal);
		//	CriticalEnd (2)
		
		if (retVal <= 0) {
			fprintf (stderr, "Can't read data\n");
			exit (EXIT_FAILURE);
		}


		retVal = fcntl (dataFifo, F_SETFL, O_RDONLY);

		if (retVal < 0) {
			fprintf (stderr, "Fcntl error %d\n", errno);
			exit (EXIT_FAILURE);
		}

		if (isatty (STDOUT_FILENO)) {
			int stdoutFlags = fcntl (STDOUT_FILENO, F_GETFL) ;
			stdoutFlags &= ~O_APPEND;
			retVal = fcntl (STDOUT_FILENO, F_SETFL, stdoutFlags);

			if (retVal < 0) {
				fprintf (stderr, "Fcntl error\n");
				exit (EXIT_FAILURE);
			}
		}

		while (true) {
			retVal = splice (dataFifo, NULL, STDOUT_FILENO, NULL, MAX_MSG_SIZE, SPLICE_F_MOVE);
			if (retVal == 0) {
				break;
			}

			if (retVal < 0) {
				fprintf (stderr, "Splice error\n");
				perror ("Splice");
				unlink (dataFifoName);
				exit (EXIT_FAILURE);
			}
		}

		close (pidFifo);
		close (dataFifo);
		unlink (dataFifoName);
		exit (EXIT_SUCCESS);

	}
	return 0;
}