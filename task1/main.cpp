#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <cstring>

const unsigned MAX_SIZE = 1e6;
const int END_SYMBOL = -1;

//  #define DEBUG

//  RETURN CODE == 0 <- Everything is OK
//  RETURN CODE == 1 <- Input is not a number
//  RETURN CODE == 2 <- Wrong number of arguments
//  RETURN CODE == 3 <- Number is less than 1

unsigned GetNumber (char* str, short* number);
void NextNumber (short* number, unsigned* len);
bool CheckEqualArrays (short* arg1, short* arg2);
void PrintNumber (short* number, unsigned len);

int main (int argc, char *argv[]) {
    short number [MAX_SIZE];
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
        if (argv[1][0] == '-') {
            printf ("Number is less than 1\n");
            return (3);
        }
        if (argv[1][0] == '+') {
            argv[1][0] = '0';
        }
        //  GET NUMBER
        GetNumber (argv[1], number);
    }
    else {
        printf ("Wrong number of command line arguments\n");
        return (2);
    }
    if (number[0] == 0) {
        printf ("Number is less than 1\n");
        return (3);
    }
    //  MAIN CYCLE
    #ifndef DEBUG
        short ans [MAX_SIZE];
        ans[0] = 1;
        ans[1] = END_SYMBOL;
        unsigned ansLen = 1;
        bool flag = true;
        while (flag) {
            PrintNumber (ans, ansLen);
            if (CheckEqualArrays (ans, number))
                flag = false;
            NextNumber (ans, &ansLen);
        }
    #endif
    return 0;
}


/*
    @return - result length
*/

unsigned GetNumber (char* str, short* number) {
    unsigned startValue = 0;
    unsigned len = strlen (str);
    if (str[0] == '+' || str[0] == '-') {
        ++startValue;
        len--;
    }
    for (unsigned i = startValue; i < len; ++i)
        number[i] = str[len - 1 - i] - '0';
    number[len] = END_SYMBOL;
    //  LEADING ZEROS DELETE
    while (number[len - 1] == 0) {
        number[len - 1] = END_SYMBOL;
        --len;
    }
    
    return len;
}

/*
    @return - none
*/

void NextNumber (short* number, unsigned* len) {
    unsigned i = 0;
    while (number[i] + 1 == 10) {
        number[i] = 0;
        if (number[i + 1] == END_SYMBOL) {
            number[i + 1] = 0;
            number[i + 2] = END_SYMBOL;
            ++*len;
        }
        ++i;
    }
    ++number[i];
}

/*
    @return - arg1 == arg2
*/

bool CheckEqualArrays (short* arg1, short* arg2) {
    unsigned i = 0;
    while (arg1[i] != END_SYMBOL) {
        if (arg1[i] != arg2[i])
            return false;
        ++i;
    }
    return (arg2[i] == END_SYMBOL);
}

/*
    @return - none
*/

void PrintNumber (short* number, unsigned len) {
    int i = len - 1;
    while (i >= 0) {
        printf ("%d", number[i]);
        --i;
    }
    printf ("\n");
}