#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>

#define TEST_EXIT exit (EXIT_FAILURE);

//  FOR MESSAGE
char cur = 100;
unsigned counter = 128;
bool msgEnd = false;

//  CONNECTION STUFF
pid_t childPid = 0;

void printfBin (char x) {
    for (int i = sizeof (char) << 3; i; --i)
        putchar('0'+ ( (x >> ( i - 1 ) ) & 1 ));
    putchar ('\n');
}

void Sig1ParentGetMessageHandler (int sig) {
}

void Sig2ParentGetMessageHandler (int sig) {
    msgEnd = true;
}

void Sig1ParentGetCharHandler (int sig) {
    counter >>= 1;
}

void Sig2ParentGetCharHandler (int sig) {
    cur += counter;
    counter >>= 1;
}

void SigChldParentHandler (int sig) {
    fprintf (stderr, "Parent caught SIGCHLD\n");
    exit (EXIT_FAILURE);
}

void SigChldParentOK (int sig) {
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
        }
        else {
            kill (parentPid, SIGUSR1);
        }
        //  Ожидание подтверждения получения бита (SIGUSR1)
        sigsuspend (&sigSet);
    }
}

void SendMessage (int fd, pid_t parentPid) {
    char temp = '\0';
    sigset_t sigSet = {};
    sigfillset (&sigSet);
    //  Только SIGUSR2 будет отвечать за получение символа
    sigdelset (&sigSet, SIGUSR2);

    int retVal = 0;

    while ((retVal = read (fd, &temp, 1) > 0) && retVal > 0) {
        //  SIGUSR1 если сообщение еще не кончилось, SIGUSR2 если кончилось
        kill (parentPid, SIGUSR1);
        //  Ожидание подтверждения получения информации о следующем символе (SIGUSR2)
        sigsuspend (&sigSet);
        //  Передача символа
        SendChar (temp, parentPid);
        //  Ожидание подтверждения получения символа (SIGUSR2)
        sigsuspend (&sigSet);
    }

    //  SIGUSR1 если сообщение еще не кончилось, SIGUSR2 если кончилось
    kill (parentPid, SIGUSR2);
    //  Ожидание подтверждения получения информации о следующем символе (SIGUSR2)
    sigsuspend (&sigSet);

    if (retVal < 0) {
        fprintf (stderr, "Error reading from file!");
        exit (EXIT_FAILURE);
    }
}

void GetChar () {
    sigset_t sigSet = {};
    sigfillset (&sigSet);
    sigdelset (&sigSet, SIGUSR1);
    sigdelset (&sigSet, SIGUSR2);
    sigdelset (&sigSet, SIGCHLD);

    for (unsigned i = 0; i < 8; ++i) {
        sigsuspend (&sigSet);
        //  Подтверждение получения бита
        kill (childPid, SIGUSR1);
    }

    if (counter == 0) counter = 128;
}

void GetMessage () {
    sigset_t getMessageSet = {};
    sigfillset (&getMessageSet);
    sigdelset (&getMessageSet, SIGUSR1);
    sigdelset (&getMessageSet, SIGUSR2);
    sigdelset (&getMessageSet, SIGCHLD);

    //  Получаем тип следующего символа
    sigsuspend (&getMessageSet);

    while (!msgEnd) {
        sigset_t sigMask = {};
        sigfillset (&sigMask);

        //  Ставим новые обработчики для получения символа
        struct sigaction sig1Handler;
        sig1Handler.sa_handler = Sig1ParentGetCharHandler;
        sig1Handler.sa_mask = sigMask;
        sig1Handler.sa_flags = 0;
        sigaction (SIGUSR1, &sig1Handler, nullptr);

        struct sigaction sig2Handler;
        sig2Handler.sa_handler = Sig2ParentGetCharHandler;
        sig2Handler.sa_mask = sigMask;
        sig2Handler.sa_flags = 0;
        sigaction (SIGUSR2, &sig2Handler, nullptr);

        //  Подтвержаем получение информации о следующем символе
        kill (childPid, SIGUSR2);

        cur = 0;

        //  Получение символа
        GetChar ();
        fprintf (stdout, "%c", cur);

        //  Ставим новые обработчики для получения типа следующего символа
        sig1Handler.sa_handler = Sig1ParentGetMessageHandler;
        sig1Handler.sa_mask = sigMask;
        sig1Handler.sa_flags = 0;
        sigaction (SIGUSR1, &sig1Handler, nullptr);

        sig2Handler.sa_handler = Sig2ParentGetMessageHandler;
        sig2Handler.sa_mask = sigMask;
        sig2Handler.sa_flags = 0;
        sigaction (SIGUSR2, &sig2Handler, nullptr);

        //  Подтверждение получения символа
        kill (childPid, SIGUSR2);

        //  Получаем тип следующего символа
        sigsuspend (&getMessageSet);
    }   
}

