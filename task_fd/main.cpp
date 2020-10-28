#define _GNU_SOURCE

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kcmp.h>
#include <sys/syscall.h>


int main () {
    int fd1 = open ("test1.txt", O_RDONLY);
    int fd2 = open ("test1.txt", O_RDONLY);
    fprintf (stdout, "fd1: %d, fd2: %d\n", fd1, fd2);
    pid_t pid = getpid ();
    fprintf (stdout, "%d", syscall (SYS_kcmp, pid, pid, KCMP_FILE, fd1, fd2));
    return 0;
}