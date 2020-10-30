#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TEST_EXIT exit (EXIT_FAILURE);

//#define DEBUG
//#define SIG_DEBUG
//#define BIN_DEBUG
//#define CHAR_DEBUG
//#define BASE_DEBUG

//  FOR MESSAGE
const int MAX_BUF_SIZE = 100;
char cur = -1;
unsigned counter = 128;

//  CONNECTION STUFF
pid_t childPid = 0;

void printfBin (char x) {
    for (int i = sizeof (char) << 3; i; --i)
        putchar('0'+ ( (x >> ( i - 1 ) ) & 1 ));
    putchar ('\n');
}

void Sig1ParentHandler (int sig) {
#ifdef SIG_DEBUG
    fprintf (stdout, "SIG1!\n");
#endif //SIG_DEBUG
    counter >>= 1;
#ifdef BIN_DEBUG
    printfBin (cur);
#endif //BIN_DEBUG
}

void Sig2ParentHandler (int sig) {
#ifdef SIG_DEBUG
    fprintf (stdout, "SIG2!\n");
#endif //SIG_DEBUG
    cur += counter;
    counter >>= 1;
#ifdef BIN_DEBUG  
    printfBin (cur);
#endif //BIN_DEBUG
}

void SigChldParentHandler (int sig) {
    fprintf (stderr, "Parent caught SIGCHLD\n");
    exit (EXIT_FAILURE);
}

void Sig1ChildHandler (int sig) {
}

void Sig2ChildHandler (int sig) {
}

void SendChar (char c, pid_t parentPid) {
    sigset_t sigSet = {};
    sigfillset (&sigSet);
    //  Только SIGUSR1 будет отвечать за получение бита
    sigdelset (&sigSet, SIGUSR1);

    for (unsigned i = 1 << 7; i >= 1; i >>= 1) {
        if (i & c) {
            kill (parentPid, SIGUSR2);
#ifdef DEBUG
        //sleep (1);
        fprintf (stderr, "(child) Bit sent\n");
#endif //DEBUG
        }
        else {
            kill (parentPid, SIGUSR1);
#ifdef DEBUG
        //sleep (1);
        fprintf (stderr, "(child) Bit sent\n");
#endif //DEBUG
        }
        //  Ожидание подтверждения получения бита (SIGUSR1)
#ifdef DEBUG
        //sleep (1);
        fprintf (stderr, "(child) Now gonna wait for approval...\n");
#endif //DEBUG
            sigsuspend (&sigSet);
        
#ifdef DEBUG
        //sleep (1);
        fprintf (stdout, "ESLI ETO SOOBSHENIE NE NAPECATALOS, TO MI ZASTRYALI NA SIGSUSPEND!\n");
        fprintf (stderr, "(child) Approval catched\n");
#endif //DEBUG
    }
}

void SendMessage (char *buf, pid_t parentPid) {
    char *temp = buf;
    sigset_t sigSet = {};
    sigfillset (&sigSet);
    //  Только SIGUSR2 будет отвечать за получение символа
    sigdelset (&sigSet, SIGUSR2);

    while (*temp != '\0') {
        SendChar (*temp, parentPid);
        //  Ожидание подтверждения получения символа (SIGUSR2)
        sigsuspend (&sigSet);
        ++temp;
    }

    SendChar ('\0', parentPid);
    //  Ожидание подтверждения получения символа (SIGUSR2)
    sigsuspend (&sigSet);
}

void GetChar () {
    sigset_t sigSet = {};
    sigfillset (&sigSet);
    sigdelset (&sigSet, SIGUSR1);
    sigdelset (&sigSet, SIGUSR2);

    for (unsigned i = 0; i < 8; ++i) {
#ifdef DEBUG
        //sleep (1);
        fprintf (stderr, "(parent) Now gonna wait for bit...\n");
#endif //DEBUG
        /* Виснет здесь */
        sigsuspend (&sigSet);
#ifdef DEBUG
        //sleep (1);
        fprintf (stderr, "(parent) Bit catched\n");
#endif //DEBUG
        //  Подтверждение получения бита
        kill (childPid, SIGUSR1);
#ifdef DEBUG
        //sleep (1);
        fprintf (stderr, "(parent) Approval sent\n");
#endif //DEBUG
    }

    if (counter == 0) counter = 128;
}