int main (int argc, char **argv) {

    if (argc != 2) {
        fprintf (stderr, "Usage: ./main [filename]\n");
        exit (EXIT_FAILURE);
    }

    //  Сначала ребёнок заблокирует сигнал о готовности родителя, чтобы 
    //  выставить свои обработчики (при этом обработчик SIGCHLD всегда активен)
    sigset_t initBlockedSignals;
    sigfillset (&initBlockedSignals);
    sigdelset (&initBlockedSignals, SIGCHLD);
    sigprocmask (SIG_SETMASK, &initBlockedSignals, nullptr);

    sigset_t sigChldMask = {};
    sigfillset (&sigChldMask);

    struct sigaction sigChldHandler;
    sigChldHandler.sa_handler = SigChldParentHandler;
    sigChldHandler.sa_mask = sigChldMask;
    sigChldHandler.sa_flags = SA_NOCLDWAIT;
    sigaction (SIGCHLD, &sigChldHandler, nullptr);

    childPid = fork ();
    if (childPid == 0) {   //  CHILD <- отправляет сообщение
        //  Ребёнок тоже будет умирать при смерти родителя
        prctl (PR_SET_PDEATHSIG, SIGKILL);

        if (getppid () == 1) {
            fprintf (stderr, "Parent died\n");
            exit (EXIT_FAILURE);
        }

        int fd = open (argv[1], O_RDONLY);

        if (fd < 0) {
            fprintf (stderr, "Error opening file!\n");
            exit (EXIT_FAILURE);
        }

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
        sigsuspend (&readySet);
        sigaddset (&readySet, SIGUSR2);

        pid_t parentPid = getppid ();
        
        SendMessage (fd, parentPid);  
        
        //  Далее, перед тем как умереть, ребёнок ждёт SIGUSR1 как подтверждение того, что родитель готов к его смерти
        //  (до этого я делал так, чтобы ребёнок ждал SIGUSR2, но это не работало, тк скорее всего ребёнку просто
        //  приходило два SIGUSR2 подряд, и он не различал это как два раздельных сигнала)
        sigset_t sigSet = {};
        sigfillset (&sigSet);
        sigdelset (&sigSet, SIGUSR1);
        sigsuspend (&sigSet);

        exit (EXIT_SUCCESS);
    }
    else {              //  PARENT <- получает сообщение
        sigset_t sigMask = {};
        sigfillset (&sigMask);
        sigdelset (&sigMask, SIGCHLD);
        
        struct sigaction sig1Handler;
        sig1Handler.sa_handler = Sig1ParentGetMessageHandler;
        sig1Handler.sa_mask = sigMask;
        sig1Handler.sa_flags = 0;
        sigaction (SIGUSR1, &sig1Handler, nullptr);

        struct sigaction sig2Handler;
        sig2Handler.sa_handler = Sig2ParentGetMessageHandler;
        sig2Handler.sa_mask = sigMask;
        sig2Handler.sa_flags = 0;
        sigaction (SIGUSR2, &sig2Handler, nullptr);
        
        //  Подтверждение готовности (SIGUSR2)
        kill (childPid, SIGUSR2);
        GetMessage ();
        
        //  Сообщение о смерти ребёнка: сначала ребёнок ждёт SIGUSR1, затем умирает, отправляя SIGCHLD

        //  1) Готовим маску, принимающую SIGHCHLD; её же будем использовать при обработке SIGCHLD
        sigset_t deadChldMask;
        sigfillset (&deadChldMask);
        sigdelset (&deadChldMask, SIGCHLD);

        //  2) Ставим новый обработчик SIGCHLD
        struct sigaction sigChldOK;
        sigChldOK.sa_handler = SigChldParentOK;
        sigChldOK.sa_mask = deadChldMask;
        sigChldOK.sa_flags = SA_NOCLDWAIT;
        sigaction (SIGCHLD, &sigChldOK, nullptr);

        //  3) Посылаем ребёнку SIGUSR1, подтвержая, что мы готовы к его смерти
        kill (childPid, SIGUSR1);
        
        exit (EXIT_SUCCESS);
    }
}