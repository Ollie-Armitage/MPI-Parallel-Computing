#include <zconf.h>
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
        send_buffer = malloc(sizeof(double) * DIMENSIONS * DIMENSIONS);
        /* For the purposes of this coursework, 2d array is generated on the first process before data is passed to other
        * processes. If not supplied a dimension, non-random 5x5 is generated. If supplied a dimension, random array
         * of that dimension is generated.*/
        double INITIAL_ARRAY[DIMENSIONS][DIMENSIONS];
        if (!d_test_flag) generate_non_random_array(DIMENSIONS, INITIAL_ARRAY);
        else generate_random_array(DIMENSIONS, INITIAL_ARRAY);

        /* From this point onwards is the parallelism of the program. */

        /* In order to use the MPI_Scatterv function:
         * - 2d double array must be converted into 1d for the send buffer. This is only relevant on the root process*/

        memcpy(top_buffer, INITIAL_ARRAY[0], sizeof(double) * DIMENSIONS);


        for (int i = 0; i < INNER_DIMENSIONS; i++) {
            memcpy(&send_buffer[i * DIMENSIONS], INITIAL_ARRAY[i + 1], sizeof(double) * DIMENSIONS);
        }

        if (NUMBER_OF_PROCESSES > 1)
            MPI_Ssend(INITIAL_ARRAY[DIMENSIONS - 1], DIMENSIONS, MPI_DOUBLE, NUMBER_OF_PROCESSES - 1, 0,
                      MPI_COMM_WORLD);
        memcpy(bottom_buffer, INITIAL_ARRAY[DIMENSIONS - 1], sizeof(double) * DIMENSIONS);

        for (int i = 0; i < DIMENSIONS; i++) printf("%f\t", INITIAL_ARRAY[DIMENSIONS - 1][i]);

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

    bool IN_PRECISION_FLAG = false;

    if (NUMBER_OF_PROCESSES == 1) {
        memcpy(processor_array[0], top_buffer, sizeof(double) * DIMENSIONS);
        memcpy(processor_array[PROCESSOR_ROWS - 1], bottom_buffer, sizeof(double) * DIMENSIONS);
        for (int i = 1; i < PROCESSOR_ROWS - 1; i++)
            memcpy(processor_array[i], receive_buffer + ((i - 1) * DIMENSIONS), sizeof(double) * DIMENSIONS);

    } else {
        for (int i = 1; i < PROCESSOR_ROWS - 1; i++)
            memcpy(processor_array[i], receive_buffer + ((i - 1) * DIMENSIONS), sizeof(double) * DIMENSIONS);
    }


    // While ANY are not in precision.

    while (!IN_PRECISION_FLAG) {
        IN_PRECISION_FLAG = true;
        //sleep(1);

        print_2d_array(PROCESSOR_ROWS, PROCESSOR_COLUMNS, processor_array);
        printf("\n");

        int process_communication = MPI_PROC_NULL;

        if (NUMBER_OF_PROCESSES > 1) {
            if (my_rank == 0) {
                memcpy(processor_array[0], top_buffer, sizeof(double) * DIMENSIONS); // Necessary? Don't think so

                // If my_rank + 1 has not exited.

                MPI_Sendrecv(processor_array[PROCESSOR_ROWS - 2], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0,
                             processor_array[PROCESSOR_ROWS - 1], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0,
                             MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);

            } else if (my_rank == NUMBER_OF_PROCESSES - 1) {
                memcpy(processor_array[PROCESSOR_ROWS - 1], bottom_buffer,
                       sizeof(double) * DIMENSIONS); // Don't think it's necessary

                // If my_rank - 1 has not exited.
                MPI_Sendrecv(processor_array[1], DIMENSIONS, MPI_DOUBLE, my_rank - 1, 0, processor_array[0], DIMENSIONS,
                             MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);
            } else {

                // If my_rank - 1 has not exited.

                MPI_Sendrecv(processor_array[1], DIMENSIONS, MPI_DOUBLE, my_rank - 1, 0, processor_array[0], DIMENSIONS,
                             MPI_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);

                // If my_rank + 1 has not exited.

                MPI_Sendrecv(processor_array[PROCESSOR_ROWS - 2], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0,
                             processor_array[PROCESSOR_ROWS - 1], DIMENSIONS, MPI_DOUBLE, my_rank + 1, 0,
                             MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);
            }
        }


        for (size_t i = 1; i < PROCESSOR_ROWS - 1; i++) {
            for (size_t j = 1; j < PROCESSOR_COLUMNS - 1; j++) {
                double previous_value = processor_array[i][j];
                average(&processor_array[i][j], &processor_array[i + 1][j], &processor_array[i - 1][j],
                        &processor_array[i][j + 1], &processor_array[i][j - 1]);

                if (IN_PRECISION_FLAG == true)
                    IN_PRECISION_FLAG = in_precision(previous_value, processor_array[i][j], PRECISION);

            }
        }

        // Send precision to rank 0
        // receive precision from rank 0



    }


    for (int i = 0; i < PROCESSOR_ROWS - 2; i++) {
        memcpy(receive_buffer + (i * DIMENSIONS), processor_array[i + 1], sizeof(double) * DIMENSIONS);
    }


    MPI_Gatherv(receive_buffer, send_counts[my_rank], MPI_DOUBLE, send_buffer + DIMENSIONS, send_counts,
                displacements, MPI_DOUBLE,
                0, MPI_COMM_WORLD);


    if (my_rank == 0) {

        memcpy(send_buffer, top_buffer, sizeof(double) * DIMENSIONS);
        memcpy(send_buffer + (DIMENSIONS * (DIMENSIONS - 1)), bottom_buffer, sizeof(double) * DIMENSIONS);

        if (v_test_flag) {
            for (int i = 0; i < DIMENSIONS; i++) {
                for (int j = 0; j < DIMENSIONS; j++) {
                    printf("%f\t", send_buffer[(i * DIMENSIONS) + j]);
                }
                printf("\n");
            }

        }
    }


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
                printf("Unrecognised Args. Please use the format %s -d [DIMENSIONS] -p [PRECISION] -v [VERBOSITY]\n",
                       *argv[0]);
                exit(0);
        }
    }

    MPI_Comm_size(MPI_COMM_WORLD, number_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, process_rank);

    if (*number_of_processes > *dimensions - 2) *number_of_processes = *dimensions - 2;
    if (*process_rank > *number_of_processes - 1) MPI_Finalize();

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

void free_array(double **array, int dimensions) {
    for (size_t i = 0; i < dimensions; i++) {
        free(array[i]);
    }
    free(array);
}

size_t
average_array(int rows, int columns, double INITIAL_ARRAY[rows][columns], double PRECISION, bool *precision_flag) {

    int loop_count = 0;

    loop_count++;

    for (size_t i = 1; i < rows - 1; i++) {
        for (size_t j = 1; j < columns - 1; j++) {
            double previous_value = INITIAL_ARRAY[i][j];
            average(&INITIAL_ARRAY[i][j], &INITIAL_ARRAY[i + 1][j], &INITIAL_ARRAY[i - 1][j],
                    &INITIAL_ARRAY[i][j + 1], &INITIAL_ARRAY[i][j - 1]);

            if (*precision_flag == true)
                *precision_flag = in_precision(previous_value, INITIAL_ARRAY[i][j], PRECISION);

        }
    }

    return loop_count;
}

bool in_precision(double previous_value, double current_value, double precision) {
    if ((int) (previous_value / precision) == (int) (current_value / precision)) return true;
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