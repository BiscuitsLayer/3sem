#include "GenLib.hpp"

void ClearBuffers (ConnectionData* connections, int n) {
	for (unsigned i = 0; i < n; ++i) {
		free (connections[i].buf.startPtr);
	}
}

bool isFull (CircleBuf* buf) {
	return buf->isFull;
}

bool isEmpty (CircleBuf* buf) {
	return !buf->isFull && (buf->readShift == buf->writeShift);
}

int ReadToBuf (ConnectionData* connections, int Id, int n) {
	CircleBuf* cur = &connections[Id].buf;

	if (isFull (cur)) {
		fprintf (stderr, "Attempt to read in a full buf in connection %d\n", Id);
		ClearBuffers (connections, n);
		exit (EXIT_FAILURE);
	}

	int retVal = -1;
	
	if (cur->writeShift > cur->readShift) {
		retVal = read (connections[Id].C2PPipeFds [FD::READ], cur->startPtr + cur->readShift, cur->writeShift - cur->readShift);
	}
	else {
		retVal = read (connections[Id].C2PPipeFds [FD::READ], cur->startPtr + cur->readShift, cur->size - cur->readShift);
	}

	if (retVal < 0) {
		fprintf (stderr, "Error reading to buf in connection %d\n", Id);
		ClearBuffers (connections, n);
		exit (EXIT_FAILURE);
	}

	cur->readShift += retVal;
    cur->readShift %= cur->size;

	if (retVal != 0) {
		if (cur->readShift == cur->writeShift)
			cur->isFull = true;

#ifdef RWDEBUG
fprintf (stderr, "Read to buf in connection %d\n", Id);
fprintf (stderr, "readShift: %d, writeShift: %d of size %d\n", cur->readShift, cur->writeShift, cur->size);
BufPrint (cur);
#endif

	}

	return retVal;
}

int WriteFromBuf (ConnectionData* connections, int Id, int n) {
	CircleBuf* cur = &connections[Id].buf;
	int retVal = -1;

	if (isEmpty (cur)) {
		fprintf (stderr, "Attempt to write from an empty buf in connection %d\n", Id);
		ClearBuffers (connections, n);
		exit (EXIT_FAILURE);
	}
	
    if (cur->writeShift >= cur->readShift) {
		if ((cur->writeShift == cur->readShift && isFull (cur)) || cur->writeShift != cur->readShift) {
	    	retVal = write (connections[Id].P2CPipeFds [FD::WRITE], cur->startPtr + cur->writeShift, cur->size - cur->writeShift);
		}
	}
    else {
        retVal = write (connections[Id].P2CPipeFds [FD::WRITE], cur->startPtr + cur->writeShift, cur->readShift - cur->writeShift);
    }
	
	if (retVal < 0 && errno != EAGAIN) {
		fprintf (stderr, "Error writing from buf in connection %d\n", Id);
		ClearBuffers (connections, n);
		exit (EXIT_FAILURE);
	}

	cur->writeShift += retVal;
    cur->writeShift %= cur->size;

	if (retVal != 0) {
		//* Что-то прочитали, значит буфер уже не полный
		cur->isFull = false;

#ifdef RWDEBUG
fprintf (stderr, "Write from buf in connection %d\n", Id);
fprintf (stderr, "readShift: %d, writeShift: %d of size %d\n", cur->readShift, cur->writeShift, cur->size);
BufPrint (cur);
#endif
    }

	return retVal;
}

#ifdef DEBUG
void ChildDebug (ConnectionData* connections, int Id, int fileFd) {
	if (Id != 1)
		return;
	fprintf (stdout, "--------------------\n");
	fprintf (stdout, "CHILD #%d:\nC2P: %d %d\nP2C: %d %d\n", Id,
	connections[Id].C2PPipeFds[FD::READ], connections[Id].C2PPipeFds[FD::WRITE],
	(Id == 0 ? fileFd : connections[Id - 1].P2CPipeFds[FD::READ]), (Id == 0 ? -666: connections[Id - 1].P2CPipeFds[FD::WRITE]));
	fprintf (stdout, "--------------------\n");
	fflush (stdout);
}

void ParentDebug (ConnectionData* connections, int n, int deadChildren) {
	return;
	for (int Id = 0; Id < deadChildren; ++Id) {
		if (connections[Id].C2PPipeFds[FD::READ] != POISON) {
			fprintf (stdout, "NON-POISON ALERT 1 in %d\n", Id);
		}
		if (connections[Id].C2PPipeFds[FD::WRITE] != POISON) {
			fprintf (stdout, "NON-POISON ALERT 2 in %d\n", Id);
		}
		if (connections[Id].P2CPipeFds[FD::READ]!= POISON) {
			fprintf (stdout, "NON-POISON ALERT 3 in %d\n", Id);
		} 
		if (connections[Id].P2CPipeFds[FD::WRITE] != POISON) {
			fprintf (stdout, "NON-POISON ALERT 4 in %d\n", Id);
		}
	}
	for (int Id = deadChildren; Id < n; ++Id) {
	fprintf (stdout, "--------------------\n");
	fprintf (stdout, "PARENT for Id == %d in %d:\nC2P: %d %d\nP2C: %d %d\n", \
		Id, n,
		connections[Id].C2PPipeFds[FD::READ], connections[Id].C2PPipeFds[FD::WRITE],
		connections[Id].P2CPipeFds[FD::READ], connections[Id].P2CPipeFds[FD::WRITE]);
	}
	fprintf (stdout, "--------------------\n");
	fflush (stdout);
}

void BufPrint (CircleBuf* buf) {
	for (unsigned i = 0; i < buf->size; ++i)
		fprintf (stdout, "%c", *(buf->startPtr + i));
	fprintf (stdout, "\nEND\n");	
}
#endif