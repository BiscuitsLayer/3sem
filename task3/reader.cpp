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

const int FILENAME_SIZE = 20;
const int MAX_SIZE = PIPE_BUF;
const char SUCCESS_CONNECT [] = "1";

int main () {

    pid_t pid = getpid ();
    char channelName [FILENAME_SIZE];
    sprintf (channelName, "channel_%d.fifo", pid);

    if (mkfifo (channelName, 0777) == -1) {
        fprintf (stderr, "Error creating channel fifo\n");
        exit (EXIT_FAILURE);
    }

    int readfile = open (channelName, O_RDONLY | O_NONBLOCK);
    if (readfile == -1) {
        fprintf (stderr, "Error opening channel fifo\n");
        exit (EXIT_FAILURE);
    }

    //char serviceName [FILENAME_SIZE];
    //serviceName = find
    int serviceWritefile = open ("service.fifo", O_WRONLY | O_NONBLOCK);
    while (serviceWritefile == -1) {
        if (errno == ENXIO) {
            //  ENXIO <- никто не открыл FIFO для чтения
            fprintf (stdout, "Writer service connection failure\n");
            exit (EXIT_FAILURE);
        }
        //  EEXIST <- файл не существует
        serviceWritefile = open ("service.fifo", O_WRONLY | O_NONBLOCK);
    }
    fprintf (stdout, "%d\n", serviceWritefile);
    
    write (serviceWritefile, SUCCESS_CONNECT, 1);

    //int fdFlags = fcntl (readfile, F_GETFD);

    char buf [MAX_SIZE];
    
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