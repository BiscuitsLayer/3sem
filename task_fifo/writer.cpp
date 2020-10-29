#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

const int MAX_FILENAME_SIZE = 20;
const int MAX_PID_SIZE = 10;
const int MAX_STRING_SIZE = PIPE_BUF;

int main () {

    pid_t writerPid = getpid ();
    char serviceName [MAX_FILENAME_SIZE];
    sprintf (serviceName, "service.fifo");
    char writerPidString [MAX_STRING_SIZE];
    sprintf (writerPidString, "%d", writerPid);

    //  Прототип (пример) передаваемого сообщения
    char msg [PIPE_BUF];
    for (unsigned i = 0; i < PIPE_BUF; ++i)
        msg[i] = 'a' + rand() % 20;
    msg[PIPE_BUF - 1] = '\0';

    //  WRITER создаёт свой service FIFO
    if (mkfifo (serviceName, 0777) == -1) {
        if (errno != EEXIST) {
            fprintf (stderr, "Error creating service fifo\n");
            exit (EXIT_FAILURE);
        }
    }

    //  WRITER открывает свой service FIFO
    int serviceReadfile = open (serviceName, O_RDONLY | O_NONBLOCK);
    if (serviceReadfile == -1) {
        fprintf (stderr, "Error opening service fifo\n");
        exit (EXIT_FAILURE);
    }

    //  WRITER читает из service FIFO PID READER-a
    char readerPidString [MAX_PID_SIZE];
    int readChars = 0;
    while (readChars == 0 || readChars == -1) {
        readChars = read (serviceReadfile, readerPidString, MAX_PID_SIZE);
    }

    fprintf (stdout, "Writer succeded!\n");

    //  WRITER открывает channel FIFO своего READER-a
    char channelName [MAX_FILENAME_SIZE];
    sprintf (channelName, "channel_%s.fifo", readerPidString);
    int writefile = open (channelName, O_WRONLY | O_NONBLOCK);
    while (writefile == -1) {
        if (errno == ENXIO) {
            //  ENXIO <- никто не открыл FIFO для чтения
            fprintf (stdout, "Reader has been broken\n");
            exit (EXIT_FAILURE);
        }
        //  EEXIST <- файл не существует
        writefile = open (channelName, O_WRONLY | O_NONBLOCK);
    }

    //  WRITER пишет в channel FIFO своего READER-a свой PID 
    //  (отвечает согласием на их кандидатуру)
    write (writefile, writerPidString, MAX_PID_SIZE);

    //
    /*
        В этом месте WRITER должен принять от READER-a свой же PID, таким образом
        READER подтвердит коннект со своей стороны. Если коннекта нет, то читаем
        следующий PID из служебной очереди
        (Но как, если там уже лежат другие PIDы...)
    */
    //

    fprintf (stderr, "My reader's pid: %s", readerPidString);
    sleep (100);
    exit (EXIT_SUCCESS);

    char buf = '\0';

    fprintf (stdout, "INPUT WHEN READY TO START (WRITER)\n");
    fscanf (stdin, "%c", &buf);

    int num = 0;
    unsigned cnt = 0;

    while (true) {
        num = write (writefile, msg, PIPE_BUF);

        if (num == PIPE_BUF) {
            fprintf (stderr, "WRITE SUCCESS\n");
        }
        else {
            fprintf (stderr, "WRITE FAIL, write returned: %d\n", num);
        }
        ++cnt;
        if (cnt == 4)
            break;
        sleep (1);
    }

    close (writefile);
    unlink ("channel.fifo");
    exit (EXIT_SUCCESS);
}