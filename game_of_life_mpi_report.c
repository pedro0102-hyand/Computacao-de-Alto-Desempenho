#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define IDX(i, j, w) ((i) * (w) + (j))

void save_pgm(int *grid, int rows, int cols, int step) {
    char filename[50];
    sprintf(filename, "gol_step_%05d.pgm", step);
    FILE *f = fopen(filename, "w");
    if (f == NULL) return;

    fprintf(f, "P2\n%d %d\n255\n", cols, rows); 

    for (int i = 0; i < rows * cols; i++) {
        fprintf(f, "%d ", grid[i] ? 0 : 255); 
        if ((i + 1) % cols == 0) fprintf(f, "\n");
    }
    fclose(f);
    printf("Imagem gerada: %s\n", filename);
}


int count_neighbors(int *grid, int r, int c, int cols) {
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            
            
            int nc = (c + j + cols) % cols; 
            
            int nr = r + i; 
            
            if (grid[IDX(nr, nc, cols)]) count++;
        }
    }
    return count;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

   
    int ROWS = 1000;
    int COLS = 1000;
    int STEPS = 200;

    if (argc >= 4) {
        ROWS = atoi(argv[1]);
        COLS = atoi(argv[2]);
        STEPS = atoi(argv[3]);
    }

    
    int local_rows = ROWS / size;
    int remainder = ROWS % size;
    if (rank < remainder) local_rows++;

    
    int *current = (int *)calloc((local_rows + 2) * COLS, sizeof(int));
    int *next    = (int *)calloc((local_rows + 2) * COLS, sizeof(int));

    
    srand(time(NULL) + rank);
    for (int i = 1; i <= local_rows; i++) {
        for (int j = 0; j < COLS; j++) {
            current[IDX(i, j, COLS)] = rand() % 2; 
        }
    }

    
    int top_neighbor = (rank == 0) ? size - 1 : rank - 1;
    int bot_neighbor = (rank == size - 1) ? 0 : rank + 1;

    
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    
    for (int s = 0; s < STEPS; s++) {
        
        
        MPI_Sendrecv(&current[IDX(1, 0, COLS)], COLS, MPI_INT, top_neighbor, 0,
                     &current[IDX(local_rows + 1, 0, COLS)], COLS, MPI_INT, bot_neighbor, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(&current[IDX(local_rows, 0, COLS)], COLS, MPI_INT, bot_neighbor, 1,
                     &current[IDX(0, 0, COLS)], COLS, MPI_INT, top_neighbor, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        
        for (int i = 1; i <= local_rows; i++) {
            for (int j = 0; j < COLS; j++) {
                int neighbors = count_neighbors(current, i, j, COLS);
                int alive = current[IDX(i, j, COLS)];
                
                if (alive && (neighbors < 2 || neighbors > 3)) next[IDX(i, j, COLS)] = 0;
                else if (!alive && neighbors == 3) next[IDX(i, j, COLS)] = 1;
                else next[IDX(i, j, COLS)] = alive;
            }
        }
        
        int *temp = current; current = next; next = temp;
    }

    
    double end_time = MPI_Wtime();
    double local_elapsed = end_time - start_time;
    double max_elapsed;
    MPI_Reduce(&local_elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("MPI processes detected: %d\n", size);
        printf("rows x cols: %d x %d, steps: %d\n", ROWS, COLS, STEPS);
        printf("Max total_time (across ranks): %f s\n", max_elapsed);
    }

    // =========================================================================
    // BLOCO DE GERAÇÃO DE IMAGEM 
    // =========================================================================
    /*
    int *send_buf = (int *)malloc(local_rows * COLS * sizeof(int));
    for (int i = 1; i <= local_rows; i++) {
        for (int j = 0; j < COLS; j++) {
            send_buf[IDX(i-1, j, COLS)] = current[IDX(i, j, COLS)];
        }
    }

    int *full_grid = NULL;
    int *recvcounts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        full_grid = (int *)malloc(ROWS * COLS * sizeof(int));
        recvcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));

        int offset = 0;
        for (int r = 0; r < size; r++) {
            int r_rows = ROWS / size;
            if (r < remainder) r_rows++;
            
            recvcounts[r] = r_rows * COLS;
            displs[r] = offset;
            offset += recvcounts[r];
        }
    }

    MPI_Gatherv(send_buf, local_rows * COLS, MPI_INT,
                full_grid, recvcounts, displs, MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        save_pgm(full_grid, ROWS, COLS, STEPS); // Salva a imagem
        free(full_grid);
        free(recvcounts);
        free(displs);
    }
    free(send_buf);
    */
    // =========================================================================

    free(current);
    free(next);

    MPI_Finalize();
    return 0;
}