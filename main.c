#include "main.h"

int main(int argc, char *argv[]) {
    /* Argument setup. */

    size_t DIMENSIONS = 0;
    double PRECISION = 0;
    bool d_test_flag = false;
    bool p_test_flag = false;

    int opt;
    while ((opt = getopt(argc, argv, "d:p:")) != -1) {
        switch (opt) {
            case 'd':
                DIMENSIONS = strtoul(optarg, NULL, 10);
                d_test_flag = true;
                break;
            case 'p':
                PRECISION = strtod(optarg, NULL);
                p_test_flag = true;
                break;
            default:
                printf("Unrecognised Args. Please use the format %s -d [DIMENSIONS] -p [PRECISION]\n", argv[0]);
                exit(0);
        }
    }


    double **INITIAL_ARRAY;

    if (!d_test_flag) {
        DIMENSIONS = 5;
        INITIAL_ARRAY = generate_non_random_array();
    } else { INITIAL_ARRAY = generate_random_array(DIMENSIONS); }

    printf("Initial Array: \n");
    print_2d_array(INITIAL_ARRAY, DIMENSIONS);
    printf("\n");

    if (!p_test_flag) { PRECISION = 0.0001; }


    /* Argument setup ends. */


    MPI_Init(&argc, &argv);

    /* Function averages array and returns the number of precision loops performed. */
    size_t loop_count = average_array(INITIAL_ARRAY, DIMENSIONS, PRECISION);

    print_2d_array(INITIAL_ARRAY, DIMENSIONS);
    printf("\nFinished in %zu runs\n", loop_count);
    free_array(INITIAL_ARRAY, DIMENSIONS);

    MPI_Finalize();

    return 0;
}

void free_array(double **array, size_t dimensions) {
    for (size_t i = 0; i < dimensions; i++) {
        free(array[i]);
    }
    free(array);
}

size_t average_array(double **INITIAL_ARRAY, size_t DIMENSIONS, double PRECISION) {
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
    //TODO: Talk to robin about this.

    if ((int) (previous_value / precision) == (int) (current_value / precision)) return current_precision;
    else return false;
}

void average(double *answer, const double *up, const double *down, const double *right, const double *left) {
    *answer = (*up + *down + *right + *left) / 4;
}

void print_2d_array(double **array, size_t dimensions) {
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


double **generate_random_array(size_t DIMENSIONS) {
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


double **generate_2d_double_array(size_t dimensions) {
    double **array = malloc(sizeof(double *) * dimensions);
    for (size_t i = 0; i < dimensions; i++) { array[i] = malloc(sizeof(double) * dimensions); }
    return array;
}