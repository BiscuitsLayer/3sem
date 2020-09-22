#include <cstdio>
#include <cstdlib>
#include <ctype.h>

#define DEBUG

//  RETURN CODE == 0 <- Everything is OK
//  RETURN CODE == 1 <- Input is not a number
//  RETURN CODE == 2 <- Wrong number of arguments
//  RETURN CODE == 3 <- Number is less than 1

int main (int argc, char *argv[]) {
    long long n = 0;
    //  COMMAND LINE ARGS
    if (argc == 2) {
        //  ONLY FIRST DIGIT (OR + OR -) TEST
        if (!isdigit (argv[1][0]) && (argv[1][0] != '-') && (argv[1][0] != '+')) {
            printf ("Not a number\n");
            return (1);
        }
        //  SPECIAL "ONLY SIGN" CHECK
        if ((argv[1][0] == '-' && argv[1][1] == '\0') ||
            (argv[1][0] == '+' && argv[1][1] == '\0')) {
            printf ("Not a number\n");
            return (1);
        }
        //  EVERY DIGIT TEST
        for (unsigned i = 1; argv[1][i] != 0; ++i) {
            if (!isdigit (argv[1][i])) {
                printf ("Not a number\n");
                return (1);
            }
        }
        //  GET NUMBER
        n = atoll (argv[1]);
        #ifdef DEBUG
            printf ("DEBUG: NUMBER IS %lld", n);
        #endif
    }
    else {
        printf ("Wrong number of command line arguments\n");
        return (2);
    }
    if (n < 1) {
        printf ("Number less than 1\n");
        return (3);
    }
    //  MAIN CYCLE
    #ifndef DEBUG
        for (long long i = 1; i <= n; ++i) {
            printf ("%lld ", i);
        }
    #endif
    printf ("\n");
    return 0;
}