#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256
#define MAX_PASSWORD 10

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void)
{
  char user_id[MAXLINE];
  char password[MAXLINE];

  /* The user will type in a user name on one line followed by a password 
     on the next.
     DO NOT add any prompts.  The only output of this program will be one 
	 of the messages defined above.
     Please read the comments in validate carefully.
   */
  if (fgets(user_id, MAXLINE, stdin) == NULL)
  {
    perror("fgets");
    exit(1);
  }
  if (fgets(password, MAXLINE, stdin) == NULL)
  {
    perror("fgets");
    exit(1);
  }
  if (strlen(user_id) > MAX_PASSWORD) {
    printf(NO_USER);
    exit(1);
  }
  if (strlen(password) > MAX_PASSWORD)
  {
    printf(INVALID);
    exit(1);
  }

  int fd[2];
  if (pipe(fd) == -1)
    perror("pipe");
  
  int childPid = fork();
  if (childPid > 0)
  {
    close(fd[0]);
    if (write(fd[1], user_id, MAX_PASSWORD) == -1)
      perror("write to pipe");
    if (write(fd[1], password, MAX_PASSWORD) == -1)
      perror("write to pipe");

    int status;
    if (wait(&status) != -1)
    {
      switch (WEXITSTATUS(status))
      {
      case 0:
        printf(SUCCESS);
        break;
      case 1:
      case 3:
        printf(NO_USER);
        break;
      default:
        printf(INVALID);
      }
    }
  }
  else if (childPid == 0)
  {
    close(fd[1]);
    if (dup2(fd[0], STDIN_FILENO) == -1)
      perror("dup2");
    close(fd[0]);
    if (execl("./validate", "validate", NULL) == -1)
      perror("execl");
    }
  return 0;
}
