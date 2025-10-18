#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <omp.h> 

#define STRING_LEN 256

struct stLattice{
    unsigned char *buff0;
    unsigned char *buff1;
    int width;
    int height;
    int steps;
};
typedef struct stLattice tpLattice;

/**
 * \brief Game of Life  help algorithm.
 **/
void help(){
    fprintf(stdout, "\nGOL algorithm (Parallel Version)\n");
    fprintf(stdout, "Usage: ./gol.exec [OPTIONS]\n");
    fprintf(stdout, "**Parameter options:**\n");
    fprintf(stdout, "\t'-h', '--help': Show this help message\n");
    fprintf(stdout, "\t'-v', '--verbose': Explain what is being done\n");
    fprintf(stdout, "\t'-a', '--answer': File to save the last state of GOL.\n");
    fprintf(stdout, "\t'-p', '--probability': Probability of a cell being live.\n");
    fprintf(stdout, "\t'-x', '--width': Lattice width.\n");
    fprintf(stdout, "\t'-y', '--height': Lattice height.\n");
    fprintf(stdout, "\t'-s', '--steps': Time steps of simulation.\n");
    exit(EXIT_FAILURE);
}

void InitRandness(tpLattice *mLattice, float p);
void GameOfLife(tpLattice *mLattice);
void print2File(char *filename, tpLattice *mLattice);

int main(int ac, char**av)
{
    tpLattice mLattice;
    float prob   = 0.25f;
    char file_name_template[STRING_LEN] ;
    int verbose_flag = 0;
    int option_index = 0;
    int input_opt = 0;
    file_name_template[0] = 0;

    //Inicializa variável
    mLattice.width  = 1024;
    mLattice.height = 1024;
    mLattice.steps  = 1000;

    if (ac == 1)
       help();
    
    struct option long_options[] =
    {
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {"answer",  required_argument, 0, 'a'},
        {"probability", required_argument, 0, 'p'},
        {"width",   required_argument, 0, 'x'},
        {"height",  required_argument, 0, 'y'},
        {"steps",   required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while ((input_opt = getopt_long (ac, av, "hva:p:x:y:s:", long_options, &option_index)) != EOF){
        switch (input_opt)
        {
            case 'h': help(); break;
            case 'v': verbose_flag = 1; break;
            case 'a': strcpy(file_name_template, optarg); break;
            case 'p': prob = atof(optarg); break;
            case 'x': mLattice.width = atoi(optarg); break;
            case 'y': mLattice.height = atoi(optarg); break;
            case 's': mLattice.steps = atoi(optarg); break;
            default: help(); break;
        }
    };

    if (verbose_flag){
      fprintf(stdout, "\nGame of life");
      fprintf(stdout, " - Dominio(%d, %d, %d)\n",   mLattice.width,   mLattice.height, mLattice.steps);
      fprintf(stdout, " - Probabilidade de estar viva = %5.3f\n", prob);
      fprintf(stdout, " - Arquivo resposta [%s]\n", file_name_template);
      fflush(stdout);
    }

    mLattice.buff0 = (unsigned char*) malloc (mLattice.width * mLattice.height * sizeof(unsigned char));
    mLattice.buff1 = (unsigned char*) malloc (mLattice.width * mLattice.height * sizeof(unsigned char));
    InitRandness(&mLattice, prob);

    //Início da medição de tempo
    double start_time = omp_get_wtime();

    for (int t = 0; t < mLattice.steps; t++)
    {
      GameOfLife(&mLattice);
      unsigned char *swap = mLattice.buff0;
      mLattice.buff0 = mLattice.buff1;
      mLattice.buff1 = swap;
    }

    //Fim da medição de tempo e impressão do resultado
    double end_time = omp_get_wtime();
    double exec_time = end_time - start_time;
    
    // Imprime o tempo de execução e o número de threads utilizadas
    #pragma omp parallel
    {
        #pragma omp master
        {
            if (verbose_flag) {
                fprintf(stdout, " - Executado com %d threads\n", omp_get_num_threads());
            }
        }
    }
    fprintf(stdout, "Tempo de execução (simulação): %f segundos\n", exec_time);

    if (strlen(file_name_template) > 0){
      if (verbose_flag) fprintf(stdout, " - Salvando o arquivo: [%s]", file_name_template);
      print2File(file_name_template, &mLattice);
      if (verbose_flag) fprintf(stdout, " [OK]\n");
    }

    free(mLattice.buff0);
    free(mLattice.buff1);
    return EXIT_SUCCESS;
}

void InitRandness(tpLattice *mLattice, float p){
  memset(mLattice->buff0, 0x00,  mLattice->width * mLattice->height * sizeof(unsigned char));
  memset(mLattice->buff1, 0x00,  mLattice->width * mLattice->height * sizeof(unsigned char));
  srand (42);
  for (int j = 1; j < mLattice->height - 1; j++){
      for (int i = 1; i < mLattice->width - 1; i++){
          int k = j * mLattice->width  +  i;
          float r = (rand() / (float)RAND_MAX);
          if (r <= p)
            mLattice->buff0[k] = 1;
      }
  }
}

void GameOfLife(tpLattice *mLattice){
    //OpenMP para paralelizar o loop principal 
    #pragma omp parallel for
    for (int j = 1; j < mLattice->height - 1; j++){
        for (int i = 1; i < mLattice->width - 1; i++){
            // Variáveis movidas para dentro do loop para serem privadas a cada iteração
            int nw, n, ne, w, e, sw, s, se, c, sum;
            
            nw = mLattice->buff0[(j - 1) * mLattice->width  +  (i - 1)];
            n  = mLattice->buff0[(j - 1) * mLattice->width  +  i];
            ne = mLattice->buff0[(j - 1) * mLattice->width  +  (i + 1)];
            w  = mLattice->buff0[j * mLattice->width  +  (i - 1)];
            c  = mLattice->buff0[j * mLattice->width  +  i];
            e  = mLattice->buff0[j * mLattice->width  +  (i + 1)];
            sw = mLattice->buff0[(j + 1) * mLattice->width  +  (i - 1)];
            s  = mLattice->buff0[(j + 1) * mLattice->width  +  i];
            se = mLattice->buff0[(j + 1) * mLattice->width  +  i+1];

            sum = nw + n + ne + w + e + sw + s + se;

            // Regras do Jogo da Vida
            if ((sum == 3) && (c == 0)) // Célula morta com 3 vizinhos vivos -> nasce [cite: 9]
                mLattice->buff1[j  * mLattice->width  +  i] = 1;
            else if ((c == 1) && (sum < 2 || sum > 3)) // Célula viva com < 2 (solidão) ou > 3 (superpopulação) vizinhos -> morre [cite: 7, 8]
                mLattice->buff1[j  * mLattice->width  +  i] = 0;
            else // Nos outros casos (viva com 2 ou 3 vizinhos, ou morta sem 3 vizinhos), o estado se mantém [cite: 10]
                mLattice->buff1[j  * mLattice->width  +  i] = c;
        }
    }
}

void print2File(char *filename, tpLattice *mLattice)
{
  FILE *ptr = fopen(filename, "w+");
  assert(ptr  != NULL);

  for (int j = 1; j < mLattice->height - 1; j++){
      for (int i = 1; i < mLattice->width - 1; i++){
          int k = j * mLattice->width  +  i;
          if (mLattice->buff0[k] == 1)
            fputc('#', ptr);
          else
            fputc(' ', ptr);
      }
      fputc('\n', ptr);
  }
  fclose(ptr);
}