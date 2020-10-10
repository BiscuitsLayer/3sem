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

    pid_t readerPid = getpid ();
    char channelName [MAX_FILENAME_SIZE];
    sprintf (channelName, "channel_%d.fifo", readerPid);
    char readerPidString [MAX_PID_SIZE];
    sprintf (readerPidString, "%d", readerPid);

    //  READER создаёт свой channel FIFO
    if (mkfifo (channelName, 0777) == -1) {
        fprintf (stderr, "Error creating channel fifo\n");
        exit (EXIT_FAILURE);
    }

    //  READER открывает свой channel FIFO
    int readfile = open (channelName, O_RDONLY | O_NONBLOCK);
    if (readfile == -1) {
        fprintf (stderr, "Error opening channel fifo\n");
        exit (EXIT_FAILURE);
    }

    //  READER перебирает все возможные service FIFO
    char serviceName [MAX_FILENAME_SIZE];
    int serviceWritefile = -1;

    //  При этом READER каждый раз читает из channel FIFO
    //  в надежде, что кто-нибудь ответит
    char writerPidString [MAX_PID_SIZE];
    int readChars = 0;
    
    while (readChars == 0 || readChars == -1) {
        for (pid_t writerPid = 0; writerPid <= 32768; ++writerPid) {
            sprintf (serviceName, "service_%d.fifo", writerPid);
            serviceWritefile = open (serviceName, O_WRONLY | O_NONBLOCK);
            if (serviceWritefile == -1 && errno == EEXIST)
                continue;
            else if (serviceWritefile != -1)
                //  READER записывает в них свой PID (предлагает свою кандидатуру)
                write (serviceWritefile, readerPidString, MAX_PID_SIZE);
        }
        readChars = read (readfile, writerPidString, MAX_PID_SIZE);
    }

    //
    /*
        В этом месте READER должен передать WRITER-у его PID, таким образом
        READER подтвердит коннект со своей стороны.
    */
    //

    fprintf (stderr, "My writer's pid: %s", writerPidString);
    sleep (100);
    exit (EXIT_SUCCESS);

    //  НА ЭТОМ МОМЕНТЕ READER СДЕЛАЛ ВСЁ, ЧТО В ЕГО СИЛАХ
    //  ДАЛЕЕ ОН ПРОСТО ОЖИДАЕТ ПОДТВЕРЖДЕНИЯ ПО CHANNEL FIFO
    //  ЕСЛИ ПОДТВЕРЖДЕНИЯ ДОЛГО НЕТ, ТО READER ПРОСТО УМИРАЕТ

    //  select если read возвращает -1
    
    char buf [MAX_STRING_SIZE];
    
    fprintf (stdout, "INPUT WHEN READY TO START (READER)\n");
    fscanf (stdin, "%s", buf);

    int num = 0;

    while ((num = read (readfile, buf, PIPE_BUF)), num >= 0) {
        if (num == PIPE_BUF) 
            fprintf (stderr, "READ SUCCESS\n");
        else if (num == 0) {
            fprintf (stderr, "ZERO\n");
            break;
        } 
        else 
            fprintf (stderr, "FAILURE\n");
        sleep (1);
    }
    
    if (num == -1 && errno == EAGAIN) {
    //  EAGAIN <- сейчас нет данных в FIFO
        fprintf (stdout, "Writer has been broken\n");
    }

    close (readfile);
    unlink ("channel.fifo");
    exit (EXIT_SUCCESS);
}