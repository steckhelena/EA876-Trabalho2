#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <imageprocessing.h>
#include <time.h>

#define MAX_THREADS 4
#define LOOPS 5

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

	imagem imgIn = abrir_imagem("images/teste.jpg");

	imagem *imgOut = malloc(sizeof(imagem));
	imgOut->width = imgIn.width;
	imgOut->height = imgIn.height;
	imgOut->r = malloc(sizeof(float)*imgIn.width*imgIn.height);
	imgOut->g = malloc(sizeof(float)*imgIn.width*imgIn.height);
	imgOut->b = malloc(sizeof(float)*imgIn.width*imgIn.height);

	workerArgs *args = malloc(sizeof(workerArgs));
	args->imgIn = &imgIn;
	args->imgOut = imgOut;
	args->matrix = blur5x5;
	args->m = 5;
	args->n = 5;
	args->divisor=25;
	args->current_line = malloc(sizeof(unsigned int));
	*args->current_line = 0;
	args->lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(args->lock, NULL);
	
	pthread_t tid[MAX_THREADS];
	time_begin = time(NULL);
	for (int i=0; i<MAX_THREADS; i++) {
		pthread_create(&tid[i], NULL, worker, (void*)args);
	}
	for (int i=0; i<MAX_THREADS; i++) {
		pthread_join(tid[i], NULL);
	}
	time_end = time(NULL);
	salvar_imagem("filtros/teste.jpg", imgOut);
	liberar_imagem(&imgIn);
	liberar_imagem(imgOut);
	free(imgOut);
	
	time_size = difftime(time_end, time_begin);
	printf("%f\n", time_size);
	return 0;
}
