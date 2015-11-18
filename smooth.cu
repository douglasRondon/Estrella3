#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
}rgb;

typedef struct
{
	int columns;
	int size;
	int flag;
}info;

__global__ void smooth (rgb *, rgb *, int , int);
void cudaError(cudaError_t);


int main (int argc, char **argv){
	FILE *file;
	int i, rows, columns, max;
	rgb *imgH, *newImgH, *imgD, *newImgD;
    clock_t cInit, cFinal;
	
	file = fopen("2.ppm", "rb");
	fseek(file, 2, SEEK_SET);
	fscanf(file, "%d", &columns);
	fscanf(file, "%d", &rows);
	/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de entrada*/
	imgH = (rgb*) malloc ((rows*columns)*sizeof(rgb));
	
	/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de saída*/
	newImgH = (rgb*) malloc ((rows*columns)*sizeof(rgb));
	fscanf(file,"%d\n",&max);
	/* lendo a imagem do arquivo de entrada para a matriz */	
	for(i = 0; i < rows*columns; i++){
		fread(&imgH[i].r,sizeof(unsigned char),1,file);
		fread(&imgH[i].g,sizeof(unsigned char),1,file);
		fread(&imgH[i].b,sizeof(unsigned char),1,file);
	}
	fclose(file);
	
    cInit = clock(); /* COLOCA AQUI OU DEPOIS DO MALLOC???? */
	
	cudaError(cudaMalloc(&imgD, sizeof(rgb)*rows*columns));
	cudaError(cudaMalloc(&newImgD, sizeof(rgb)*rows*columns));
	cudaError(cudaMemcpy(imgD, imgH, sizeof(rgb)*rows*columns ,cudaMemcpyHostToDevice));
	dim3 threadsPerBlock(32, 32);
	dim3 numBlocks ((columns + threadsPerBlock.x - 1) / threadsPerBlock.x, (rows + threadsPerBlock.y - 1 ) / threadsPerBlock.y);
	
	smooth<<<numBlocks, threadsPerBlock>>>(imgD, newImgD, columns, rows);
	cudaError(cudaThreadSynchronize());
	cudaError(cudaMemcpy(newImgH, newImgD, sizeof(rgb)*rows*columns ,cudaMemcpyDeviceToHost));
    
    cFinal = clock();
	
    printf("Tempo: %lf segundos\n", (double)(cFinal - cInit) / CLOCKS_PER_SEC);

	/*criando a nova imagem */
	file = fopen("out.ppm", "wb");
	fprintf(file, "P6\n");
	fprintf(file, "%d %d\n",columns,rows);
	fprintf(file, "%d\n",max);
	for(i = 0; i < rows*columns; i++){
		fwrite(&newImgH[i].r ,sizeof(unsigned char),1,file);
		fwrite(&newImgH[i].g ,sizeof(unsigned char),1,file);
		fwrite(&newImgH[i].b ,sizeof(unsigned char),1,file);
	}
	fclose(file);

	/* liberando a memória utilizada */ 
	free(imgH);
	free(newImgH);
	cudaFree(imgD);
	cudaFree(newImgD);	
	return 0;	
}

/* função que retorna a média de uma componente do pixel utilizando os valores da componente ao redor dela (numa sub matriz 5x5) */
__global__ void smooth(rgb *image, rgb *newImg, int cols, int rows){
	int x, y;
	x = blockIdx.y * blockDim.y + threadIdx.y;
	y = blockIdx.x * blockDim.x + threadIdx.x;
	if(x > rows-1 || y > cols - 1)
		return;
	int i, j;
	int sumR = 0,sumG = 0,sumB = 0, count = 0;
	for(i = x-2; i < x+2; i++){
		for(j = y-2; j < y+2; j++){
			if((j < 0 || j > cols-1) || (i < 0 || i > rows-1));
			else{				
				sumR += image[i * cols + j].r;
				sumG += image[i * cols + j].g;
				sumB += image[i * cols + j].b;
				count++;
			}
		}
	}
	newImg[x * cols + y].r = sumR/count;
	newImg[x * cols + y].g = sumG/count;
	newImg[x * cols + y].b = sumB/count;
}

void cudaError(cudaError_t error){
	if (error != cudaSuccess) {
		fprintf(stderr,"ERROR: %s\n", cudaGetErrorString(error));
		exit(EXIT_FAILURE);
	}
}