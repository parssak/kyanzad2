/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC209H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Mustafa Quraish, Bianca Schroeder, Karen Reid
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2021 Karen Reid
 */

#include "dectree.h"

// Makefile included in starter:
//    To compile:               make
//    To decompress dataset:    make datasets
//
// Running decision tree generation / validation:
//    ./classifier datasets/training_data.bin datasets/testing_data.bin

/*****************************************************************************/
/* Do not add anything outside the main function here. Any core logic other  */
/* than what is described below should go in `dectree.c`. You've been warned!*/
/*****************************************************************************/

/**
 * main() takes in 2 command line arguments:
 *    - training_data: A binary file containing training image / label data
 *    - testing_data: A binary file containing testing image / label data
 *
 * You need to do the following:
 *    - Parse the command line arguments, call `load_dataset()` appropriately.
 *    - Call `make_dec_tree()` to build the decision tree with training data
 *    - For each test image, call `dec_tree_classify()` and compare the real 
 *        label with the predicted label
 *    - Print out (only) one integer to stdout representing the number of 
 *        test images that were correctly classified.
 *    - Free all the data dynamically allocated and exit.
 * 
 */
int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    // fprintf(stderr, "Usage: %s training_data testing_data\n", argv[0]);
    exit(1);
  }
  char *training_data = argv[1];
  char *testing_data = argv[2];
  Dataset *dataset_train = load_dataset(training_data);
  DTNode *dec_tree = build_dec_tree(dataset_train);
  Dataset *dataset_test = load_dataset(testing_data);
  int total_correct = 0;
  int total_wrong = 0;
  for (int a = 0; a < dataset_test->num_items; a++)
  {
    int classified_val = dec_tree_classify(dec_tree, &dataset_test->images[a]);
    if (classified_val == dataset_test->labels[a]) {
      total_correct++;
    } else {
      total_wrong++;
    }
  }
  free_dec_tree(dec_tree);
  free_dataset(dataset_test);
  free_dataset(dataset_train);
  printf("%d\n", total_correct);
  return 0;
}
