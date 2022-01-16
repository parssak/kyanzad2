#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char const *argv[])
{
    int i = 0;
    int iterations = 0;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: forkloop <iterations>\n");
        exit(1);
    }

    iterations = strtol(argv[1], NULL, 10);

    int num = 0;
    for (int a = 0; a < iterations; a++)
    {
        printf("ppid = %d, pid = %d, i = %d\n", getppid(), getpid(), i);
        num = fork();
        if (num < 0)
        {
            perror("fork");
            exit(1);
        }
        else if (num > 0)
        {
            exit(1);
        }
    }
    return 0;
}
