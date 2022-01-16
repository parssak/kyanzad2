#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "knn.h"
#include "math.h"

/*****************************************************************************/
/* Do not add anything outside the main function here. Any core logic other  */
/*   than what is described below should go in `knn.c`. You've been warned!  */
/*****************************************************************************/

/**
 * main() takes in the following command line arguments.
 *   -K <num>:  K value for kNN (default is 1)
 *   -d <distance metric>: a string for the distance function to use
 *          euclidean or cosine (or initial substring such as "eucl", or "cos")
 *   -p <num_procs>: The number of processes to use to test images
 *   -v : If this argument is provided, then print additional debugging information
 *        (You are welcome to add print statements that only print with the verbose
 *         option.  We will not be running tests with -v )
 *   training_data: A binary file containing training image / label data
 *   testing_data: A binary file containing testing image / label data
 *   (Note that the first three "option" arguments (-K <num>, -d <distance metric>,
 *   and -p <num_procs>) may appear in any order, but the two dataset files must
 *   be the last two arguments.
 * 
 * You need to do the following:
 *   - Parse the command line arguments, call `load_dataset()` appropriately.
 *   - Create the pipes to communicate to and from children
 *   - Fork and create children, close ends of pipes as needed
 *   - All child processes should call `child_handler()`, and exit after.
 *   - Parent distributes the test dataset among children by writing:
 *        (1) start_idx: The index of the image the child should start at
 *        (2)    N:      Number of images to process (starting at start_idx)
 *     Each child should get at most N = ceil(test_set_size / num_procs) images
 *      (The last child might get fewer if the numbers don't divide perfectly)
 *   - Parent waits for children to exit, reads results through pipes and keeps
 *      the total sum.
 *   - Print out (only) one integer to stdout representing the number of test 
 *      images that were correctly classified by all children.
 *   - Free all the data allocated and exit.
 *   - Handle all relevant errors, exiting as appropriate and printing error message to stderr
 */
void usage(char *name)
{
    fprintf(stderr, "Usage: %s -v -K <num> -d <distance metric> -p <num_procs> training_list testing_list\n", name);
}

int main(int argc, char *argv[])
{

    int opt;
    int K = 1;                       // default value for K
    char *dist_metric = "euclidean"; // default distant metric
    int num_procs = 1;               // default number of children to create
    int verbose = 0;                 // if verbose is 1, print extra debugging statements
    int total_correct = 0;           // Number of correct predictions

    while ((opt = getopt(argc, argv, "vK:d:p:")) != -1)
    {
        switch (opt)
        {
        case 'v':
            verbose = 1;
            break;
        case 'K':
            K = atoi(optarg);
            break;
        case 'd':
            dist_metric = optarg;
            break;
        case 'p':
            num_procs = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Expecting training images file and test images file\n");
        exit(1);
    }

    char *training_file = argv[optind];
    optind++;
    char *testing_file = argv[optind];

    // Set which distance function to use
    /* You can use the following string comparison which will allow
     * prefix substrings to match:
     * 
     * If the condition below is true then the string matches
     * if (strncmp(dist_metric, "euclidean", strlen(dist_metric)) == 0){
     *      //found a match
     * }
     */

    double (*distance_function)(Image *, Image *);
    distance_function = distance_euclidean;
    if (strncmp(dist_metric, "cosine", strlen(dist_metric)) == 0)
    {
        if (verbose)
        {
            fprintf(stderr, "- Found a match \n");
        }
        distance_function = distance_cosine;
    }
    else if (strncmp(dist_metric, "euclidean", strlen(dist_metric)) == 0) {
        if (verbose)
        {
            fprintf(stderr, "- Found a match \n");
        }
        distance_function = distance_euclidean;
    }

        // Load data sets
        if (verbose)
        {
            fprintf(stderr, "- Loading datasets...\n");
        }

    Dataset *training = load_dataset(training_file);
    if (training == NULL)
    {
        fprintf(stderr, "The data set in %s could not be loaded\n", training_file);
        exit(1);
    }

    Dataset *testing = load_dataset(testing_file);
    if (testing == NULL)
    {
        fprintf(stderr, "The data set in %s could not be loaded\n", testing_file);
        exit(1);
    }

    // Create the pipes and child processes who will then call child_handler
    int in_pipes[num_procs][2];
    int out_pipe[2];
    if (pipe(out_pipe) < 0)
        perror("pipe");
    for (int a = 0; a < num_procs; a++)
    {
        if (pipe(in_pipes[a]) < 0)
            perror("pipe");
    }
    printf("\n");

    if (verbose)
    {
        printf("- Creating children ...\n");
    }

    int interval = testing->num_items / num_procs;
    int currIndex = 0;
    pid_t pid;

    for (int a = 0; a < num_procs; a++)
    {
        pid = fork();

        if (pid == 0) // Child process
        {
            close(in_pipes[a][1]);
            close(out_pipe[0]);

            child_handler(training, testing, K, distance_function, in_pipes[a][0], out_pipe[1]);
            
            close(out_pipe[1]);
            close(in_pipes[a][0]);

            free_dataset(training);
            free_dataset(testing);

            exit(0);
        }
        else
        {
            close(in_pipes[a][0]);
            // * If we're on the last iteration, make interval be the remainder
            if (a + 1 == num_procs)
            {
                interval = testing->num_items - currIndex;
            }

            //* Distribute to children
            if (write(in_pipes[a][1], &currIndex, sizeof(int)) == -1)
                perror("write to pipe");

            if (write(in_pipes[a][1], &interval, sizeof(int)) == -1)
                perror("write to pipe");
            close(in_pipes[a][1]);

            currIndex += interval;
        }
    }
    close(out_pipe[1]);

    // Wait for children to finish
    if (verbose)
    {
        printf("- Waiting for children...\n");
    }
    while (wait(NULL) != -1)
        ;

    // When the children have finished, read their results from their pipe
    for (int a = 0; a < num_procs; a++)
    {
        //* Read result from children
        int r_data;
        if (read(out_pipe[0], &r_data, sizeof(int)) == -1)
            perror("read to pipe");
        total_correct += r_data;
    }

    if (verbose)
    {
        printf("Number of correct predictions: %d\n", total_correct);
    }

    // This is the only print statement that can occur outside the verbose check
    printf("%d\n", total_correct);

    // Clean up any memory, open files, or open pipes
    close(out_pipe[0]);

    free_dataset(training);
    free_dataset(testing);

    return 0;
}
