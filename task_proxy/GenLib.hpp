#ifndef GEN_LIB_HPP_
#define GEN_LIB_HPP_

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

//#define DEBUG

#define CLOSE(fd) { 	\
	if (fd != POISON) {	\
		close (fd);		\
		fd = POISON;	\
	}					\
}

enum FD {
	READ = 0,
	WRITE = 1
};

const int MAX_CHILDREN_COUNT = 14;
const int MAX_MSG_SIZE = 100;
const int POISON = 666;

struct CircleBuf {
	char* startPtr = nullptr;
	size_t size = 0;
	int readShift = 0, writeShift = 0;
	bool isFull = false;
};

struct ConnectionData {
	pid_t parentPid = 0, childPid = 0;
	int C2PPipeFds [2];			// Child to parent
	int P2CPipeFds [2]; 		// Parent to child
	
	CircleBuf buf {};
};

void ClearBuffers (ConnectionData* connections, int n);

bool isFull (CircleBuf* buf);
bool isEmpty (CircleBuf* buf);

int ReadToBuf (ConnectionData* connections, int Id, int n);
int WriteFromBuf (ConnectionData* connections, int Id, int n);

//  PARENT
void ParentInit (ConnectionData* connections, int Id, int n, int childPid);
void ParentWork (ConnectionData* connections, int n);

//  CHILD
void ChildInit (ConnectionData* connections, int Id, int n, int* fileFd, char* filepath);
void ChildWork (ConnectionData* connections, int Id, int fileFd);

#endif //GEN_LIB_HPP_

#ifdef DEBUG
//	DEBUG PART
void ChildDebug (ConnectionData* connections, int Id, int fileFd);
void ParentDebug (ConnectionData* connections, int n, int deadChildren);
void BufPrint (CircleBuf* buf);
#endif