void GetMessage (char *buf) {
    char *bufStart = buf;
    while (cur != '\0' && (buf - bufStart < MAX_BUF_SIZE - 1)) {
        cur = 0;
        GetChar ();
        *buf = cur;
#ifdef CHAR_DEBUG
        fprintf (stdout, "Char caught: %c\n", *buf);
#endif //CHAR_DEBUG
        ++buf;
        //  Подтверждение получения символа
        kill (childPid, SIGUSR2);
    }
#ifdef BASE_DEBUG
    fprintf (stderr, "Well done!\n");
#endif //BASE_DEBUG
    *buf = '\0';
}

int main (int argc, char **argv) {

    if (argc != 2) {
        fprintf (stderr, "Usage: ./main [filename]\n");
        exit (EXIT_FAILURE);
    }

    int fd = open (argv[1], O_RDONLY);

    if (fd < 0) {
        fprintf (stderr, "Error opening file!\n");
        exit (EXIT_FAILURE);
    }

    char sendMessage [MAX_BUF_SIZE];
    int retVal = 0;
    while ((retVal = read (fd, sendMessage, MAX_BUF_SIZE) > 0) && (retVal > 0));

    if (retVal < 0) {
        fprintf (stderr, "Error reading from file!\n");
        exit (EXIT_FAILURE);
    }

    //  Сначала ребёнок заблокирует сигнал о готовности родителя, чтобы 
    //  выставить свои обработчики (при этом обработчик SIGCHLD всегда активен)
    sigset_t initBlockedSignals;
    sigfillset (&initBlockedSignals);
//    sigdelset (&initBlockedSignals, SIGCHLD);
    sigprocmask (SIG_SETMASK, &initBlockedSignals, nullptr);

    sigset_t sigChldMask = {};
    sigfillset (&sigChldMask);
/*
    struct sigaction sigChldHandler;
    sigChldHandler.sa_handler = SigChldParentHandler;
    sigChldHandler.sa_mask = sigChldMask;
    sigChldHandler.sa_flags = SA_NOCLDWAIT;
    sigaction (SIGCHLD, &sigChldHandler, nullptr);
*/
    childPid = fork ();
    if (childPid == 0) {   //  CHILD
        sigset_t sigMask = {};
        sigfillset (&sigMask);

        //  Ребёнок ставит свои обработчики
        struct sigaction sig1Handler;
        sig1Handler.sa_handler = Sig1ChildHandler;
        sig1Handler.sa_mask = sigMask;
        sig1Handler.sa_flags = 0;
        sigaction (SIGUSR1, &sig1Handler, nullptr);

        struct sigaction sig2Handler;
        sig2Handler.sa_handler = Sig2ChildHandler;
        sig2Handler.sa_mask = sigMask;
        sig2Handler.sa_flags = 0;
        sigaction (SIGUSR2, &sig2Handler, nullptr);
        
        //  Ждём SIGUSR2 как знак готовности родителя
        sigset_t readySet = {};
        sigfillset (&readySet);
        sigdelset (&readySet, SIGUSR2);
#ifdef DEBUG
        fprintf (stderr, "Now child gonna wait until parent ready...\n");
#endif //DEBUG
        sigsuspend (&readySet);
#ifdef DEBUG
        fprintf (stderr, "Parent ready\n");
#endif //DEBUG
        //sigaddset (&readySet, SIGUSR2);
#ifdef BASE_DEBUG
        fprintf (stderr, "Broadcast started...\n");
#endif //BASE_DEBUG
        pid_t parentPid = getppid ();

        SendMessage (sendMessage, parentPid);
        exit (EXIT_SUCCESS);
    }
    else {              //  PARENT
        sigset_t sigMask = {};
        sigfillset (&sigMask);
        
        struct sigaction sig1Handler;
        sig1Handler.sa_handler = Sig1ParentHandler;
        sig1Handler.sa_mask = sigMask;
        sig1Handler.sa_flags = 0;
        sigaction (SIGUSR1, &sig1Handler, nullptr);

        struct sigaction sig2Handler;
        sig2Handler.sa_handler = Sig2ParentHandler;
        sig2Handler.sa_mask = sigMask;
        sig2Handler.sa_flags = 0;
        sigaction (SIGUSR2, &sig2Handler, nullptr);
        
        //  Подтверждение готовности (SIGUSR2)
        kill (childPid, SIGUSR2);

        char getMessage [MAX_BUF_SIZE];
        GetMessage (getMessage);
        fprintf (stdout, "%s", getMessage);

        int status;
        waitpid (childPid, &status, 0);
        exit (EXIT_SUCCESS);
    }
}