#include "GenLib.h"

//#define DEBUG

#ifdef DEBUG
#define SLEEP(msg) {								\
	fprintf (stderr, "Sleeping... %s\n", msg);		\
	sleep (1);										\
}
#else
#define SLEEP(msg) 
#endif


int main (int argc, char **argv) {
	key_t key = ftok (argv[0], FTOK_ID);
    CHECK (key == -1);

	int semId = semget (key, SEM_ARRAY_SIZE, 0666);
	CHECK (semId == -1);

	int shmId = shmget (key, MAX_SHMEM_TEXT_SIZE + sizeof (BroadcastFlag) + sizeof (int), IPC_CREAT | 0666);
	CHECK (shmId == -1);
	
	ShmStruct *shmPtr = (ShmStruct *) shmat (shmId, nullptr, 0);
	CHECK ((void *) shmPtr == (void *) -1);

#ifdef DEBUG
	for (int i = 0; i < SEM_ARRAY_SIZE; ++i)
		fprintf (stdout, "sem %d == %d\n", i, GetValue (semId, (SemType)i));
#endif

	if (argc == 1) { 		//	READER
    	WaitZero (semId, SemType::busy);
		V (semId, SemType::readyReader, 1, false);
		P (semId, SemType::readyWriter, 1, false, 0L);
    	V (semId, SemType::busy, 1, true);

		V (semId, SemType::empty, 1, false);
		P (semId, SemType::empty, 1, true, 0L);

    	//TODO: criticalGuard check
    	V (semId, SemType::criticalGuardWriter, 1, false);
    	P (semId, SemType::criticalGuardReader, 1, false, 1L);

		char* buf = (char *) calloc (MAX_SHMEM_TEXT_SIZE, sizeof (char));
		CHECK (buf == nullptr);

    	while (true) {
			P (semId, SemType::full, 1, false, 0L);
SLEEP ("I'm in critical")
			BroadcastFlag flag = shmPtr->flag;
			shmPtr->flag = BroadcastFlag::poisonFlag;
			if (flag == BroadcastFlag::endFlag) {
				memcpy (buf, shmPtr->text, shmPtr->size);
				write (STDOUT_FILENO, buf, shmPtr->size);
			}
			else if (flag == BroadcastFlag::continueFlag) {
				memcpy (buf, shmPtr->text, MAX_SHMEM_TEXT_SIZE);
				write (STDOUT_FILENO, buf, MAX_SHMEM_TEXT_SIZE);
			}

			//* Смысл проверять, жив ли writer, есть только, если мы ждём продолжения сообщения (flag != endFlag)
			if (flag != BroadcastFlag::endFlag) {
    	    	if (GetValue (semId, SemType::busy) != 2 || flag == BroadcastFlag::poisonFlag) {
					fprintf (stderr, "Flag == poison: %d\n", flag == BroadcastFlag::poisonFlag);
					free (buf);
					fprintf (stderr, "Writer dead\n");
					exit (EXIT_FAILURE);
				}
			}

			if (flag == BroadcastFlag::endFlag)
				break;

    	    V (semId, SemType::empty, 1, false);
		}
		free (buf);
	}
	else if (argc == 2) {	//	WRITER

		int fd = open (argv[1], O_RDONLY);

		WaitZero (semId, SemType::busy);
		V (semId, SemType::readyWriter, 1, false);
		P (semId, SemType::readyReader, 1, false, 0L);
		V (semId, SemType::busy, 1, true);
		//* Теперь пара начала работать, а значит writer может замусорить ввод
		shmPtr->flag == BroadcastFlag::poisonFlag;

		V (semId, SemType::full, 1, false);
		P (semId, SemType::full, 1, true, 0L);

		//TODO: criticalGuard check 
		P (semId, SemType::criticalGuardWriter, 1, false, 1L);
		V (semId, SemType::criticalGuardReader, 1, false);

		char* buf = (char *) calloc (MAX_SHMEM_TEXT_SIZE, sizeof (char));
		CHECK (buf == nullptr);

		int retVal = read (fd, buf, MAX_SHMEM_TEXT_SIZE);
		int shmTextSize = 0;

    	while (retVal > 0) {
			P (semId, SemType::empty, 1, false, 0L);
SLEEP ("I'm in critical")
			if (GetValue (semId, SemType::busy) != 2) {
				fprintf (stderr, "Reader dead\n");
				free (buf);
				close (fd);
				exit (EXIT_FAILURE);
			}	

			memcpy (shmPtr->text, buf, MAX_SHMEM_TEXT_SIZE);
			shmTextSize = retVal;
			retVal = read (fd, buf, MAX_SHMEM_TEXT_SIZE);

			if (retVal == 0) {
#ifdef DEBUG
				fprintf (stdout, "Writer end \"%s\"\n", shmPtr->text);
#endif
				shmPtr->size = shmTextSize;
				shmPtr->flag = BroadcastFlag::endFlag;
			}
			else {
#ifdef DEBUG
				fprintf (stdout, "Writer continue \"%s\"\n", shmPtr->text);
#endif
				shmPtr->size = shmTextSize;
				shmPtr->flag = BroadcastFlag::continueFlag;
			}
    	    V (semId, SemType::full, 1, false);
		}
		//* Нейтрализуем действие SEM_UNDO на семафоре full
		V (semId, SemType::full, 1, true);
		P (semId, SemType::full, 1, false, 0L);

		free (buf);
		close (fd);
	}
	else {
		fprintf (stderr, "Usage:\nReader: ./main\nWriter: ./main [filename]\n");
	}
	shmdt (shmPtr);
	exit (EXIT_SUCCESS);
}
