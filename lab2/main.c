#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX_SIZE 20

// Dimensions of a and b matrices and c matrix
int row1, row2, col1, col2, MatA[MAX_SIZE][MAX_SIZE], MatB[MAX_SIZE][MAX_SIZE], MatOut[MAX_SIZE][MAX_SIZE];
struct timeval stop, start;
char *Mat1_file = "a", *Mat2_file = "b", *MatOut_prefix = "c";

struct indexes {
    int i;
    int j;
};

void per_matrix_body();
void per_row_body();
void per_element_body();
int thread_per_matrix();
int thread_per_row();
int thread_per_element();
void reset_mat_out();
void write_mat_to_file(int mat[MAX_SIZE][MAX_SIZE], char *method, int row, int col);
void read_from_file(char file[50], int *row, int *col, int mat[MAX_SIZE][MAX_SIZE]);
void name_file(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    name_file(argc, argv);
    read_from_file(Mat1_file, &row1, &col1, MatA);
    read_from_file(Mat2_file, &row2, &col2, MatB);
    if(row2 != col1) {
        perror("Invalid dimensions");
        exit(1);
    }
    per_matrix_body();
    per_row_body();
    per_element_body();
}

void name_file(int argc, char* argv[]) {
    // Use default filenames if no arguments provided
    if(argc == 1 && argv[0] != NULL) {
        Mat1_file = "a";
        Mat2_file = "b";
        MatOut_prefix = "c";
    }
    if(argc > 1) {
        Mat1_file = argv[1];
    }
    if(argc > 2) {
        Mat2_file = argv[2];
    }
    if(argc > 3) {
        MatOut_prefix = argv[3];
    }
}

void reset_mat_out() {
    for(int i = 0; i < MAX_SIZE; i++) {
        for(int j = 0; j < MAX_SIZE; j++) {
            MatOut[i][j] = 0;
        }
    }
}

void read_from_file(char file[50], int *row, int *col, int matrix[MAX_SIZE][MAX_SIZE]) {
    FILE *fp = fopen(file, "r");
    fscanf(fp, "row=%d col=%d", row, col);
    for(int i = 0; i < *row; i++)
        for(int j = 0; j < *col; j++)
            fscanf(fp, "%d", &matrix[i][j]);
    fclose(fp);
}

void write_mat_to_file(int mat[MAX_SIZE][MAX_SIZE], char *method, int row, int col) {
    char filename[MAX_SIZE];
    sprintf(filename, "%s_per_%s.txt", MatOut_prefix, method);
    // Take matrix entries and write them into the file
    FILE *out = fopen(filename, "w");
    fprintf(out, "Method: A thread per %s.\nrow=%d col=%d\n", method, row, col);
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            fprintf(out, "%d ", mat[i][j]);
        }
        fprintf(out, "\n");
    }
    fclose(out);
}

void per_matrix_body() {
    gettimeofday(&start, NULL); //start checking time
    int no_threads = thread_per_matrix();
    gettimeofday(&stop, NULL); //end checking time

    printf("Number of threads Thread Per MATRIX Method: %d\n", no_threads);
    printf("Microseconds taken Per MATRIX: %lu\n\n", stop.tv_usec - start.tv_usec);

    write_mat_to_file(MatOut, "matrix", row1, col2);
    reset_mat_out();
}

void per_row_body() {
    gettimeofday(&start, NULL); //start checking time
    int no_threads = thread_per_row();
    gettimeofday(&stop, NULL); //end checking time

    printf("Number of threads Thread Per ROW Method: %d\n", no_threads);
    printf("Microseconds taken Per ROW: %lu\n\n", stop.tv_usec - start.tv_usec);

    write_mat_to_file(MatOut, "row", row1, col2);
    reset_mat_out();
}

void per_element_body() {
    gettimeofday(&start, NULL); //start checking time
    int no_threads = thread_per_element();
    gettimeofday(&stop, NULL); //end checking time

    printf("Number of threads Thread Per ELEMENT Method: %d\n", no_threads);
    printf("Microseconds taken Per Element: %lu\n", stop.tv_usec - start.tv_usec);

    write_mat_to_file(MatOut, "element", row1, col2);
    reset_mat_out();
}

int thread_per_matrix() {
    // Perform matrix multiplication
    for (int i = 0; i < row1; i++) {
        for (int j = 0; j < col2; j++) {
            MatOut[i][j] = 0;
            for (int k = 0; k < col1; k++) {
                MatOut[i][j] += MatA[i][k] * MatB[k][j];
            }
        }
    }
    return  1;
}

void *thread_per_row_routine(void *ind){
    int i = (int)ind;
    for(int j = 0; j < col2; j++) {
        MatOut[i][j] = 0;
        for(int k = 0; k < col1; k++) {
            MatOut[i][j] += MatA[i][k] * MatB[k][j];
        }
    }
}

int thread_per_row() {
    pthread_t thread[MAX_SIZE];
    int indexes[MAX_SIZE];
    for(int i = 0; i < row1; i++) {
        // Different address is sent for each thread avoiding race condition
        pthread_create(&thread[i], NULL, thread_per_row_routine, (void *)i);
    }

    for(int i = 0; i < row1; i++) {
        pthread_join(thread[i], NULL);
    }
    return row1;
}

void *thread_per_element_routine(void *index) {
    struct indexes *inds = (struct indexes *) index;
    int i = inds->i, j = inds->j;
    MatOut[i][j] = 0;
    for(int k = 0; k < col1; k++) {
        MatOut[i][j] += MatA[i][k] * MatB[k][j];
    }
    free(inds);
}

int thread_per_element() {
    pthread_t thread[MAX_SIZE][MAX_SIZE];
    for(int i = 0; i < row1; i++) {
        for(int j = 0; j < col2; j++) {
            struct indexes *inds = malloc(sizeof(struct indexes));
            inds->i = i, inds->j = j;
            pthread_create(&thread[i][j], NULL, thread_per_element_routine, (void *)inds);
        }
    }

    for(int i = 0; i < row1; i++) {
        for(int j = 0; j < col2; j++) {
            pthread_join(thread[i][j], NULL);
        }
    }
    return  row1 * col2;
}
