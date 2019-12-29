#include <stdio.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <stdbool.h>

double **generate_2d_double_array(size_t dimensions);

double **generate_non_random_array();

double **generate_random_array(size_t DIMENSIONS);

size_t average_array(double **INITIAL_ARRAY, size_t DIMENSIONS, double PRECISION);

void print_2d_array(double **array, size_t dimensions);

bool in_precision(bool current_precision, double previous_value, double current_value, double precision);

void average(double *answer, const double *up, const double *down, const double *right, const double *left);

void free_array(double **array, size_t dimensions);