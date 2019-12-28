#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

double **generate_2d_array(int dimensions);

double **generate_non_random_array();

void print_2d_array(double **array, size_t dimensions);

int IN_PRECISION(double previous_value, double current_value, double precision);

void average(double *answer, const double *up, const double *down, const double *right, const double *left);

int main(int argc, char *argv[]) {
    size_t DIMENSIONS = 0;
    double PRECISION = 0;
    size_t d_test_flag = 1;
    size_t p_test_flag = 1;

    int opt;
    while ((opt = getopt(argc, argv, "d:p:")) != -1) {
        switch (opt) {
            case 'd':
                DIMENSIONS = strtoul(optarg, NULL, 10);
                d_test_flag = 0;
                break;
            case 'p':
                PRECISION = strtod(optarg, NULL);
                p_test_flag = 0;
                break;
            default:
                printf("Unrecognised Args. Please use the format %s -d [DIMENSIONS] -p [PRECISION]\n", argv[0]);
                exit(0);
        }
    }

    double **INITIAL_ARRAY;

    if (d_test_flag == 1) {
        DIMENSIONS = 5;
        INITIAL_ARRAY = generate_non_random_array();
    } else {
        INITIAL_ARRAY = generate_2d_array(DIMENSIONS);
    }

    if (p_test_flag == 1) { PRECISION = 0.00000001; }


    int precision_flag = 0;

    int loop_count = 0;

    while (!precision_flag) {
        loop_count++;
        print_2d_array(INITIAL_ARRAY, DIMENSIONS);
        printf("\n");

        precision_flag = 0;

        for (size_t i = 1; i < DIMENSIONS - 1; i++) {
            for (size_t j = 1; j < DIMENSIONS - 1; j++) {
                double previous_value = INITIAL_ARRAY[i][j];
                average(&INITIAL_ARRAY[i][j], &INITIAL_ARRAY[i + 1][j], &INITIAL_ARRAY[i - 1][j],
                        &INITIAL_ARRAY[i][j + 1], &INITIAL_ARRAY[i][j - 1]);
                precision_flag = IN_PRECISION(previous_value, INITIAL_ARRAY[i][j], PRECISION);
            }
        }
    }

    print_2d_array(INITIAL_ARRAY, DIMENSIONS);
    printf("\nFinished in %d runs.\n", loop_count);

    return 0;
}

int IN_PRECISION(double previous_value, double current_value, double precision) {


    if ((int)(previous_value / precision) == (int)(current_value / precision)) return 1;
    else return 0;
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
    double **array = generate_2d_array(5);

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

double **generate_2d_array(int dimensions) {
    double **array = malloc(sizeof(double *) * dimensions);
    for (size_t i = 0; i < dimensions; i++) { array[i] = malloc(sizeof(double) * dimensions); }
    return array;
}