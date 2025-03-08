#include <stdio.h>

int function1();
int function2();
int function3();

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
    int rv = function3();
    return rv;
}

int function3()
{
    int sum = 0;

    for (int i = 1; i <= 10; ++i) {
        sum = sum + i;
    }

    return sum;
}
