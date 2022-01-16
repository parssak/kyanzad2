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

/**
 * Load the binary file, filename into a Dataset and return a pointer to 
 * the Dataset. The binary file format is as follows:
 *
 *     -   4 bytes : `N`: Number of images / labels in the file
 *     -   1 byte  : Image 1 label
 *     - NUM_PIXELS bytes : Image 1 data (WIDTHxWIDTH)
 *          ...
 *     -   1 byte  : Image N label
 *     - NUM_PIXELS bytes : Image N data (WIDTHxWIDTH)
 *
 * You can set the `sx` and `sy` values for all the images to WIDTH. 
 * Use the NUM_PIXELS and WIDTH constants defined in dectree.h
 */
Dataset *load_dataset(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        exit(1);
    }
    Dataset *dataset = (Dataset *)malloc(sizeof(Dataset));
    int numItems = 0;
    fread(&numItems, 4, 1, file);
    
    dataset->num_items = numItems;

    Image *images = (Image *)malloc(sizeof(Image) * numItems);
    unsigned char *labels = (unsigned char *)malloc(sizeof(unsigned char) * numItems);

    for (int a = 0; a < numItems; a++)
    {
        unsigned char label = '\0';
        unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char) * NUM_PIXELS);
        fread(&label, 1, 1, file);
        fread(buffer, NUM_PIXELS, 1, file);
        Image img = {WIDTH, WIDTH, buffer};
        images[a] = img;
        labels[a] = label;
    }
    dataset->images = images;
    dataset->labels = labels;
    fclose(file);
    return dataset;
}

/**
 * Compute and return the Gini impurity of M images at a given pixel
 * The M images to analyze are identified by the indices array. The M
 * elements of the indices array are indices into data.
 * This is the objective function that you will use to identify the best 
 * pixel on which to split the dataset when building the decision tree.
 *
 * Note that the gini_impurity implemented here can evaluate to NAN 
 * (Not A Number) and will return that value. Your implementation of the 
 * decision trees should ensure that a pixel whose gini_impurity evaluates 
 * to NAN is not used to split the data.  (see find_best_split)
 * 
 * DO NOT CHANGE THIS FUNCTION; It is already implemented for you.
 */
double gini_impurity(Dataset *data, int M, int *indices, int pixel)
{
    int a_freq[10] = {0}, a_count = 0;
    int b_freq[10] = {0}, b_count = 0;

    for (int i = 0; i < M; i++)
    {
        int img_idx = indices[i];

        // The pixels are always either 0 or 255, but using < 128 for generality.
        if (data->images[img_idx].data[pixel] < 128)
        {
            a_freq[data->labels[img_idx]]++;
            a_count++;
        }
        else
        {
            b_freq[data->labels[img_idx]]++;
            b_count++;
        }
    }

    double a_gini = 0, b_gini = 0;
    for (int i = 0; i < 10; i++)
    {
        double a_i = ((double)a_freq[i]) / ((double)a_count);
        double b_i = ((double)b_freq[i]) / ((double)b_count);
        a_gini += a_i * (1 - a_i);
        b_gini += b_i * (1 - b_i);
    }

    // Weighted average of gini impurity of children
    return (a_gini * a_count + b_gini * b_count) / M;
}

/**
 * Given a subset of M images and the array of their corresponding indices, 
 * find and use the last two parameters (label and freq) to store the most
 * frequent label in the set and its frequency.
 *
 * - The most frequent label (between 0 and 9) will be stored in `*label`
 * - The frequency of this label within the subset will be stored in `*freq`
 * 
 * If multiple labels have the same maximal frequency, return the smallest one.
 */
void get_most_frequent(Dataset *data, int M, int *indices, int *label, int *freq)
{
    // Initialize the label_frequency array
    int label_frequencies[10];
    for (int c = 0; c < 10; c++)
        label_frequencies[c] = 0;

    // Populate the array
    for (int a = 0; a < M; a++)
        label_frequencies[data->labels[indices[a]]]++;

    // Find most frequent label
    int most_frequent_label = 0;
    int most_frequent_frequency = 0;
    for (int b = 0; b < 10; b++)
    {
        if (label_frequencies[b] > most_frequent_frequency)
        {
            most_frequent_frequency = label_frequencies[b];
            most_frequent_label = b;
        }
    }
    *label = most_frequent_label;
    *freq = most_frequent_frequency;
    
    return;
}

/**
 * Given a subset of M images as defined by their indices, find and return
 * the best pixel to split the data. The best pixel is the one which
 * has the minimum Gini impurity as computed by `gini_impurity()` and 
 * is not NAN. (See handout for more information)
 * 
 * The return value will be a number between 0-783 (inclusive), representing
 *  the pixel the M images should be split based on.
 * 
 * If multiple pixels have the same minimal Gini impurity, return the smallest.
 */
