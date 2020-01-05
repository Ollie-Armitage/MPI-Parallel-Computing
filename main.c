#include "main.h"

double **array_to_matrix(double *array, int dimensions);

void
setup_args(int *dimensions, double *precision, int *number_of_processes, int *process_rank, int *argc, char ***argv,
           bool *d_test_flag, bool *p_test_flag, bool *v_test_flag);


void set_scatterv_values(int dimensions, int inner_dimensions, int processes, int *send_counts, int *displacements);

int main(int argc, char *argv[]) {

    /* Default environment variables initialized in case not input from command line arguments. */
    int DIMENSIONS = 5, NUMBER_OF_PROCESSES, my_rank;;
    double PRECISION = 0.01;

    /* Command line flag checks initialized. */
    bool d_test_flag = false, p_test_flag = false, v_test_flag = false;

    /* MPI setup. */
    MPI_Init(&argc, &argv);

    setup_args(&DIMENSIONS, &PRECISION, &NUMBER_OF_PROCESSES, &my_rank, &argc, &argv, &d_test_flag, &p_test_flag,
               &v_test_flag);
    int INNER_DIMENSIONS = DIMENSIONS - 2;

    if (DIMENSIONS <= 2) {
        if (my_rank == 0) printf("Dimensions supplied invalid. Minimum dimensions of 3x3.\n");
        exit(-1);
    }

    /* Variables declared for the purposes of scattering the values from the root process. */
    int send_counts[NUMBER_OF_PROCESSES];
    int displacements[NUMBER_OF_PROCESSES];
    double *send_buffer = NULL;
    double top_buffer[DIMENSIONS];
    double bottom_buffer[DIMENSIONS];

    /* Argument setup ends. */

    if (my_rank == 0) {
        /* For the purposes of this coursework, 2d array is generated on the first process before data is passed to other
        * processes. If not supplied a dimension, non-random 5x5 is generated. If supplied a dimension, random array
         * of that dimension is generated.*/
        double INITIAL_ARRAY[DIMENSIONS][DIMENSIONS];
        if (!d_test_flag) generate_non_random_array(DIMENSIONS, INITIAL_ARRAY);
        else generate_random_array(DIMENSIONS, INITIAL_ARRAY);

        /* From this point onwards is the parallelism of the program. */

        /* In order to use the MPI_Scatterv function:
         * - 2d double array must be converted into 1d for the send buffer. This is only relevant on the root process*/

        send_buffer = malloc(sizeof(double) * INNER_DIMENSIONS * DIMENSIONS);
        memcpy(top_buffer, INITIAL_ARRAY[0], sizeof(double) * DIMENSIONS);


        for (int i = 0; i < INNER_DIMENSIONS; i++) {
            memcpy(&send_buffer[i * DIMENSIONS], INITIAL_ARRAY[i + 1], sizeof(double) * DIMENSIONS);
        }

        if (NUMBER_OF_PROCESSES > 1)
            MPI_Ssend(INITIAL_ARRAY[DIMENSIONS - 1], DIMENSIONS, MPI_DOUBLE, NUMBER_OF_PROCESSES - 1, 0,
                      MPI_COMM_WORLD);
        else memcpy(bottom_buffer, INITIAL_ARRAY[DIMENSIONS - 1], sizeof(double) * DIMENSIONS);

        if (v_test_flag) {
            printf("Split up array by dimensions. (Dimensions supplied: %d)\n", DIMENSIONS);
            print_2d_array(DIMENSIONS, DIMENSIONS, INITIAL_ARRAY);
            printf("\n");
        }
    } else if (my_rank == NUMBER_OF_PROCESSES - 1) {
        MPI_Recv(bottom_buffer, DIMENSIONS, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    /* - MPI_Scatterv must be supplied the number of elements to send to each process, as Scatterv allows for a varying
     * number of elements.*/
    /* - MPI_Scatterv must also be supplied with an array of displacements from the start of the send buffer for each
     * of the processes*/

    set_scatterv_values(DIMENSIONS, INNER_DIMENSIONS, NUMBER_OF_PROCESSES, send_counts, displacements);

    /* - MPI_Scatterv must be supplied a receive buffer for each process (including the root).*/

    double receive_buffer[send_counts[my_rank]];

    MPI_Scatterv(
            send_buffer,
            send_counts,
            displacements,
            MPI_DOUBLE,
            receive_buffer,
            send_counts[my_rank],
            MPI_DOUBLE,
            0,
            MPI_COMM_WORLD
    );

    int PROCESSOR_ROWS = (send_counts[my_rank] / DIMENSIONS) + 2;
    int PROCESSOR_COLUMNS = DIMENSIONS;

    double processor_array[PROCESSOR_ROWS][PROCESSOR_COLUMNS];

    if (NUMBER_OF_PROCESSES == 1) {
        memcpy(processor_array[0], top_buffer, sizeof(double) * DIMENSIONS);
        memcpy(processor_array[PROCESSOR_ROWS - 1], bottom_buffer, sizeof(double) * DIMENSIONS);
        for (int i = 1; i < PROCESSOR_ROWS - 1; i++)
            memcpy(processor_array[i], receive_buffer + ((i - 1) * DIMENSIONS), sizeof(double) * DIMENSIONS);

    } else {
        for (int i = 1; i < PROCESSOR_ROWS - 1; i++)
            memcpy(processor_array[i], receive_buffer + ((i - 1) * DIMENSIONS), sizeof(double) * DIMENSIONS);

        if (my_rank == 0) {
            memcpy(processor_array[0], top_buffer, sizeof(double) * DIMENSIONS);
            MPI_Recv(processor_array[PROCESSOR_ROWS - 1], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            MPI_Ssend(processor_array[PROCESSOR_ROWS - 2], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD);
        } else if (my_rank == NUMBER_OF_PROCESSES - 1) {
            memcpy(processor_array[PROCESSOR_ROWS - 1], bottom_buffer, sizeof(double) * DIMENSIONS);
            MPI_Ssend(processor_array[1], DIMENSIONS, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD);
            MPI_Recv(processor_array[0], DIMENSIONS, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        } else {
            printf("%d\n", my_rank - 1);
            MPI_Ssend(processor_array[1], DIMENSIONS, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD);
            MPI_Recv(processor_array[PROCESSOR_ROWS - 1], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            MPI_Ssend(processor_array[PROCESSOR_ROWS - 2], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD);
            MPI_Recv(processor_array[0], DIMENSIONS, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }
    }



    printf("%d for rank: %d\n", send_counts[my_rank] / DIMENSIONS, my_rank);

    /*for (int i = 0; i < send_counts[my_rank] / DIMENSIONS; i++)
        memcpy(processor_array[i + 1], &receive_buffer[(i) * DIMENSIONS], sizeof(double) * DIMENSIONS);

    // TODO: For Rank 0 only. For the top rows of each processor, for the bottom too.
    if (my_rank == NUMBER_OF_PROCESSES - 1)
        memcpy(processor_array[PROCESSOR_ROWS - 1], bottom_buffer, sizeof(double) * DIMENSIONS);

    if (my_rank == 0) {
        memcpy(processor_array[0], top_buffer, sizeof(double) * DIMENSIONS);
        MPI_Recv(processor_array[PROCESSOR_ROWS - 1], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(processor_array[0], DIMENSIONS, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (my_rank + 1 < NUMBER_OF_PROCESSES)
        MPI_Ssend(processor_array[PROCESSOR_ROWS - 2], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD);

    if (my_rank > 0) {
        MPI_Ssend(processor_array[1], DIMENSIONS, MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD);
    }*/



    //printf("Array: %d\n", my_rank);
    print_2d_array(PROCESSOR_ROWS, PROCESSOR_COLUMNS, processor_array);
    printf("\n");
    //printf("---\n");

    if (v_test_flag) {
        /* Print received buffers on all processes. */
        //printf("Receive buffer of process %d: ", my_rank);
        //for (int i = 0; i < send_counts[my_rank]; i++) printf("%f\t", receive_buffer[i]);
        //printf("\n");
    }

    if (my_rank == 0) free(send_buffer);

    MPI_Finalize();

    return 0;
}


void set_scatterv_values(const int dimensions, const int inner_dimensions, const int processes, int *send_counts,
                         int *displacements) {
    int ROWS_PER_PROCESS = inner_dimensions / processes;
    int REMAINING_ROWS = inner_dimensions % processes;

    int PROCESS_ELEMENTS = 0, DISPLACEMENT = 0;
    for (int i = 0; i < processes; i++) {
        if (REMAINING_ROWS != 0) {
            PROCESS_ELEMENTS = dimensions * (ROWS_PER_PROCESS + 1);
            REMAINING_ROWS--;
        } else PROCESS_ELEMENTS = dimensions * ROWS_PER_PROCESS;
        send_counts[i] = PROCESS_ELEMENTS;
        displacements[i] = DISPLACEMENT;
        DISPLACEMENT += PROCESS_ELEMENTS;
    }
}

void
setup_args(int *dimensions, double *precision, int *number_of_processes, int *process_rank, int *argc, char ***argv,
           bool *d_test_flag, bool *p_test_flag, bool *v_test_flag) {
    int opt;
    while ((opt = getopt(*argc, *argv, "d:p:v")) != -1) {
        switch (opt) {
            case 'd':
                *dimensions = (int) strtol(optarg, NULL, 10);
                *d_test_flag = true;
                break;
            case 'p':
                *precision = strtod(optarg, NULL);
                *p_test_flag = true;
                break;
            case 'v':
                *v_test_flag = true;
                break;
            default:
                printf("Unrecognised Args. Please use the format %s -d [DIMENSIONS] -p [PRECISION] -v\n", *argv[0]);
                exit(0);
        }
    }

    MPI_Comm_size(MPI_COMM_WORLD, number_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, process_rank);

}

double **array_to_matrix(double *array, int dimensions) {
    double **matrix = malloc(sizeof(double) * dimensions);

    for (int i = 0; i < dimensions; i++) {
        matrix[i] = &array[i * dimensions];
    }
    return matrix;
}

void print_1D_array(double *array, int row, int column) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < column; j++) {
            printf("%f\t", array[i * column + j]);
        }
        printf("\n");
    }
    printf("\n");
}

double *matrix_to_array(double **array, int dimensions) {
    double *send_buffer = malloc((sizeof(double) * dimensions * dimensions) + 1);
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            send_buffer[i * dimensions + j] = array[i][j];
        }
    }
    return send_buffer;
}

void free_array(double **array, int dimensions) {
    for (size_t i = 0; i < dimensions; i++) {
        free(array[i]);
    }
    free(array);
}

size_t average_array(double **INITIAL_ARRAY, int DIMENSIONS, double PRECISION) {
    /* This is where the program needs to be parallelized. */

    bool precision_flag = false;
    int loop_count = 0;
    while (!precision_flag) {
        loop_count++;
        precision_flag = true;

        for (size_t i = 1; i < DIMENSIONS - 1; i++) {
            for (size_t j = 1; j < DIMENSIONS - 1; j++) {
                double previous_value = INITIAL_ARRAY[i][j];
                average(&INITIAL_ARRAY[i][j], &INITIAL_ARRAY[i + 1][j], &INITIAL_ARRAY[i - 1][j],
                        &INITIAL_ARRAY[i][j + 1], &INITIAL_ARRAY[i][j - 1]);

                if (precision_flag == true)
                    precision_flag = in_precision(precision_flag, previous_value, INITIAL_ARRAY[i][j], PRECISION);

            }
        }
    }
    return loop_count;
}

bool in_precision(bool current_precision, double previous_value, double current_value, double precision) {
    if ((int) (previous_value / precision) == (int) (current_value / precision)) return current_precision;
    else return false;
}

void average(double *answer, const double *up, const double *down, const double *right, const double *left) {
    *answer = (*up + *down + *right + *left) / 4;
}

void print_2d_array(int rows, int columns, double array[rows][columns]) {
    int process_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    for (size_t i = 0; i < rows; i++) {
        printf("%d[%ld]: \t", process_rank, i);
        for (size_t j = 0; j < columns; j++) {
            printf("%f\t", array[i][j]);
        }
        printf("\n");
    }
}

/* For the purposes of the initial test array, random numbers will not be used, but instead a pre generated array
 * that's 5x5*/

void generate_non_random_array(int DIMENSIONS, double array[][DIMENSIONS]) {
    array[0][0] = 8.404724;
    array[0][1] = 7.569421;
    array[0][2] = 2.292463;
    array[0][3] = 9.467519;
    array[0][4] = 4.212304;
    array[1][0] = 5.116189;
    array[1][1] = 9.763912;
    array[1][2] = 1.046068;
    array[1][3] = 8.074366;
    array[1][4] = 7.547208;
    array[2][0] = 2.94646;
    array[2][1] = 8.876242;
    array[2][2] = 8.991023;
    array[2][3] = 8.288983;
    array[2][4] = 6.927029;
    array[3][0] = 9.551567;
    array[3][1] = 7.468525;
    array[3][2] = 7.99688;
    array[3][3] = 9.027695;
    array[3][4] = 5.143387;
    array[4][0] = 3.751949;
    array[4][1] = 5.633357;
    array[4][2] = 7.095444;
    array[4][3] = 9.030868;
    array[4][4] = 6.725292;
}


void generate_random_array(int DIMENSIONS, double array[][DIMENSIONS]) {
    srand(time(NULL));
    double random_value;
    for (int i = 0; i < DIMENSIONS; i++) {
        for (int j = 0; j < DIMENSIONS; j++) {
            random_value = (double) rand() / RAND_MAX * 10;
            array[i][j] = random_value;
        }
    }
}


double **generate_2d_double_array(int x_dimension, int y_dimension) {
    double **array = malloc(sizeof(double *) * x_dimension);
    for (size_t i = 0; i < x_dimension; i++) { array[i] = malloc(sizeof(double) * x_dimension); }
    return array;
}