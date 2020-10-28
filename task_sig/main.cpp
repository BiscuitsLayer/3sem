#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

//#define DEBUG

//  FOR MESSAGE
const int MAX_BUF_SIZE = 100;
char cur = -1;
unsigned counter = 128;

//  CONNECTION STUFF
pid_t childPid = 0;

template <typename T>
void printfBin (T x) {
    for (int i = sizeof(x) << 3; i; --i)
        putchar('0'+ ( (x >> ( i - 1 ) ) & 1 ));
    putchar ('\n');
}

void SendChar (char c, pid_t parentPid) {
    sigset_t sigSet = {};
    sigfillset (&sigSet);
    //  Только SIGUSR1 будет отвечать за получение бита
    sigdelset (&sigSet, SIGUSR1);

    for (unsigned i = 1 << 7; i >= 1; i >>= 1) {
        if (i & c) {
            kill (parentPid, SIGUSR2);
        }
        else {
            kill (parentPid, SIGUSR1);
        }
        //  Ожидание подтверждения получения бита (SIGUSR1)
#ifdef DEBUG
        sleep (1);
        fprintf (stderr, "Now gonna wait for approval...\n");
#endif //DEBUG
            sigsuspend (&sigSet);
#ifdef DEBUG
        sleep (1);
        fprintf (stderr, "Approval catched\n");
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
        sleep (1);
        fprintf (stderr, "Now gonna wait for byte...\n");
#endif //DEBUG
        sigsuspend (&sigSet);
#ifdef DEBUG
        sleep (1);
        fprintf (stderr, "Byte catched\n");
#endif //DEBUG
        //  Подтверждение получения бита
        kill (childPid, SIGUSR1);
    }

    if (counter == 0) counter = 128;
}

void GetMessage (char *buf) {
    char *bufStart = buf;
    while (cur != '\0' && (buf - bufStart < MAX_BUF_SIZE - 1)) {
        cur = 0;
        GetChar ();
        *buf = cur;
        fprintf (stdout, "Char caught: %c\n", *buf);
        ++buf;
        //  Подтверждение получения символа
        kill (childPid, SIGUSR2);
    }
    fprintf (stdout, "WELL DONE!\n");
    *buf = '\0';
}


void Sig1ParentHandler (int sig) {
#ifdef SIG_DEBUG
    fprintf (stdout, "SIG1!\n");
#endif //SIG_DEBUG
    counter >>= 1;
    printfBin (cur);
}

void Sig2ParentHandler (int sig) {
#ifdef SIG_DEBUG
    fprintf (stdout, "SIG2!\n");
#endif //SIG_DEBUG
    cur += counter;
    counter >>= 1;
    printfBin (cur);
}

void SigChldParentHandler (int sig) {
    fprintf (stderr, "Parent caught SIGCHLD\n");
    exit (EXIT_FAILURE);
}

void Sig1ChildHandler (int sig) {
}

void Sig2ChildHandler (int sig) {
}

int main () {
    //  Сначала ребёнок заблокирует сигнал о готовности родителя, чтобы 
    //  выставить свои обработчики
    sigset_t initBlockedSinals;
    sigfillset (&initBlockedSinals);
    sigprocmask (SIG_SETMASK, &initBlockedSinals, nullptr);

    childPid = fork ();
    if (childPid == 0) {   //  CHILD
        //  Ребёнок ставит свои обработчики
        struct sigaction sig1Handler;
        sig1Handler.sa_handler = Sig1ChildHandler;
        sigaction (SIGUSR1, &sig1Handler, nullptr);

        struct sigaction sig2Handler;
        sig2Handler.sa_handler = Sig2ChildHandler;
        sigaction (SIGUSR2, &sig2Handler, nullptr);
        
        //  Ждём SIGUSR2 как знак готовности родителя
        sigdelset (&initBlockedSinals, SIGUSR2);
#ifdef DEBUG
        fprintf (stderr, "Now child gonna wait until parent ready...\n");
#endif //DEBUG
        sigsuspend (&initBlockedSinals);
#ifdef DEBUG
        fprintf (stderr, "Parent ready\n");
#endif //DEBUG
        sigaddset (&initBlockedSinals, SIGUSR2);

        fprintf (stderr, "Work started...\n");

        char message [] = "bitch ya vishu kak molodoi pushkin";
        pid_t parentPid = getppid ();

        SendMessage (message, parentPid);
        exit (EXIT_SUCCESS);
    }

    else {              //  PARENT
        sigset_t sigMask = {};
        sigfillset (&sigMask);
        
        struct sigaction sig1Handler;
        sig1Handler.sa_handler = Sig1ParentHandler;
        sig1Handler.sa_mask = sigMask;
        sigaction (SIGUSR1, &sig1Handler, nullptr);

        struct sigaction sig2Handler;
        sig2Handler.sa_handler = Sig2ParentHandler;
        sig2Handler.sa_mask = sigMask;
        sigaction (SIGUSR2, &sig2Handler, nullptr);

        /*
        struct sigaction sigChldHandler;
        sigChldHandler.sa_handler = SigChldParentHandler;
        sigChldHandler.sa_mask = sigMask;
        sigaction (SIGCHLD, &sigChldHandler, nullptr);
        */
        
        //  Подтверждение готовности (SIGUSR2)
        kill (childPid, SIGUSR2);

        char message [MAX_BUF_SIZE];
        GetMessage (message);
        fprintf (stdout, "Final message: %s", message);

        int status;
        wait (&status);
    }
    return 0;
}