int find_best_split(Dataset *data, int M, int *indices)
{
    double smallest_impurity = 1000000;
    int smallest_impurity_pixel = 0;

    int dimensions = NUM_PIXELS;
    for (int b = 0; b < dimensions; b++)
    {
        double pixel_impurity = gini_impurity(data, M, indices, b);
        if (pixel_impurity != NAN && pixel_impurity < smallest_impurity)
        {
            smallest_impurity = pixel_impurity;
            smallest_impurity_pixel = b;
        }
    }
    return smallest_impurity_pixel;
}

/**
 * Create the Decision tree. In each recursive call, we consider the subset of the
 * dataset that correspond to the new node. To represent the subset, we pass 
 * an array of indices of these images in the subset of the dataset, along with 
 * its length M. Be careful to allocate this indices array for any recursive 
 * calls made, and free it when you no longer need the array. In this function,
 * you need to:
 *
 *    - Compute ratio of most frequent image in indices, do not split if the
 *      ration is greater than THRESHOLD_RATIO
 *    - Find the best pixel to split on using `find_best_split`
 *    - Split the data based on whether pixel is less than 128, allocate 
 *      arrays of indices of training images and populate them with the 
 *      subset of indices from M that correspond to which side of the split
 *      they are on
 *    - Allocate a new node, set the correct values and return
 *       - If it is a leaf node set `classification`, and both children = NULL.
 *       - Otherwise, set `pixel` and `left`/`right` nodes 
 *         (using build_subtree recursively). 
 */
DTNode *build_subtree(Dataset *data, int M, int *indices)
{
    DTNode *subtree = (DTNode *)malloc(sizeof(DTNode));
    subtree->pixel = -1;
    subtree->classification = -1;
    subtree->left = NULL;
    subtree->right = NULL;

    int label = -1;
    int freq = -1; 

    get_most_frequent(data, M, indices, &label, &freq);
    double freqD = (double)freq;
    double ratio = freqD / M; 

    if (ratio > THRESHOLD_RATIO) // Leaf Node Check
    {
        subtree->classification = label;
        return subtree;
    }

    int best_split_pixel = find_best_split(data, M, indices);
    subtree->pixel = best_split_pixel;
    int last_left = 0;
    int last_right = 0;

    int *left_indices = (int*) malloc(sizeof(int) * M);
    int *right_indices = (int *) malloc(sizeof(int) * M);

    for (int a = 0; a < M; a++) // populating arrays
    {
        Image img = data->images[indices[a]];
        if (img.data[best_split_pixel] < 128)
        {
            left_indices[last_left] = indices[a];
            last_left++;
        }
        else
        {
            right_indices[last_right] = indices[a];
            last_right++;
        }
    }

    DTNode *left_child = build_subtree(data, last_left, left_indices);
    DTNode *right_child = build_subtree(data, last_right, right_indices);

    free(left_indices);
    free(right_indices);

    subtree->left = left_child;
    subtree->right = right_child;
    return subtree;
}

/**
 * This is the function exposed to the user. All you should do here is set
 * up the `indices` array correctly for the entire dataset and call 
 * `build_subtree()` with the correct parameters.
 */
DTNode *build_dec_tree(Dataset *data)
{
    int *indices = (int *)malloc(sizeof(int) * data->num_items);
    for (int a = 0; a < data->num_items; a++)
        indices[a] = a;
    DTNode *decision_tree = NULL;
    decision_tree = build_subtree(data, data->num_items, indices);
    free(indices);
    return decision_tree;
}

/**
 * Given a decision tree and an image to classify, return the predicted label.
 */
int dec_tree_classify(DTNode *root, Image *img)
{
    // int class = -1;
    if (root->left == NULL && root->right == NULL)
        return root->classification;
    else if (img->data[root->pixel] < 128)
        return dec_tree_classify(root->left, img);
    else
        return dec_tree_classify(root->right, img);
}

/**
 * This function frees the Decision tree.
 */
void free_dec_tree(DTNode *node)
{
    
    if (node->left == NULL && node->right == NULL)
    {
        free(node);
        return;
    }

    if (node->left != NULL)
        free_dec_tree(node->left);

    if (node->right != NULL)
        free_dec_tree(node->right);

    free(node);
    return;
}

/**
 * Free all the allocated memory for the dataset
 */
void free_dataset(Dataset *data)
{
    free(data->labels);
    for (int a = 0; a < data->num_items; a++)
    {
        free(data->images[a].data);
    }
    free(data->images);
    free(data);
    return;
}
