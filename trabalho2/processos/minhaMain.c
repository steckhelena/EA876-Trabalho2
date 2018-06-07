
#include <imageprocessing.h>

#define LINE 3
#define COLUNM 3


void kindeffect(int** convolution);
void application_R(imagem img);

int convolution[LINE][COLUNM];

int main() {
  imagem img;
  img = abrir_imagem("images/montanha.jpg");
  unsigned int tmp;

  kindeffect();
  


  application(img);

  salvar_imagem("filtros/montanha.jpg", &img);
  liberar_imagem(&img);
  return 0;
}

void kindeffect(int** convolution){

  int i,k, lim, sum;
  for(i = 0; i < LINE; i++){
    for(k = 0; k < COLUNM; k++)
      convolution[i][k] = 0;; 
  }
  convolution[0][2] = -1;
  convolution[2][0] = -1;
}

void application(imagem img, index_line){

  int index_colunm, acumulator, index_v, index_h;
  int pixel_v, pixel_h, conv_h = 0, conv_v = 0;

  for(index_colunm = 0; index_colunm < img.height; index_colunm++){
      
      acumulator_R = 0;
      acumulator_G = 0;
      acumulator_B = 0;

      for(pixel_v = index_line; pixel_v < LINE + index_line ; pixel_v++){
        index_v = pixel_v - 1;
        if(index_v < 0) index_v = 0;

        for(pixel_h = index_colunm; pixel_h < COLUNM + index_colunm; pixel_h){
          index_h = pixel_h - 1;
          if(index_h < 0) index_h = 0;
          acumulator_R += (img.r[index_v][index_h])*(convolution[conv_v%LINE][conv_h%COLUNM]);
          acumulator_G += (img.g[index_v][index_h])*(convolution[conv_v%LINE][conv_h%COLUNM]);
          acumulator_B += (img.b[index_v][index_h])*(convolution[conv_v%LINE][conv_h%COLUNM]);
          conv_h++;
        }
        conv_v++;
      }
  }
  img.r[index_line][index_colunm] = acumulator_R;
  img.g[index_line][index_colunm] = acumulator_G;
  img.b[index_line][index_colunm] = acumulator_B;
}