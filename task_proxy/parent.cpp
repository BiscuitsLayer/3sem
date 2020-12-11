#include "GenLib.hpp"

void ParentInit (ConnectionData* connections, int Id, int n, int childPid) {
	CLOSE (connections[Id].C2PPipeFds[FD::WRITE]);	
	if (Id == n - 1) { // в последнем ребёнке будем сразу писать в stdout
		CLOSE (connections[Id].P2CPipeFds[FD::READ]);
		CLOSE (connections[Id].P2CPipeFds[FD::WRITE]);
		connections[Id].P2CPipeFds[FD::WRITE] = STDOUT_FILENO;
	}
	/* Не закрываем его здесь, так как этот файловый дескриптор должен быть открыт в следующем ребёнке
	else {
		CLOSE (connections[Id].P2CPipeFds[FD::READ]);
	}
	Закроем его при вызове ParentWork, когда все дети уже будут созданы */ 
	connections[Id].childPid = childPid;
}

void ParentWork (ConnectionData* connections, int n) {
	// Закрываем нужные файловые дескрипторы (см инициализацию), кроме последнего
	for (unsigned i = 0; i < n - 1; ++i) {
		CLOSE (connections[i].P2CPipeFds[FD::READ]);
	}
    for (unsigned i = 0; i < n; ++i) {
			ConnectionData *cur = &connections[i];
			cur->buf.size = pow (3, n - i) * 1024;
			cur->buf.startPtr = (char *) calloc (cur->buf.size, sizeof (char));

			if (!cur->buf.startPtr) {
				fprintf (stderr, "Error allocating buf\n");
				ClearBuffers (connections, n);
				exit (EXIT_FAILURE);
			}

			fcntl (cur->C2PPipeFds[FD::READ],  F_SETFL, O_NONBLOCK);
			fcntl (cur->P2CPipeFds[FD::WRITE], F_SETFL, O_NONBLOCK);
		}
		
	fd_set readFds {}, writeFds {};
	int deadChildren = 0;
	
	while (deadChildren != n) {
		FD_ZERO (&readFds);
		FD_ZERO (&writeFds);
		int maxFd = 0;
		
		for (unsigned i = deadChildren; i < n; ++i) {
			if (!isFull (&connections[i].buf)) {
				int readFd = connections[i].C2PPipeFds[FD::READ];
				FD_SET (readFd, &readFds);
				maxFd = (readFd > maxFd ? readFd : maxFd);
			}
			
			if (!isEmpty (&connections[i].buf)) {
				int writeFd = connections[i].P2CPipeFds[FD::WRITE];
				FD_SET (writeFd, &writeFds);
				maxFd = (writeFd > maxFd ? writeFd : maxFd);
			}
		}

#ifdef DEBUG
ParentDebug (connections, n, deadChildren);
#endif

		int retVal = select (maxFd + 1, &readFds, &writeFds, nullptr, nullptr);
		if (retVal < 0) {
			fprintf (stderr, "Select error\n");
			if (errno == EINTR) {
				fprintf (stderr, "Signal interrupt caught\n");
				continue;
			}
			ClearBuffers (connections, n);
			exit (EXIT_FAILURE);
		}

		if (retVal == 0) {
			continue;
		}

		for (int i = deadChildren; i < n; ++i) {
			if (FD_ISSET (connections[i].C2PPipeFds[FD::READ], &readFds)) { 
            	int retVal = ReadToBuf (connections, i, n);

				if (retVal == 0) {

#ifdef CHILDDEBUG					
fprintf (stderr, "Parent finished connection with child %d\n", i);
#endif

            	    CLOSE (connections[i].C2PPipeFds[FD::READ]);
					connections[i].isFinished = true;
            	}
			}
        
        	if (FD_ISSET (connections[i].P2CPipeFds[FD::WRITE], &writeFds)) {
        	    int retVal = WriteFromBuf (connections, i, n);
        	}
			
        	if (isEmpty (&connections[i].buf) && connections[i].isFinished) {
				waitpid (connections[i].childPid, nullptr, 0);
        	    if (i != deadChildren++) {
        	        fprintf (stderr, "Wrong child death sequence\n");
        	        ClearBuffers (connections, n);
        	        exit (EXIT_FAILURE);
        	    }

#ifdef CHILDDEBUG				
fprintf (stderr, "Now deadChildren == %d\n", deadChildren);
#endif

				CLOSE (connections[i].P2CPipeFds[FD::WRITE]);
        	}
		}
    }
}