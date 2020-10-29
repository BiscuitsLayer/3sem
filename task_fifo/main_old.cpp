#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

const int MAX_SIZE = 1e6;

int GetFileSize (char *filename) {
    struct stat fileinfo = {};
    stat (filename, &fileinfo);
    return fileinfo.st_size;
}

int main(int argc, char *argv[]) {
    int pipefd[2];
    pid_t cpid;
    if (argc != 2) {
        fprintf (stderr, "%s", "Wrong number of arguments\n");
        exit (EXIT_FAILURE);
    }
    if (pipe (pipefd) == -1) {
        perror ("pipe");
        exit (EXIT_FAILURE);
    }
    int readSize = GetFileSize (argv[1]);
    if (readSize > MAX_SIZE) {
        perror ("filesize exceeded");
        exit (EXIT_FAILURE);
    }
    cpid = fork();
    if (cpid == -1) {
        perror ("fork");
        exit (EXIT_FAILURE);
    }
    if (cpid != 0) {    /* Parent reads from pipe */
        close (pipefd[1]);          /* Close unused write end */
        char *buf = (char *) calloc (readSize, sizeof (char));
        read (pipefd[0], buf, readSize);
        write (STDOUT_FILENO, buf, readSize);
        write (STDOUT_FILENO, "\n", 1);
        close (pipefd[0]);
        wait (NULL);
        _exit (EXIT_SUCCESS);
    } else {            /* Child writes from argv[1] to pipe */
        close (pipefd[0]);          /* Close unused read end */
        FILE *readfile = fopen (argv[1], "r");
        if (!readfile) {
            perror ("file");
            exit (EXIT_FAILURE);
        }
        char *buf = (char *) calloc (MAX_SIZE, sizeof (char));
        int readSize = read (fileno (readfile), buf, MAX_SIZE);
        write (pipefd[1], buf, readSize);
        close (pipefd[1]);          /* Reader will see EOF */
        exit (EXIT_SUCCESS);
    }
    return 0;
}
