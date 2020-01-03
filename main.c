#include "main.h"

int main(int argc, char *argv[]) {

    /* Default environment variables initialized in case not input from command line arguments. */
    int DIMENSIONS = 5;
    double PRECISION = 0.01;
    int *send_counts;
    int *displacements;
    int NUMBER_OF_PROCESSES, my_rank;

    /* Command line flag checks initialized. */
    bool d_test_flag = false, p_test_flag = false, v_test_flag = false;

    int opt;
    while ((opt = getopt(argc, argv, "d:p:v")) != -1) {
        switch (opt) {
            case 'd':
                DIMENSIONS = (int) strtol(optarg, NULL, 10);
                d_test_flag = true;
                break;
            case 'p':
                PRECISION = strtod(optarg, NULL);
                p_test_flag = true;
                break;
            case 'v':
                v_test_flag = true;
                break;
            default:
                printf("Unrecognised Args. Please use the format %s -d [DIMENSIONS] -p [PRECISION] -v\n", argv[0]);
                exit(0);
        }
    }

    /* MPI setup. */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &NUMBER_OF_PROCESSES);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);


    /* Variables declared for the purposes of sharing and calculating the result from the root process. */
    double *send_buffer = NULL;
    send_counts = malloc(sizeof(int) * NUMBER_OF_PROCESSES);
    displacements = malloc(sizeof(int) * NUMBER_OF_PROCESSES);

    /* Variables declared for the purposes of sending from the root process. */
    int ROWS_PER_PROCESS = DIMENSIONS / NUMBER_OF_PROCESSES;
    int REMAINING_ROWS = DIMENSIONS % NUMBER_OF_PROCESSES;

    /* Argument setup ends. */

    if (my_rank == 0) {
        /* For the purposes of this coursework, 2d array is generated on the first process before data is passed to other
        * processes. If not supplied a dimension, non-random 5x5 is generated. If supplied a dimension, random array
         * of that dimension is generated.*/

        double **INITIAL_ARRAY;
        if (!d_test_flag) INITIAL_ARRAY = generate_non_random_array();
        else INITIAL_ARRAY = generate_random_array(DIMENSIONS);

        /* From this point onwards is the parallelism of the program. */

        /* In order to use the MPI_Scatterv function:
         * - 2d double array must be converted into 1d for the send buffer. This is only relevant on the root process*/

        send_buffer = matrix_to_array(INITIAL_ARRAY, DIMENSIONS);
        if (v_test_flag) {
            printf("Split up array by dimensions. (Dimensions supplied: %d)\n", DIMENSIONS);
            print_1D_array(send_buffer, DIMENSIONS);
        }
    }

    /* - MPI_Scatterv must be supplied the number of elements to send to each process, as Scatterv allows for a varying
     * number of elements.*/
    /* - MPI_Scatterv must also be supplied with an array of displacements from the start of the send buffer for each
     * of the processes*/


    int PROCESS_ELEMENTS;
    int DISPLACEMENT = 0;

    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        if (REMAINING_ROWS != 0) {
            PROCESS_ELEMENTS = DIMENSIONS * (ROWS_PER_PROCESS + 1);
            REMAINING_ROWS--;
        } else PROCESS_ELEMENTS = DIMENSIONS * ROWS_PER_PROCESS;
        send_counts[i] = PROCESS_ELEMENTS;
        displacements[i] = DISPLACEMENT;
        DISPLACEMENT += PROCESS_ELEMENTS;
    }


    /* - MPI_Scatterv must be supplied a receive buffer for each process (including the root).*/

    double *receive_buffer = malloc(sizeof(double) * send_counts[my_rank]);


    MPI_Scatterv(
            &send_buffer[0],
            send_counts,
            displacements,
            MPI_DOUBLE,
            &receive_buffer[0],
            send_counts[my_rank],
            MPI_DOUBLE,
            0,
            MPI_COMM_WORLD
    );



    /* Print received buffers on all processes. */

    if(v_test_flag){
        printf("%d: ", my_rank);
        for(int i = 0; i < send_counts[my_rank]; i++){
            printf("%f\t", receive_buffer[i]);
        }
        printf("\n");
    }



    if (my_rank == 0) free(send_buffer);
    free(displacements);
    free(receive_buffer);

    MPI_Finalize();

    return 0;
}

void print_1D_array(double *array, int dimensions) {
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            printf("%f\t", array[i * dimensions + j]);
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

void print_2d_array(double **array, int dimensions) {
    for (size_t i = 0; i < dimensions; i++) {
        for (size_t j = 0; j < dimensions; j++) {
            printf("%f\t", array[i][j]);
        }
        printf("\n");
    }
}

/* For the purposes of the initial test array, random numbers will not be used, but instead a pre generated array
 * that's 5x5*/

double **generate_non_random_array() {
    double **array = generate_2d_double_array(5);
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
    return array;
}


double **generate_random_array(int DIMENSIONS) {
    srand(time(NULL));
    double **array = generate_2d_double_array(DIMENSIONS);
    double random_value;
    for (int i = 0; i < DIMENSIONS; i++) {
        for (int j = 0; j < DIMENSIONS; j++) {
            random_value = (double) rand() / RAND_MAX * 10;
            array[i][j] = random_value;
        }
    }
    return array;
}


double **generate_2d_double_array(int dimensions) {
    double **array = malloc(sizeof(double *) * dimensions);
    for (size_t i = 0; i < dimensions; i++) { array[i] = malloc(sizeof(double) * dimensions); }
    return array;
}