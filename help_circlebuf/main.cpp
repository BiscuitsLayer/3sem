#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <errno.h>
#include <limits.h>
#include <cstring>

struct CircleBuf {
	char* startPtr = nullptr;
	size_t size = 0;
	int readShift = 0, writeShift = 0;
	bool isFull = false;
};

bool isFull (CircleBuf* buf) {
	return buf->isFull;
}

bool isEmpty (CircleBuf* buf) {
	return !buf->isFull && (buf->readShift == buf->writeShift);
}

int ReadToBuf (CircleBuf *cur, int fileFrom) {
	int retVal = -1;

	if (isFull (cur)) {
		fprintf (stderr, "Error: buf full\n");
		return -1;
	}
	
	if (cur->writeShift > cur->readShift) {
		retVal = read (fileFrom, cur->startPtr + cur->readShift, cur->writeShift - cur->readShift);
	}
	else {
		retVal = read (fileFrom, cur->startPtr + cur->readShift, cur->size - cur->readShift);
	}

	if (retVal < 0) {
		fprintf (stderr, "Error reading to buf in connection %d\n", 0);
	}

	cur->readShift += retVal;
    cur->readShift %= cur->size;

	if (retVal != 0) {
		if (cur->readShift == cur->writeShift)
			cur->isFull = true;
#ifdef DEBUG
fprintf (stderr, "SUCCESS!! read to buf in connection %d\n", Id);
fprintf (stderr, "readShift: %d, writeShift: %d of size %d\n", cur->readShift, cur->writeShift, cur->size);
BufPrint (cur);
#endif
	}

	return retVal;
}

int WriteFromBuf (CircleBuf *cur, int fileTo) {
	int retVal = -1;

	if (isEmpty (cur)) {
		fprintf (stderr, "Error: buf empty\n");
		return -1;
	}
	
    if (cur->writeShift >= cur->readShift) {
		if ((cur->writeShift == cur->readShift && isFull (cur)) || cur->writeShift != cur->readShift) {
	    	retVal = write (fileTo, cur->startPtr + cur->writeShift, cur->size - cur->writeShift);
		}
	}
    else {
        retVal = write (fileTo, cur->startPtr + cur->writeShift, cur->readShift - cur->writeShift);
    }
	
	if (retVal < 0) {
		fprintf (stderr, "Error writing from buf in connection %d\n", 0);
	}

	cur->writeShift += retVal;
    cur->writeShift %= cur->size;

	if (retVal != 0) {
		//* Что-то прочитали, значит буфер уже не полный
		cur->isFull = false;
#ifdef DEBUG
fprintf (stderr, "SUCCESS!! write from buf in connection %d\n", Id);
fprintf (stderr, "readShift: %d, writeShift: %d of size %d\n", cur->readShift, cur->writeShift, cur->size);
BufPrint (cur);
#endif
    }

	return retVal;
}

void BufPrint (CircleBuf* buf) {
	for (unsigned i = 0; i < buf->size; ++i)
		fprintf (stdout, "%c", *(buf->startPtr + i));
	fprintf (stdout, "\nEND\n");	
}

int main () {
	int fileFrom = open ("fileFrom.txt", O_RDONLY);
	int fileTo = open ("fileTo.txt", O_WRONLY);

	CircleBuf buf {};
	buf.size = 10;
	buf.startPtr = (char *) calloc (buf.size, sizeof (char));

	while (ReadToBuf (&buf, fileFrom)) {
		BufPrint (&buf);
		char temp[100];
		fscanf (stdin, "%s", &temp);
		if (temp[0] == 'w')
			WriteFromBuf (&buf, fileTo);
	}
	return 0;
}
