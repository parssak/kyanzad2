#include <stdio.h>

int main(int argc, char const *argv[])
{
    char phone[11];
    int chosenIndex;
    scanf("%s", phone);

    while (scanf("%d", &chosenIndex) == 1) {
        if (chosenIndex < -1 || chosenIndex > 9)
        {
            printf("ERROR\n");
            return 1;
        }
        else if (chosenIndex == -1)
            printf("%s\n", phone);
        else
            printf("%c\n", phone[chosenIndex]);
    }

    return 0;
}
