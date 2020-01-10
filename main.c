#include <zconf.h>
#include "main.h"

void interprocess_sends(double **processor_array, int send_count, int my_rank, bool IN_PRECISION_FLAG,
                        int number_of_processes, int processor_rows);

void average_array(int rows, int columns, double **processor_array, bool *IN_PRECISION_FLAG, double PRECISION);

int main(int argc, char *argv[]) {

    /* Default environment variables initialized in case not input from command line arguments. */
    int DIMENSIONS = 5, NUMBER_OF_PROCESSES, my_rank;
    double PRECISION = 0.01;


    double **processor_array;
    int PROCESSOR_ROWS, PROCESSOR_COLUMNS;


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

    if(NUMBER_OF_PROCESSES > DIMENSIONS - 2){
        if(my_rank == 0) printf("Too many processes supplied! Please reduce the number of processes to below the input dimensions - 2. \n");
        MPI_Finalize();
        exit(1);
    }

    /* Variables declared for the purposes of scattering the values from the root process. */
    int send_counts[NUMBER_OF_PROCESSES], displacements[NUMBER_OF_PROCESSES];
    double *send_buffer = NULL;

    /* Hard set arrays for top and bottom rows of each processor. */
    double top_processor_row[DIMENSIONS], bottom_processor_row[DIMENSIONS];

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

        memcpy(top_processor_row, INITIAL_ARRAY[0], sizeof(double) * DIMENSIONS);
        memcpy(bottom_processor_row, INITIAL_ARRAY[DIMENSIONS - 1], sizeof(double) * DIMENSIONS);

        send_buffer = malloc(sizeof(double) * INNER_DIMENSIONS * DIMENSIONS);
        for (int i = 0; i < INNER_DIMENSIONS; i++) {
            memcpy(&send_buffer[i * DIMENSIONS], INITIAL_ARRAY[i + 1], sizeof(double) * DIMENSIONS);
        }

        if (NUMBER_OF_PROCESSES > 1) {
            MPI_Send(INITIAL_ARRAY[DIMENSIONS - 1], DIMENSIONS, MPI_DOUBLE, NUMBER_OF_PROCESSES - 1, 0,
                     MPI_COMM_WORLD);
        }

        print_1D_array(INITIAL_ARRAY[0], 1, DIMENSIONS);
        print_1D_array(send_buffer, INNER_DIMENSIONS, DIMENSIONS);
        print_1D_array(INITIAL_ARRAY[DIMENSIONS - 1], 1, DIMENSIONS);

        printf("\n");

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


    if (my_rank == NUMBER_OF_PROCESSES - 1 && NUMBER_OF_PROCESSES > 1)
        MPI_Recv(bottom_processor_row, DIMENSIONS, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);


    PROCESSOR_ROWS = (send_counts[my_rank] / DIMENSIONS) + 2;
    PROCESSOR_COLUMNS = DIMENSIONS;

    processor_array = build_processor_array(PROCESSOR_ROWS, PROCESSOR_COLUMNS, receive_buffer, top_processor_row,
                                            bottom_processor_row, my_rank, NUMBER_OF_PROCESSES);
    MPI_Status send_status_top, send_status_bottom;

    bool top_lock = false, bottom_lock = false, first_run_flag = true, IN_PRECISION_FLAG = false;
    while (!IN_PRECISION_FLAG) {

        if (first_run_flag && NUMBER_OF_PROCESSES > 1)
            interprocess_sends(processor_array, DIMENSIONS, my_rank, IN_PRECISION_FLAG, NUMBER_OF_PROCESSES,
                               PROCESSOR_ROWS);
        else first_run_flag = false;

        IN_PRECISION_FLAG = true;

        if (NUMBER_OF_PROCESSES > 1) {

            if (!top_lock && my_rank != 0) {
                MPI_Probe(my_rank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &send_status_top);

                int tag = 0;
                if (send_status_top.MPI_TAG == 1) {
                    top_lock = true;
                    tag = 1;
                }

                MPI_Recv(processor_array[0], DIMENSIONS, MPI_DOUBLE, my_rank - 1, tag,
                         MPI_COMM_WORLD, &send_status_top);
            }

            if (!bottom_lock && my_rank != NUMBER_OF_PROCESSES - 1) {
                MPI_Probe(my_rank + 1, MPI_ANY_TAG, MPI_COMM_WORLD, &send_status_bottom);
                int tag = 0;
                if (send_status_bottom.MPI_TAG == 1) {
                    bottom_lock = true;
                    tag = 1;
                }
                MPI_Recv(processor_array[PROCESSOR_ROWS - 1], DIMENSIONS, MPI_DOUBLE, my_rank + 1, tag,
                         MPI_COMM_WORLD, &send_status_bottom);

            }


            if (v_test_flag) {
                //sleep(1);
                print_2d_array(PROCESSOR_ROWS, PROCESSOR_COLUMNS, processor_array);
                printf("\n");

            }

            interprocess_sends(processor_array, DIMENSIONS, my_rank, IN_PRECISION_FLAG, NUMBER_OF_PROCESSES,
                               PROCESSOR_ROWS);

        }
        average_array(PROCESSOR_ROWS, PROCESSOR_COLUMNS, processor_array, &IN_PRECISION_FLAG, PRECISION);
    }

    MPI_Gatherv(receive_buffer, send_counts[my_rank], MPI_DOUBLE, send_buffer, send_counts,
                displacements, MPI_DOUBLE,
                0, MPI_COMM_WORLD);


    if (my_rank == 0) {
        double **END_ARRAY = malloc(sizeof(double) * DIMENSIONS);

        END_ARRAY[0] = top_processor_row;

        for (int i = 1; i < DIMENSIONS - 1; i++) {
            END_ARRAY[i] = send_buffer + ((i - 1) * DIMENSIONS);
        }

        END_ARRAY[DIMENSIONS - 1] = bottom_processor_row;

        if (my_rank == 0) {
            for (int i = 0; i < DIMENSIONS; i++) {
                for (int j = 0; j < DIMENSIONS; j++) {
                    printf("%f\t", END_ARRAY[i][j]);
                }
                printf("\n");
            }
        }
    }


    MPI_Finalize();
    return 0;
}

