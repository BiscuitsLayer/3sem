#include "GenLib.h"

//#define DEBUG

#define TEST_EXIT exit (EXIT_FAILURE);

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
		//* Ждём, пока завершатся все предыдущие процессы
    	WaitZero (semId, SemType::busy);

		//* Сначала говорим, что готовы сами, потом уже забираем себе напарника
		//* (чтобы он не остался без пары)
		V (semId, SemType::readyReader, 1, false);
		P (semId, SemType::readyWriter, 1, false, 0L);

		//* Начинаем работать
    	V (semId, SemType::busy, 1, true);

		//* Ставим семафоры так, чтобы в случае вылета во время
		//* передачи данных парный процесс смог войти в цикл
		V (semId, SemType::empty, 1, false);
		P (semId, SemType::empty, 1, true, 0L);

    	//* Последняя проверка перед циклом на готовность
		//* (пользуемся тем, что если получилось сделать P без IPC_NOWAIT, то должно сразу получиться)
		//TODO
    	V (semId, SemType::criticalGuardWriter, 1, false);
    	P (semId, SemType::criticalGuardReader, 1, false, 1L);

		char* buf = (char *) calloc (MAX_SHMEM_TEXT_SIZE, sizeof (char));
		CHECK (buf == nullptr);

    	while (true) {
			//* Входим в цикл
			P (semId, SemType::full, 1, false, 0L);

			//* Сохраняем флаг типа переданных данных, сам флаг мусорим
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
			
			//* Проверяем, жив ли writer (смысл в этом есть только
			//* если мы ждём продолжения сообщения (flag != endFlag)
			if (flag != BroadcastFlag::endFlag) {
    	    	if (GetValue (semId, SemType::busy) != 2 || flag == BroadcastFlag::poisonFlag) {
					free (buf);
					fprintf (stderr, "Writer dead\n");
					exit (EXIT_FAILURE);
				}
			}

			//* Если передача закончилась выходим
			if (flag == BroadcastFlag::endFlag)
				break;

			//* Если передача не закончилась, запускаем writer
			//* если же передача закончилась, семафор сам увеличится
			//* из-за SEM_UNDO
    	    V (semId, SemType::empty, 1, false);
		}
		free (buf);
	}
	else if (argc == 2) {	//	WRITER

		int fd = open (argv[1], O_RDONLY);
		//* Ждём, пока завершатся все предыдущие процессы
		WaitZero (semId, SemType::busy);

		//* Сначала говорим, что готовы сами, потом уже забираем себе напарника
		//* (чтобы он не остался без пары)
		V (semId, SemType::readyWriter, 1, false);
		P (semId, SemType::readyReader, 1, false, 0L);

		//* Начинаем работать
		V (semId, SemType::busy, 1, true);

		//* Теперь пара начала работать, а значит writer может замусорить флаг
		shmPtr->flag == BroadcastFlag::poisonFlag;

		//* Ставим семафоры так, чтобы в случае вылета во время
		//* передачи данных парный процесс смог войти в цикл
		V (semId, SemType::full, 1, false);
		P (semId, SemType::full, 1, true, 0L);

		//* Последняя проверка перед циклом на готовность
		//TODO
		P (semId, SemType::criticalGuardWriter, 1, false, 1L);
		V (semId, SemType::criticalGuardReader, 1, false);

		char* buf = (char *) calloc (MAX_SHMEM_TEXT_SIZE, sizeof (char));
		CHECK (buf == nullptr);

		//* Первый раз читаем из файла
		int retVal = read (fd, buf, MAX_SHMEM_TEXT_SIZE);
		int shmTextSize = 0;

    	while (retVal > 0) {
			//* Входим в цикл
			P (semId, SemType::empty, 1, false, 0L);

			//* Проверяем, жив ли reader
			if (GetValue (semId, SemType::busy) != 2) {
				fprintf (stderr, "Reader dead\n");
				free (buf);
				close (fd);
				exit (EXIT_FAILURE);
			}	

			//* Кладём прочитанные данные в память и сразу читаем
			//* следующие, чтобы понять, достигнут ли конец файла
			memcpy (shmPtr->text, buf, MAX_SHMEM_TEXT_SIZE);
			shmTextSize = retVal;
			retVal = read (fd, buf, MAX_SHMEM_TEXT_SIZE);

			//* Даём reader-у понять, сколько символов считывать и
			//* нужно ли ждать продолжения сообщения
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

			//* Если передача не закончилась, запускаем reader
			//* если же передача закончилась, семафор сам увеличится
			//* из-за SEM_UNDO
    	    V (semId, SemType::full, 1, false);
		}
		//* Нейтрализуем действие SEM_UNDO на семафоре full
		//* (иначе он увеличится при завершении процесса)
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