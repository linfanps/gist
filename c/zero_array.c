#include <stdio.h>
#include <stdlib.h>
typedef struct sdshdr {
    int len;
    int free;
    char buf[];
} sdshdr;
int main(int argc, char **argv)
{
    sdshdr s;
    printf("sizeof(sdshrd)=%d\n", sizeof(s));
    return 0;
}

