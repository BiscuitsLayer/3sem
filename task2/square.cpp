#include <cstdio>
#include <cassert>
#include <cstdlib>

int main (int argc, char **argv) {
    assert (argc == 2);
    int n = atoll (argv[1]);
    printf ("square of %d is %d\n", n, n * n);
    return 0;
}