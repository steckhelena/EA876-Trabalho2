#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <imageprocessing.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <assert.h>

#define MAX_PROCESSOS 4
#define NORMALIZADOR 20
#define LOOPS 1

typedef struct
{
  bool done;
  pthread_mutex_t mutex;
} shared_data;

static shared_data* data = NULL;

void clear(imagem* I, imagem* O);

void applyConvolution(imagem* imgIn, imagem* imgOut, int line, int *matrix, int m, int n, int divisor) {
  for (int i=0; i<imgIn->width; i++) {
    float auxR = 0;
    float auxG = 0;
    float auxB = 0;

    for (int j=i-m/2; j<=i+m/2; j++) {
      for (int k=line-n/2; k<=line+n/2; k++) {
        int x=j, y=k, posC=m*(k-line+n/2)+j-i+m/2, pos;


        if (j < 0) {
          x = 0;
        } else if (j >= imgIn->width) {
          x = imgIn->width-1;
        }

        if (k < 0) {
          y = 0;
        } else if (k >= imgIn->height) {
          y = imgIn->height-1;
        }
        
        y *= imgIn->width;
        pos = x+y;
        auxR += matrix[posC]*imgIn->r[pos];
        auxG += matrix[posC]*imgIn->g[pos];
        auxB += matrix[posC]*imgIn->b[pos];
      }
    }
    auxR /= divisor;
    auxG /= divisor;
    auxB /= divisor;

    imgOut->r[line*imgOut->width + i] = auxR;
    imgOut->g[line*imgOut->width + i] = auxG;
    imgOut->b[line*imgOut->width + i] = auxB;
  }
}

void process(imagem *imgIn, imagem *imgOut, int line, int* blur){

  int line_local;
  do{
    pthread_mutex_lock(&data->mutex);
    line_local = line;
    line++;
    if(line > imgIn->height){
      pthread_mutex_unlock(&data->mutex);
      break;
    }
    pthread_mutex_unlock(&data->mutex);
    applyConvolution(imgIn, imgOut, line_local, blur, 3, 3, 9);
  }while(true);

}

int main() {

	double time_begin, time_end, time_size;

  int blur[9] = 
  {1,1,1,
   1,1,1,
   1,1,1};

  int blur5x5[25] = 
  {1,1,1,1,1,
   1,1,1,1,1,
   1,1,1,1,1,
   1,1,1,1,1,
   1,1,1,1,1};

  int gaussianBlur[9] = 
  {1,2,1,
   2,4,2,
   1,2,1};

   //Definicao de variaveis para MMAP
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_SHARED | MAP_ANONYMOUS, line = 0;
  pid_t filho[4];
  imagem imgIn = abrir_imagem("images/teste1.jpg");

 //MEMORIA COMPARTILHADA
  data = mmap(NULL, sizeof(shared_data), protection, visibility, -1, 0);
  data->done = false;

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&data->mutex, &attr);

  imagem* imgOut = (imagem*)mmap(NULL, sizeof(imagem), protection, visibility, 0, 0);
  imgOut->width = imgIn.width;
  imgOut->height = imgIn.height;
  imgOut->r = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);
  imgOut->g = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);
  imgOut->b = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);

  time_begin = time(NULL);
  for(int i = 0; i < 4; i++){
    filho[i] = fork();
    if(filho[i] == 0){
      process(&imgIn, imgOut, i, blur);
      exit(0);
    }
  }

	while(wait(NULL) > 0);
  time_end = time(NULL);
  time_size = difftime(time_begin, time_end);

  salvar_imagem("filtros/teste1.jpg", imgOut);
  clear(&imgIn, imgOut);
	printf("%f\n", time_size);
  return 0;
}

//Liberar Memoria
void clear(imagem* I, imagem* O){
	munmap(I->r, sizeof(float));
	munmap(I->g, sizeof(float));
	munmap(I->b, sizeof(float));
  munmap(O->r, sizeof(float));
  munmap(O->g, sizeof(float));
  munmap(O->b, sizeof(float));
  munmap(I, sizeof(imagem));
  munmap(O, sizeof(imagem));
  munmap(data, sizeof(shared_data));
}
