#include <zconf.h>
#include "main.h"

void interprocess_sends(double **processor_array, int send_count, int my_rank, bool IN_PRECISION_FLAG,
                        int number_of_processes, int processor_rows, MPI_Request *request_top,
                        MPI_Request *request_bottom, bool first_run);

void average_array(int rows, int columns, double **processor_array, bool *IN_PRECISION_FLAG, double PRECISION);

int main(int argc, char *argv[]) {

    /* Default environment variables initialized in case not input from command line arguments. */
    int DIMENSIONS = 5, NUMBER_OF_PROCESSES, my_rank;
    double PRECISION = 0.01;

    /* Each processor has its own memory, which includes its array portion. */
    double **processor_array;
    int PROCESSOR_ROWS, PROCESSOR_COLUMNS;


    /* Command line flag checks initialized. */
    bool d_test_flag = false, p_test_flag = false, v_test_flag = false;

    /* MPI setup. */
    MPI_Init(&argc, &argv);

    /* Setup args using getopt(). */
    setup_args(&DIMENSIONS, &PRECISION, &NUMBER_OF_PROCESSES, &my_rank, &argc, &argv, &d_test_flag, &p_test_flag,
               &v_test_flag);
    int INNER_DIMENSIONS = DIMENSIONS - 2;

    if (DIMENSIONS <= 2) {
        if (my_rank == 0) printf("Dimensions supplied invalid. Minimum dimensions of 3x3.\n");
        exit(-1);
    }

    if (NUMBER_OF_PROCESSES > DIMENSIONS - 2) {
        if (my_rank == 0)
            printf("Too many processes requested! Please reduce the number of processors to below the value of the input dimensions - 2. \n");
        MPI_Finalize();
        exit(1);
    }

    /* Variables declared for the purposes of scattering the values from the root process. */
    int send_counts[NUMBER_OF_PROCESSES], displacements[NUMBER_OF_PROCESSES];
    double *send_buffer = NULL;

    /* Hard set arrays for top and bottom rows of each processor. */
    double *top_processor_row = NULL;
    if (my_rank == 0) top_processor_row = malloc(sizeof(double) * DIMENSIONS);
    double *bottom_processor_row = NULL;
    if (my_rank == NUMBER_OF_PROCESSES - 1) bottom_processor_row = malloc(sizeof(double) * DIMENSIONS);


    /* Argument setup ends. */

    if (my_rank == 0) {

        /* For the purposes of this coursework, 2d array is generated on the first process before data is passed to other
        * processes. If not supplied a dimension, non-random 5x5 is generated. If supplied a dimension, random array
         * of that dimension is generated.*/
        double **INITIAL_ARRAY = malloc(sizeof(double) * DIMENSIONS);


        for (int i = 0; i < DIMENSIONS; i++) {
            INITIAL_ARRAY[i] = malloc(sizeof(double) * DIMENSIONS);
        }


        if (!d_test_flag) generate_non_random_array(DIMENSIONS, INITIAL_ARRAY);
        else generate_random_array(DIMENSIONS, INITIAL_ARRAY);

        /* From this point onwards is the parallelism of the program. */

        /* In order to use the MPI_Scatterv function:
         * - 2d double array must be converted into 1d for the send buffer. This is only relevant on the root process*/

        top_processor_row = INITIAL_ARRAY[0];
        bottom_processor_row = INITIAL_ARRAY[DIMENSIONS - 1];


        /* send buffer for scatterv function.*/

        send_buffer = malloc(sizeof(double) * INNER_DIMENSIONS * DIMENSIONS);

        for (int i = 0; i < INNER_DIMENSIONS; i++) {
            memcpy(&send_buffer[i * DIMENSIONS], INITIAL_ARRAY[i + 1], sizeof(double) * DIMENSIONS);
        }

        /* As long as there is more than one processor, send the last of the array to the last processor. */

        if (NUMBER_OF_PROCESSES > 1) {
            MPI_Send(INITIAL_ARRAY[DIMENSIONS - 1], DIMENSIONS, MPI_DOUBLE, NUMBER_OF_PROCESSES - 1, 0,
                     MPI_COMM_WORLD);
        }

        if (v_test_flag) {
            print_1D_array(top_processor_row, 1, DIMENSIONS);
            print_1D_array(send_buffer, INNER_DIMENSIONS, DIMENSIONS);
            print_1D_array(bottom_processor_row, 1, DIMENSIONS);
            printf("\n");
        }


    }

    /* Last processor receives last line.  */
    if (my_rank == NUMBER_OF_PROCESSES - 1 && NUMBER_OF_PROCESSES > 1)
        MPI_Recv(bottom_processor_row, DIMENSIONS, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    /* - MPI_Scatterv must be supplied the number of elements to send to each process, as Scatterv allows for a varying
     * number of elements.
     - MPI_Scatterv must also be supplied with an array of displacements from the start of the send buffer for each
     * of the processes. The set scatterv values function works out both of these arrays considering the dimensions
     * and the number of processes. */

    set_scatterv_values(DIMENSIONS, INNER_DIMENSIONS, NUMBER_OF_PROCESSES, send_counts, displacements);

    /* - MPI_Scatterv must be supplied a receive buffer for each process (including the root).*/
    double receive_buffer[send_counts[my_rank]];


    /* Scatterv sends chunks of the INITIAL_ARRAY to each processor, minus the first and last rows. The first row is
     * pre-stored on process 0. The last row is pre-sent to the last process. All the lines in between are split
     * based on the send counts and the displacement. Scatterv compared with scatter allows for varying sizes by
     * supplying it a send_counts array of differing values rather than a single value, along with their displacements
     * from the original address (which allow for spaces in between the data). */

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


    /* Dimensions of process. +2 for each processor is necessary in order to add an edge for averaging*/
    PROCESSOR_ROWS = (send_counts[my_rank] / DIMENSIONS) + 2;
    PROCESSOR_COLUMNS = DIMENSIONS;


    /* Initial memory allocation for processor. All rows are bound bar the top and bottom rows, which are instead only
     * bound if they are process 0 or the last process. There is however memory allocated for the missing rows. */
    processor_array = build_processor_array(PROCESSOR_ROWS, PROCESSOR_COLUMNS, receive_buffer, top_processor_row,
                                            bottom_processor_row, my_rank, NUMBER_OF_PROCESSES);

    /* Once probed, MPI send_status' store data on the last message sent from processes on above and below the
     * current process. */
    MPI_Status send_status_top, send_status_bottom;

    /* MPI requests act as locks for instant sends. (MPI_Isend).*/
    MPI_Request request_top, request_bottom;

    bool top_lock = false, bottom_lock = false, first_run_flag = true, IN_PRECISION_FLAG = false;

    /* While each array is not in precision for all values, loop averaging. */
    while (!IN_PRECISION_FLAG) {

        /* Run once to send initial rows upwards and downwards. interprocess_sends uses Isends (non-blocking sends) to
         * send rows upwards and downwards, both of which are blocked seperately with MPI_Waits. The waits for the first
         * runs are in the receiving section, future waits are inside interprocess in order to make sure the send buffer
         * isn't being re-used early. */

        if (first_run_flag && NUMBER_OF_PROCESSES > 1) {
            interprocess_sends(processor_array, DIMENSIONS, my_rank, IN_PRECISION_FLAG, NUMBER_OF_PROCESSES,
                               PROCESSOR_ROWS, &request_top, &request_bottom, first_run_flag);
            first_run_flag = false;
        }

        if (NUMBER_OF_PROCESSES > 1) {

            /* Probes the received messages from above and below. If they have a tag of 1, it means that will be the
             * last row that process will send as it has reached the specified precision. If that's the case, the
             * process will lock that row only using the last received one, and not attempting to receive more.*/

            if (!top_lock && my_rank != 0) {
                if (first_run_flag) MPI_Wait(&request_bottom, MPI_STATUS_IGNORE);
                MPI_Probe(my_rank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &send_status_top);


                /* tag value necessary to be stored and receives need to have same tags as sends. */
                int tag = 0;
                if (send_status_top.MPI_TAG == 1) {
                    top_lock = true;
                    tag = 1;
                }

                MPI_Recv(processor_array[0], DIMENSIONS, MPI_DOUBLE, my_rank - 1, tag,
                         MPI_COMM_WORLD, &send_status_top);
            }

            if (!bottom_lock && my_rank != NUMBER_OF_PROCESSES - 1) {
                if (first_run_flag) MPI_Wait(&request_top, MPI_STATUS_IGNORE);
                MPI_Probe(my_rank + 1, MPI_ANY_TAG, MPI_COMM_WORLD, &send_status_bottom);
                int tag = 0;
                if (send_status_bottom.MPI_TAG == 1) {
                    bottom_lock = true;
                    tag = 1;
                }

                MPI_Recv(processor_array[PROCESSOR_ROWS - 1], DIMENSIONS, MPI_DOUBLE, my_rank + 1, tag,
                         MPI_COMM_WORLD, &send_status_bottom);

            }

        }
        /* in precision flag initially set to true. if during averaging the value of the averaged value is outside
         * the precision of the current value, flag gets set to false. */
        IN_PRECISION_FLAG = true;

        /* Takes the entire processor array and, for each element, takes it's surrounding values and averages based
         * on them. */
        average_array(PROCESSOR_ROWS, PROCESSOR_COLUMNS, processor_array, &IN_PRECISION_FLAG, PRECISION);
        /* Once averaging is done, send out new rows to processes above and below.*/
        if (NUMBER_OF_PROCESSES > 1)
            interprocess_sends(processor_array, DIMENSIONS, my_rank, IN_PRECISION_FLAG, NUMBER_OF_PROCESSES,
                               PROCESSOR_ROWS, &request_top, &request_bottom, first_run_flag);
    }


    /* Once all rows have been averaged to the point where they are in precision, gather all rows back to the
     * initial buffer. */
    MPI_Gatherv(receive_buffer, send_counts[my_rank], MPI_DOUBLE, send_buffer, send_counts,
                displacements, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    /* End values stored in end array on rank 0.*/
    if (my_rank == 0) {
        double **END_ARRAY = malloc(sizeof(double) * DIMENSIONS);

        END_ARRAY[0] = top_processor_row;

        for (int i = 1; i < DIMENSIONS - 1; i++) {
            END_ARRAY[i] = send_buffer + ((i - 1) * DIMENSIONS);
        }

        END_ARRAY[DIMENSIONS - 1] = bottom_processor_row;

        if (my_rank == 0 && v_test_flag) {
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

            /* Only repeat precision check if values coming before are true. if not, there's no point as array is already
             * not within precision. */
            if (*IN_PRECISION_FLAG == true)
                *IN_PRECISION_FLAG = in_precision(previous_value, processor_array[i][j], PRECISION);

        }
    }
}

void interprocess_sends(double **processor_array, int send_count, int my_rank, bool IN_PRECISION_FLAG,
                        int number_of_processes, int processor_rows, MPI_Request *request_top,
                        MPI_Request *request_bottom, bool first_run) {

    /* If within precision, mark tag as 1 to signify this being the last send this process will be doing to other
     * processes. */
    int tag = 0;
    if (IN_PRECISION_FLAG == true) tag = 1;

    /* Wait on buffer to make sure it isn't still in use. */
    if (my_rank == 0) {
        if (!first_run) MPI_Wait(request_bottom, MPI_STATUS_IGNORE);
        MPI_Isend(processor_array[processor_rows - 2], send_count, MPI_DOUBLE, my_rank + 1, tag, MPI_COMM_WORLD,
                  request_bottom);
        return;
    }
    if (my_rank == number_of_processes - 1) {
        if (!first_run) MPI_Wait(request_top, MPI_STATUS_IGNORE);
        MPI_Isend(processor_array[1], send_count, MPI_DOUBLE, my_rank - 1, tag, MPI_COMM_WORLD, request_top);
        return;
    }

    if (!first_run) {
        MPI_Wait(request_top, MPI_STATUS_IGNORE);
        MPI_Wait(request_bottom, MPI_STATUS_IGNORE);
    }

    MPI_Isend(processor_array[1], send_count, MPI_DOUBLE, my_rank - 1, tag, MPI_COMM_WORLD, request_top);
    MPI_Isend(processor_array[processor_rows - 2], send_count, MPI_DOUBLE, my_rank + 1, tag, MPI_COMM_WORLD,
              request_bottom);

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

void generate_non_random_array(int DIMENSIONS, double **array) {
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

void generate_random_array(int DIMENSIONS, double **array) {
    srand(time(NULL));
    double random_value;
    for (int i = 0; i < DIMENSIONS; i++) {
        for (int j = 0; j < DIMENSIONS; j++) {
            random_value = (double) rand() / RAND_MAX * 10;
            array[i][j] = random_value;
        }
    }
}