void average_array(int rows, int columns, double **processor_array, bool *IN_PRECISION_FLAG, double PRECISION) {
    for (size_t i = 1; i < rows - 1; i++) {
        for (size_t j = 1; j < columns - 1; j++) {
            double previous_value = processor_array[i][j];
            average(&processor_array[i][j], &processor_array[i + 1][j], &processor_array[i - 1][j],
                    &processor_array[i][j + 1], &processor_array[i][j - 1]);

            if (*IN_PRECISION_FLAG == true)
                *IN_PRECISION_FLAG = in_precision(previous_value, processor_array[i][j], PRECISION);

        }
    }
}

void interprocess_sends(double **processor_array, int send_count, int my_rank, bool IN_PRECISION_FLAG,
                        int number_of_processes, int processor_rows) {
    int tag = 0;
    if (IN_PRECISION_FLAG == true) tag = 1;

    if (my_rank == 0) {
        MPI_Send(processor_array[processor_rows - 2], send_count, MPI_DOUBLE, my_rank + 1, tag, MPI_COMM_WORLD);
        return;
    }
    if (my_rank == number_of_processes - 1) {
        MPI_Send(processor_array[1], send_count, MPI_DOUBLE, my_rank - 1, tag, MPI_COMM_WORLD);
        return;
    }

    MPI_Send(processor_array[1], send_count, MPI_DOUBLE, my_rank - 1, tag, MPI_COMM_WORLD);
    MPI_Send(processor_array[processor_rows - 2], send_count, MPI_DOUBLE, my_rank + 1, tag, MPI_COMM_WORLD);

}


double **build_processor_array(int processor_rows, int processor_columns, double *receive_buffer, double *top_row,
                               double *bottom_row, int my_rank, int number_of_processes) {


    double **array = malloc(processor_rows * sizeof(double));

    if (my_rank == 0) array[0] = top_row;
    else array[0] = calloc(processor_columns, sizeof(double));

    for (int i = 1; i < processor_rows; i++) {
        array[i] = receive_buffer + ((i - 1) * processor_columns);
    }


    if (my_rank == number_of_processes - 1) array[processor_rows - 1] = bottom_row;
    else array[processor_rows - 1] = calloc(processor_columns, sizeof(double));

    return array;
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


}

bool in_precision(double previous_value, double current_value, double precision) {
    if ((int) (previous_value / precision) == (int) (current_value / precision)) return true;
    else return false;
}

void average(double *answer, const double *up, const double *down, const double *right, const double *left) {
    *answer = (*up + *down + *right + *left) / 4;
}

void print_1D_array(double *array, int row, int column) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < column; j++) {
            printf("%f\t", array[i * column + j]);
        }
        printf("\n");
    }
}

void print_2d_array(int rows, int columns, double **array) {
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