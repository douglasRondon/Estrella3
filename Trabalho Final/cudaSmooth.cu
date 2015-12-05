#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* estrutura para tratar cada pixel da imagem, ela possui as tres componentes de cada pixel (red, green e blue) */
typedef struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;
}rgb;

//assinatura das funcoes
__global__ void smooth (rgb *, rgb *, int , int);
void cudaError(cudaError_t);


int main (int argc, char **argv){
	FILE *file;
	int i, rows, columns, max;
	rgb *imgH, *newImgH, *imgD, *newImgD;
    clock_t cInit, cFinal;
	
	/* abre a imagem a qual sera aplicado o filtro */
	file = fopen("in.ppm", "rb");
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
	
	/* aloca memoria na GPU para a imagem a ser processada */
	cudaMalloc(&imgD, sizeof(rgb)*rows*columns);

	/* aloca memoria na GPU para o resultado da imagem a ser processada */
	cudaError(cudaMalloc(&newImgD, sizeof(rgb)*rows*columns));

	/* copia a matriz da imagem da memoria da CPU para a da GPU */
	cudaError(cudaMemcpy(imgD, imgH, sizeof(rgb)*rows*columns ,cudaMemcpyHostToDevice));
	
	/* define o numero de threads nas dimensoes x e y por blocos e o tamanho do grid */ 
	dim3 threadsPerBlock(32, 32);
	dim3 numBlocks ((columns + threadsPerBlock.x - 1) / threadsPerBlock.x, (rows + threadsPerBlock.y - 1 ) / threadsPerBlock.y);

    cInit = clock();
	/* faz a chamada da funcao que aplica o filtro em CUDA, passando os parametros definidos acima */
	smooth<<<numBlocks, threadsPerBlock>>>(imgD, newImgD, columns, rows);

	/*  garante que o host não execute até que todas as operações CUDA terminem */
	cudaDeviceSynchronize();

    cFinal = clock();

	/* copia a nova imagem da memoria da GPU para a memoria da CPU */
	cudaMemcpy(newImgH, newImgD, sizeof(rgb)*rows*columns ,cudaMemcpyDeviceToHost);    
	
    printf("Tempo: %lf segundos\n", (double)(cFinal - cInit) / CLOCKS_PER_SEC);

	/*escreve a nova imagem */
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
	/* define o valor para x e y que ira trabalhar agora baseado na thread e no block que esta */
	x = blockIdx.y * blockDim.y + threadIdx.y; 
	y = blockIdx.x * blockDim.x + threadIdx.x;
	/* caso seja maior que o limite da imagem nao faz nada e termina */
	if(x > rows-1 || y > cols - 1)
		return;
	int i, j;
	int sumR = 0,sumG = 0,sumB = 0, count = 0;
	/* percorre os 5 pixels ao redor do ponto atual(em todas as direcoes) calculando a soma de seus valores para r,g e b */
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
	/* divide a soma calculada anteriomente pelo numero de pixels percorridos e escreve na matriz da nova imagem */
	newImg[x * cols + y].r = sumR/count;
	newImg[x * cols + y].g = sumG/count;
	newImg[x * cols + y].b = sumB/count;
}

/* funcao que trata dos erros que podem ocorre na execucao CUDA */
void cudaError(cudaError_t error){
	if (error != cudaSuccess) {
		fprintf(stderr,"ERROR: %s\n", cudaGetErrorString(error));
		exit(EXIT_FAILURE);
	}
}
