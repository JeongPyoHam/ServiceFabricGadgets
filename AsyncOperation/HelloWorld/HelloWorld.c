#include <stdio.h>

int function1();
int function2();
int function3(int from, int to);

int main()
{
    printf("hello world\r\n");

    int rv = function1();

    printf("function1: %d\r\n", rv);

    return 0;
}

int function1()
{
    int rv = function2();
    return rv;
}

int function2()
{
    int from = 1;
    int to = 10;

    int rv = function3(from, to);
    return rv;
}

int function3(int from, int to)
{
    if (from > to) {
        return -1;
    }

    int sum = 0;

    for (int i = from; i <= to; ++i) {
        sum = sum + i;
    }

    return sum;
}
