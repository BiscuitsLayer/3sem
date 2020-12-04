#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kcmp.h>
#include <sys/syscall.h>


int main () {
    mkfifo ("error.fifo", 666);
    mkfifo ("true.fifo", 0666);
    return 0;
}