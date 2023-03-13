#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_SIZE 20

// Dimensions of a and b matrices and c matrix
int row1, row2, col1, col2, MatA[MAX_SIZE][MAX_SIZE], MatB[MAX_SIZE][MAX_SIZE], MatOut[MAX_SIZE][MAX_SIZE];
FILE *fa, *fb, *fOut, *fOutRow, *fOutElement;
struct timeval stop, start;
const char file_names[5][25] = {"x", "y", "z_per_matrix", "z_per_row", "z_per_element"};

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
int  check_dimentions();
void take_mat(FILE **file, const char *filename, int mat[MAX_SIZE][MAX_SIZE], int *row, int *col);
void write_mat_to_file(int mat[MAX_SIZE][MAX_SIZE], FILE **file, const char *filename, int row, int col);
int file_exists(FILE **file);

int main() {
    take_mat(&fa, file_names[0], MatA, &row1, &col1);
    take_mat(&fb, file_names[1], MatB, &row2, &col2);
    if(!check_dimentions()) {
        perror("Invalid dimensions");
        exit(1);
    }
    per_matrix_body();
    per_row_body();
    per_element_body();
}

void per_matrix_body() {
    fOut = fopen(file_names[2], "a");
    if(!file_exists(&fOut)) exit(1);
    fprintf(fOut, "Method: A thread per Matrix\nrow=%d col=%d\n", row1, col2);
    fclose(fOut);

    gettimeofday(&start, NULL); //start checking time
    int no_threads = thread_per_matrix();
    printf("Number of threads Thread Per MATRIX Method: %d\n", no_threads);
    gettimeofday(&stop, NULL); //end checking time
    printf("Microseconds taken Per MATRIX: %lu\n\n", stop.tv_usec - start.tv_usec);

    write_mat_to_file(MatOut, &fOut, file_names[2], row1, col2);
    reset_mat_out();
}

void per_row_body() {
    fOutRow = fopen(file_names[3], "a");
    if(!file_exists(&fOutRow)) exit(1);
    fprintf(fOutRow, "Method: A thread per row\nrow=%d col=%d\n", row1, col2);
    fclose(fOutRow);

    gettimeofday(&start, NULL); //start checking time
    int no_threads = thread_per_row();
    printf("Number of threads Thread Per ROW Method: %d\n", no_threads);
    gettimeofday(&stop, NULL); //end checking time
    printf("Microseconds taken Per ROW: %lu\n\n", stop.tv_usec - start.tv_usec);

    write_mat_to_file(MatOut, &fOutRow, file_names[3], row1, col2);
    reset_mat_out();
}

void per_element_body() {
    fOutElement = fopen(file_names[4], "a");
    if(!file_exists(&fOutElement)) exit(1);
    fprintf(fOutElement, "Method: A thread per element\nrow=%d col=%d\n", row1, col2);
    fclose(fOutElement);

    gettimeofday(&start, NULL); //start checking time
    int no_threads = thread_per_element();
    printf("Number of threads Thread Per ELEMENT Method: %d\n", no_threads);
    gettimeofday(&stop, NULL); //end checking time
    printf("Microseconds taken Per Element: %lu\n", stop.tv_usec - start.tv_usec);

    write_mat_to_file(MatOut, &fOutElement, file_names[4], row1, col2);
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
            fprintf(fOut, "%d ", MatOut[i][j]);
        }
        fprintf(fOut, "\n");
    }
    return  1;
}

void *thread_per_row_routine(void *ind){
    int i = *(int *)ind;
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
        indexes[i] = i;
        pthread_create(&thread[i], NULL, &thread_per_row_routine, &indexes[i]);
    }

    for(int i = 0; i < row1; i++) {
        pthread_join(thread[i], NULL);
    }
    return row1;
}

void *thread_per_element_routine(void *index) {
    struct indexes *inds;
    inds = (struct indexes *) index;
    int i = inds->i;
    int j = inds->j;
    MatOut[i][j] = 0;
    for(int k = 0; k < col1; k++) {
        MatOut[i][j] += MatA[i][k] * MatB[k][j];
    }
}

int thread_per_element() {
    pthread_t thread[MAX_SIZE][MAX_SIZE];
    struct indexes idx[MAX_SIZE][MAX_SIZE];
    for(int i = 0; i < row1; i++) {
        for(int j = 0; j < col2; j++) {
            struct indexes inds;
            inds.i = i;
            inds.j = j;
            // Different address is sent for each thread avoiding race condition
            idx[i][j] = inds;
            pthread_create(&thread[i][j], NULL, &thread_per_element_routine, &idx[i][j]);
        }
    }

    for(int i = 0; i < row1; i++) {
        for(int j = 0; j < col2; j++) {
            pthread_join(thread[i][j], NULL);
        }
    }
    return  row1 * col2;
}

void take_mat(FILE **file, const char *filename, int mat[MAX_SIZE][MAX_SIZE], int *row, int *col) {
    char input[100], _temp[100];
    fgets(_temp, 100, stdin);
    strcpy(input, _temp);

    // input "m=m1 n=n1"
    if (sscanf(_temp, "row=%d col=%d", row, col) != 2) {
        fprintf(stderr, "Invalid input format.\n");
        return;
    }

    // Open the file
    *file = fopen(filename, "a");
    if(!file_exists(file)) exit(1);
    fprintf(*file, "%s", input);
    fclose(*file);

    for(int i = 0; i < *row; i++) {
        for(int j = 0; j < *col; j++) {
            scanf("%d", &mat[i][j]);
        }
    }
    char c = getchar();
    write_mat_to_file(mat, file, filename, *row, *col);
}

void reset_mat_out() {
    for(int i = 0; i < MAX_SIZE; i++) {
        for(int j = 0; j < MAX_SIZE; j++) {
            MatOut[i][j] = 0;
        }
    }
}

int check_dimentions() {
    if(col1 != row2) return 0;
    else return 1;
}

int file_exists(FILE **file) {
    if(file == NULL) {
        perror("Error open file");
        return 0;
    } else {
        return  1;
    }
}

void write_mat_to_file(int mat[MAX_SIZE][MAX_SIZE], FILE **file, const char *filename, int row, int col) {
    *file = fopen(filename, "a");
    // Take matrix entries and write them into the file
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            fprintf(*file, "%d ", mat[i][j]);
        }
        fprintf(*file, "\n");
    }
    fclose(*file);
}