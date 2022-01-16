#include <stdio.h>

int main(int argc, char **argv) {
    
    char phone[11];
    int chosenIndex;

    scanf("%s %d", phone, &chosenIndex);

    if (chosenIndex < -1 || chosenIndex > 9) {
        printf("ERROR \n");
        return 1;
    }
    else if (chosenIndex == -1)
        printf("%s \n", phone);
    else
        printf("%c \n", phone[chosenIndex]);

    return 0;
}