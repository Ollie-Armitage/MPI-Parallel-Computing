#include <stdio.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <stdbool.h>


void generate_non_random_array(int DIMENSIONS, double array[][DIMENSIONS]);

void generate_random_array(int DIMENSIONS, double array[][DIMENSIONS]);


void print_2d_array(int rows, int columns, double **array);

bool in_precision(double previous_value, double current_value, double precision);

void average(double *answer, const double *up, const double *down, const double *right, const double *left);

void print_1D_array(double *array, int row, int column);
void
setup_args(int *dimensions, double *precision, int *number_of_processes, int *process_rank, int *argc, char ***argv,
           bool *d_test_flag, bool *p_test_flag, bool *v_test_flag);


void set_scatterv_values(int dimensions, int inner_dimensions, int processes, int *send_counts, int *displacements);

double **build_processor_array(int processor_rows, int processor_columns, double *receive_buffer, double *top_row,
                               double *bottom_row, int my_rank, int number_of_processes);
