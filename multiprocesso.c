#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <imageprocessing.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <assert.h>

#define MAX_PROCESSOS 4

typedef struct {
	imagem *imgIn;
	imagem *imgOut;
	int *matrix;
	int m;
	int n;
	int divisor;
	pthread_mutex_t* lock;
	int *current_line;
} workerArgs;

void clear(imagem* I, imagem* O, workerArgs* args);

float timedifference_msec(struct timeval t0, struct timeval t1) {
	return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

//APLICACAO DA CONVOLUCAO
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

void *worker(void *arg) {
	workerArgs *args = (workerArgs *)arg;

	unsigned int line;
	do {
		pthread_mutex_lock(args->lock);
		line = *args->current_line;
		if (line >= args->imgIn->height) {
			pthread_mutex_unlock(args->lock);
			break;
		}
		*args->current_line = (*args->current_line) + 1;
		pthread_mutex_unlock(args->lock);

		applyConvolution(args->imgIn, args->imgOut, line, args->matrix, args->m, args->n, args->divisor);
	} while(1);

	return NULL;
}

int main(int argc, char *argv[]) {
	struct timeval time_begin, time_end;
	float time_size;

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
	imagem imgIn = abrir_imagem(argv[1]);

	imagem* imgOut = (imagem*)mmap(NULL, sizeof(imagem), protection, visibility, 0, 0);
	imgOut->width = imgIn.width;
	imgOut->height = imgIn.height;
	imgOut->r = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);
	imgOut->g = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);
	imgOut->b = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);
	
	//MEMORIA COMPARTILHADA
	workerArgs *args = mmap(NULL, sizeof(workerArgs), protection, visibility, -1, 0);
	args->imgIn = &imgIn;
	args->imgOut = imgOut;
	args->matrix = blur5x5;
	args->m = 5;
	args->n = 5;
	args->divisor=25;
	args->current_line = mmap(NULL, sizeof(unsigned int), protection, visibility, -1, 0);
	*args->current_line = 0;
	args->lock = mmap(NULL, sizeof(pthread_mutex_t), protection, visibility, -1, 0);

	//INICIALIZANDO MUTEX
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(args->lock, &attr);

	double averages = atoi(argv[2]);
	gettimeofday(&time_begin, 0);
	for (int j = 0; j < averages; j++) {
		for(int i = 0; i < 4; i++){
			filho[i] = fork();
			if(filho[i] == 0){
				worker(args);
				exit(0);
			}
		}

		while(wait(NULL) > 0);
		*args->current_line = 0;
	}
	gettimeofday(&time_end, 0);
	time_size = timedifference_msec(time_begin, time_end);

	printf("Arquivo da imagem: %s\n", argv[1]);
	printf("Resolução: %dx%d\n", imgIn.width, imgIn.height);
	printf("Tamanho da matriz de convolução: 5x5\n");
	printf("Arquivo da imagem: %s\n", argv[1]);
	printf("Estrategia: processos, 4\n");
	printf("Tempo gasto: %f\n", (time_size/averages));

	salvar_imagem("teste1.jpg", imgOut);
	clear(&imgIn, imgOut, args);
	

	return 0;
}

//Liberar Memoria
void clear(imagem* I, imagem* O, workerArgs* args){
	munmap(I->r, sizeof(float));
	munmap(I->g, sizeof(float));
	munmap(I->b, sizeof(float));
	munmap(O->r, sizeof(float));
	munmap(O->g, sizeof(float));
	munmap(O->b, sizeof(float));
	munmap(I, sizeof(imagem));
	munmap(O, sizeof(imagem));
	munmap(args->current_line, sizeof(unsigned int));
	munmap(args->lock, sizeof(pthread_mutex_t));
	munmap(args, sizeof(workerArgs));
}
