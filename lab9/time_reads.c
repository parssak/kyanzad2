/* The purpose of this program is to practice writing signal handling
 * functions and observing the behaviour of signals.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* Message to print in the signal handling function. */
#define MESSAGE "%ld reads were done in %ld seconds.\n"

/* Global variables to store number of read operations and seconds elapsed. 
 */
long num_reads, seconds;
void handler(int code)
{
  printf(MESSAGE, num_reads, seconds);
  exit(code);
}

/* The first command-line argument is the number of seconds to set a timer to run.
 * The second argument is the name of a binary file containing 100 ints.
 * Assume both of these arguments are correct.
 */

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage: time_reads s filename\n");
    exit(1);
  }
  seconds = strtol(argv[1], NULL, 10);

  FILE *fp;
  if ((fp = fopen(argv[2], "r")) == NULL)
  {
    perror("fopen");
    exit(1);
  }

  /* In an infinite loop, read an int from a random location in the file,
     * and print it to stderr.
     */
  // Add signal handler
  struct sigaction sigact;
  sigact.sa_flags = 0;
  sigact.sa_handler = handler;
  sigemptyset(&sigact.sa_mask);
  sigaction(SIGPROF, &sigact, NULL);

  // Add timer
  struct itimerval timer;
  timer.it_value.tv_sec = seconds;
  timer.it_value.tv_usec = seconds;

  timer.it_interval.tv_usec = 0;
  timer.it_interval.tv_sec = 0;

  if (setitimer(ITIMER_PROF, &timer, NULL) < 0)
  {
    return 0;
  }

  for (;;)
  {
    num_reads++;
    int n = 0;
    long randomPosition = sizeof(int) % rand();
    fseek(fp, randomPosition, SEEK_SET);
    fprintf(stderr, "%d\n", n);
  }
  return 1; // something is wrong if we ever get here!
}
