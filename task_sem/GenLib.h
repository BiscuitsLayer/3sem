#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>

#define CHECK(cond) { 											\
	if (cond) {													\
        fprintf (stderr, "Line: %d, Function: %s, File: %s\n", 	\
			__LINE__, __PRETTY_FUNCTION__, __FILE__);           \
		perror (nullptr);										\
		exit (EXIT_FAILURE);									\
	}															\
}

enum SemType {
	readyReader = 0,
	readyWriter = 1,
	criticalGuardWriter = 2,
	criticalGuardReader = 3,
	busy = 4,
	full = 5,
	empty = 6
};

enum BroadcastFlag {
	poisonFlag = 0,
	continueFlag = 1,
	endFlag = 2
};

const int SEM_ARRAY_SIZE = 7;
const int MAX_STRING_SIZE = 1000;
const int MAX_SHMEM_TEXT_SIZE = 3; 
const int MAX_SHMEM_COUNT = 128;
const int FTOK_ID = 3;

struct ShmStruct {
	BroadcastFlag flag;
	int size = 0;
	char text [MAX_SHMEM_TEXT_SIZE];
};

void PrintShm (ShmStruct *shm) {
	fprintf (stdout, "Flag: %d\nSize: %d\nText: \"%s\"\n", shm->flag, shm->size, shm->text);
}

union semun {
	int val; 				/* для SETVAL */
	struct semid_ds *buf; 	/* для IPC_STAT и IPC_SET */
	unsigned short *array; 	/* для GETALL и SETALL */
};

void V (int semId, SemType type, unsigned value, bool isUndo) {
	sembuf semOp {};
	semOp.sem_num = type;
	semOp.sem_op = value;
	semOp.sem_flg = (isUndo ? SEM_UNDO : 0);
	CHECK (semop (semId, &semOp, 1) == -1);
}

void P (int semId, SemType type, unsigned value, bool isUndo, long seconds) {
	sembuf semOp {};
	semOp.sem_num = type;
	semOp.sem_op = -value;
	semOp.sem_flg = (isUndo ? SEM_UNDO : 0);
	if (seconds != 0L) {
		timespec timeout {};
		timeout.tv_sec = seconds;
		CHECK (semtimedop (semId, &semOp, 1, &timeout) == -1);
	}
	else {
		CHECK (semop (semId, &semOp, 1) == -1);
	}
}

void WaitZero (int semId, SemType type, long seconds = 0L) {
	sembuf semOp {};
	semOp.sem_num = type;
	semOp.sem_op = 0;
	semOp.sem_flg = 0;
	if (seconds != 0) {
		timespec timeout {};
		timeout.tv_sec = seconds;
		CHECK (semtimedop (semId, &semOp, 1, &timeout) == -1);
	}
	else {
		CHECK (semop (semId, &semOp, 1) == -1);
	}
}

int GetValue (int semId, SemType type) {
	int retVal = semctl (semId, type, GETVAL);
	CHECK (retVal == -1);
	return retVal;
}