#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <imageprocessing.h>
#include <unistd.h>
#include <time.h>

#define MAX_PROCESSOS 4
#define NORMALIZADOR 20
#define LOOPS 5

void clear(imagem* I);
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

int main() {

	double time_begin, time_end, acumulator;

	for (int i = 0; i < LOOPS; ++i)
	{
		
	time_begin = time(NULL);
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
  int visibility = MAP_SHARED | MAP_ANON, line = 0;
  pid_t filho[MAX_PROCESSOS], filho_rebelde;
  imagem imgIn = abrir_imagem("images/teste2.jpg");
 
 printf("%dx%d\n", imgIn.width, imgIn.height);

 //MEMORIA COMPARTILHADA
  imagem* imgOut = (imagem*)mmap(NULL, sizeof(imagem), protection, visibility, 0, 0);
  imgOut->width = imgIn.width;
  imgOut->height = imgIn.height;
  imgOut->r = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);
  imgOut->g = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);
  imgOut->b = mmap(NULL, sizeof(float)*(imgIn.width)*imgIn.height, protection, visibility, 0, 0);

  while(line < imgIn.height){

    if(line < 5){ 
      for(line = 0; line < MAX_PROCESSOS; line++){
        filho[line] = fork();
        if(filho[line] == 0){
          applyConvolution(&imgIn, imgOut, line, blur, 3, 3, 9);
          exit(0);
        }
      }
    }
    wait(NULL);
   
    filho_rebelde = fork();
    if(filho_rebelde == 0){
      applyConvolution(&imgIn, imgOut, line, blur, 3, 3, 9);
      exit(0);
    }
    line++;
  }

	while(wait(NULL) > 0);

  salvar_imagem("filtros/pazrotacionado.jpg", imgOut);
  liberar_imagem(&imgIn);
  clear(imgOut);
  munmap(imgOut, sizeof(imagem));

  time_end = time(NULL);
  acumulator += difftime(time_end, time_begin);
}
	acumulator = acumulator/LOOPS;
	printf("%f\n", acumulator);
  return 0;
}

void clear(imagem* I){
	munmap(I->r, sizeof(float));
	munmap(I->g, sizeof(float));
	munmap(I->b, sizeof(float));
}