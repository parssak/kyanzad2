#include <stdio.h>
#include <math.h> // Need this for sqrt()
#include <stdlib.h>
#include <string.h>

#include "knn.h"

/* Print the image to standard output in the pgmformat.  
 * (Use diff -w to compare the printed output to the original image)
 */
void print_image(unsigned char *image)
{
    printf("P2\n %d %d\n 255\n", WIDTH, HEIGHT);
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        printf("%d ", image[i]);
        if ((i + 1) % WIDTH == 0)
        {
            printf("\n");
        }
    }
}

/* Return the label from an image filename.
 * The image filenames all have the same format:
 *    <image number>-<label>.pgm
 * The label in the file name is the digit that the image represents
 */
unsigned char get_label(char *filename)
{
    // Find the dash in the filename
    char *dash_char = strstr(filename, "-");
    // Convert the number after the dash into a integer
    return (char)atoi(dash_char + 1);
}

/* ******************************************************************
 * Complete the remaining functions below
 * ******************************************************************/

/* Read a pgm image from filename, storing its pixels
 * in the array img.
 * It is recommended to use fscanf to read the values from the file. First, 
 * consume the header information.  You should declare two temporary variables
 * to hold the width and height fields. This allows you to use the format string
 * "P2 %d %d 255 "
 * 
 * To read in each pixel value, use the format string "%hhu " which will store
 * the number it reads in an an unsigned character.
 * (Note that the img array is a 1D array of length WIDTH*HEIGHT.)
 */
void load_image(char *filename, unsigned char *img)
{
    // Open corresponding image file, read in header (which we will discard)
    FILE *f2 = fopen(filename, "r");
    if (f2 == NULL)
    {
        perror("fopen");
        exit(1);
    }
    int width;
    int height;

    fscanf(f2, "P2 %d %d 255 ", &width, &height);

    for (int a = 0; a < width * height; a++)
    {
        fscanf(f2, "%hhu ", &img[a]);
    }
    fclose(f2);
}

/**
 * Load a full dataset into a 2D array called dataset.
 *
 * The image files to load are given in filename where
 * each image file name is on a separate line.
 * 
 * For each image i:
 *  - read the pixels into row i (using load_image)
 *  - set the image label in labels[i] (using get_label)
 * 
 * Return number of images read.
 */
int load_dataset(char *filename,
                 unsigned char dataset[MAX_SIZE][NUM_PIXELS],
                 unsigned char *labels)
{
    // We expect the file to hold up to MAX_SIZE number of file names
    FILE *f1 = fopen(filename, "r");
    if (f1 == NULL)
    {
        perror("fopen");
        exit(1);
    }

    char buffer[MAX_NAME];
    int numRead = 0;

    while (fscanf(f1, "%s ", buffer) == 1)
    {
        if (numRead < MAX_SIZE)
        {
            load_image(buffer, dataset[numRead]);
            labels[numRead] = get_label(buffer);
            numRead++;
        }
        else
        {
            break;
        }
    }

    fclose(f1);
    return numRead;
}

/** 
 * Return the euclidean distance between the image pixels in the image
 * a and b.  (See handout for the euclidean distance function)
 */
double distance(unsigned char *a, unsigned char *b)
{

    int sum = 0;
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        sum += (b[i] - a[i]) * (b[i] - a[i]);
    }
    double val = sqrt(sum);
    return val;
}

/**
 * Return the most frequent label of the K most similar images to "input"
 * in the dataset
 * Parameters:
 *  - input - an array of NUM_PIXELS unsigned chars containing the image to test
 *  - K - an int that determines how many training images are in the 
 *        K-most-similar set
 *  - dataset - a 2D array containing the set of training images.
 *  - labels - the array of labels that correspond to the training dataset
 *  - training_size - the number of images that are actually stored in dataset
 * 
 * Steps
 *   (1) Find the K images in dataset that have the smallest distance to input
 *         When evaluating an image to decide whether it belongs in the set of 
 *         K closest images, it will only replace an image in the set if its
 *         distance to the test image is strictly less than all of the images in 
 *         the current K closest images.
 *   (2) Count the frequencies of the labels in the K images
 *   (3) Return the most frequent label of these K images
 *         In the case of a tie, return the smallest value digit.
 */

int knn_predict(unsigned char *input, int K,
                unsigned char dataset[MAX_SIZE][NUM_PIXELS],
                unsigned char *labels,
                int training_size)
{

    typedef struct Image
    {
        unsigned char label;
        double distance;
    } Image;

    Image images[K];
    int array_index = 0;

    // -- Fill up images array until full --
    for (int a = 0; a < K; a++)
    {
        double d = distance(input, dataset[a]);
        unsigned char img_label = labels[a];

        Image new_img;
        new_img.label = img_label;
        new_img.distance = d;

        images[array_index] = new_img;
        array_index++;
    }

    // -- Sort Array --  [ Bubble sort ]
    for (int b = 0; b < K - 1; b++)
    {
        for (int z = 0; z < K - b - 1; z++)
        {
            if (images[z].distance > images[z + 1].distance)
            {
                Image temp = images[z];
                images[z] = images[z + 1];
                images[z + 1] = temp;
            }
        }
    }

    double curr_smallest = images[0].distance;
    for (int c = K; c < training_size; c++) // Where it left off from filling array
    {
        double d = distance(input, dataset[c]);
        if (d < curr_smallest) // This is closer than the current smallest element
        {
            // Shift all array elements down one
            for (int x = K - 1; x > 0; x--)
            {
                images[x] = images[x - 1];
            }

            // Add new image at index 0
            unsigned char img_label = labels[c];
            Image new_img;
            new_img.label = img_label;
            new_img.distance = d;
            images[0] = new_img;
            curr_smallest = d;
        }
    }

    // -- Getting label with highest frequency --

    int label_freqs[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};      // Storing Frequencies
    int label_freqs_vals[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Storing Values

    for (int i = 0; i < K; i++)
    {
        // Add one to frequency
        label_freqs[images[i].label] += 1;

        // Set value in parallel array to this label
        label_freqs_vals[images[i].label] = images[i].label;
    }

    int highest_freq = 0;
    int highest_freq_index = 0;

    for (int j = 0; j < 10; j++)
    {
        if (label_freqs[j] > highest_freq)
        {
            highest_freq = label_freqs[j];
            highest_freq_index = j;
        }
    }

    int highest_frequency_value = label_freqs_vals[highest_freq_index];
    return highest_frequency_value;
}
