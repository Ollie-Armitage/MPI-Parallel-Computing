#include <stdio.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <stdbool.h>

double **generate_2d_double_array(int x_dimension, int y_dimension);

void generate_non_random_array(int DIMENSIONS, double array[][DIMENSIONS]);

void generate_random_array(int DIMENSIONS, double array[][DIMENSIONS]);

size_t average_array(double **INITIAL_ARRAY, int DIMENSIONS, double PRECISION);

void print_2d_array(int rows, int columns, double array[rows][columns]);

bool in_precision(bool current_precision, double previous_value, double current_value, double precision);

void average(double *answer, const double *up, const double *down, const double *right, const double *left);

void free_array(double **array, int dimensions);

double *matrix_to_array(double **array, int dimensions);

void print_1D_array(double *array, int row, int column);
