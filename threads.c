#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <imageprocessing.h>
#include <sys/time.h>

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

float timedifference_msec(struct timeval t0, struct timeval t1) {
	return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

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

	imagem imgIn = abrir_imagem(argv[1]);

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

	int averages = atoi(argv[2]);
	gettimeofday(&time_begin, 0);
	for (int j=0; j<averages; j++) {
		for (int i=0; i<MAX_THREADS; i++) {
			pthread_create(&tid[i], NULL, worker, (void*)args);
		}
		for (int i=0; i<MAX_THREADS; i++) {
			pthread_join(tid[i], NULL);
		}
		*args->current_line = 0;
	}
	gettimeofday(&time_end, 0);
	time_size = timedifference_msec(time_begin, time_end);

	printf("Arquivo da imagem: %s\n", argv[1]);
	printf("Resolução: %dx%d\n", imgIn.width, imgIn.height);
	printf("Tamanho da matriz de convolução: 5x5\n");
	printf("Arquivo da imagem: %s\n", argv[1]);
	printf("Estrategia: threads, 4\n");
	printf("Tempo gasto: %f\n", (time_size/averages));

	salvar_imagem("teste.jpg", imgOut);
	liberar_imagem(&imgIn);
	liberar_imagem(imgOut);
	free(imgOut);

	return 0;
}
