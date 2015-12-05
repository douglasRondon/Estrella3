
	#include <stdio.h>
	#include <stdlib.h>
	#include <omp.h>
	#include <time.h>
	#include "mpi.h"

	#define MASTER 0

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


	unsigned char avg (rgb **image, int x, int y, int cols, int rows, int flag);

	int main (int argc, char **argv){
		FILE *file;
		info imgInfo;
		int i, j,k,l, rows, columns, max, stripSize, extra, rc, size, rank;
		rgb **img, **newImg;
		double startTime, endTime;
		MPI_Status status;
		rc = MPI_Init(&argc, &argv);

		if(rc != MPI_SUCCESS){
			printf("Error!\n");
			return -1;
		}
		
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		
		if(rank == MASTER ){
			file = fopen("gray/ppm/3.ppm", "rb");
			fseek(file, 2, SEEK_SET);
			fscanf(file, "%d", &columns);
			fscanf(file, "%d", &rows);
			/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de entrada*/
			img = (rgb**) malloc (rows*sizeof(rgb*));
			
			/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de saída*/
			newImg = (rgb**) malloc (rows*sizeof(rgb*));
			
			for(i = 0; i < rows; i++){
				img[i] = (rgb*)malloc(columns*sizeof(rgb));

				newImg[i] = (rgb*)malloc(columns*sizeof(rgb));
			}
			fscanf(file,"%d\n",&max);
			/* lendo a imagem do arquivo de entrada para a matriz */	
			for(i = 0; i < rows; i++){
				for(j = 0; j < columns; j++){
					fread(&img[i][j].r,sizeof(unsigned char),1,file);
					fread(&img[i][j].g,sizeof(unsigned char),1,file);
					fread(&img[i][j].b,sizeof(unsigned char),1,file);
				}
			}
			fclose(file);
			stripSize = rows / (size - 1) ;
			extra = (rows % (size-1)) + stripSize + 2;
			j = 0;

			startTime = MPI_Wtime();
			
			imgInfo.columns = columns;
			for(i = 1; i < size; i++){
				if(i == 1){ /* envia as primeiras linhas para escravo */
					imgInfo.size = stripSize + 2;
					imgInfo.flag = 0;
					MPI_Send(&imgInfo, 3, MPI_INT, i, 40*i, MPI_COMM_WORLD);					
					for(k = 0; k < imgInfo.size; k++){
						MPI_Send(img[k], 3*imgInfo.columns, MPI_BYTE, i, 10*i, MPI_COMM_WORLD);						
					}
				}
				else if(i > 1 && i < size - 1){ /* envia as linhas seguintes para os escravos */
					imgInfo.size = stripSize + 4;
					imgInfo.flag = 1;
					MPI_Send(&imgInfo, 3, MPI_INT, i, 40*i, MPI_COMM_WORLD);
					for(k = j-2; k < j+imgInfo.size; k++){
						MPI_Send(img[k], 3*imgInfo.columns, MPI_BYTE, i, 10*i, MPI_COMM_WORLD);
					}
				}
				else{ /* envia a ultima linha para escravo */
					imgInfo.size = extra;
					imgInfo.flag = 2;
					MPI_Send(&imgInfo, 3, MPI_INT, i, 40*i, MPI_COMM_WORLD);
					for(k = j-2; k < j+extra-2; k++){
						MPI_Send(img[k], 3*imgInfo.columns, MPI_BYTE, i, 10*i, MPI_COMM_WORLD);
					}
				}
				j += stripSize;
			}
			

			/* recebendo resultados apos nós processarem */
			j = 0;
			for(i = 1; i < size-1; i++){
				for(k = j; k < j+stripSize; k++){
					MPI_Recv(newImg[k], 3*imgInfo.columns, MPI_BYTE, i, i*10 + 1, MPI_COMM_WORLD, &status);
				}
				j += stripSize;
			}
			for(k = j; k < j+extra-2; k++){
				MPI_Recv(newImg[k], 3*imgInfo.columns, MPI_BYTE, i, i*10 + 1, MPI_COMM_WORLD, &status);
			}

			endTime = MPI_Wtime();
			printf("%.6lf segundos\n", endTime - startTime);
			
			/*criando a nova imagem */
			file = fopen("out2.ppm", "wb");
			fprintf(file, "P6\n");
			fprintf(file, "%d %d\n",columns,rows);
			fprintf(file, "%d\n",max);
			for(i = 0; i < rows; i++){
				for(j = 0; j < columns; j++){
					fwrite(&newImg[i][j].r ,sizeof(unsigned char),1,file);
					fwrite(&newImg[i][j].g ,sizeof(unsigned char),1,file);
					fwrite(&newImg[i][j].b ,sizeof(unsigned char),1,file);
				}
			}
			fclose(file);
			
			/* liberando a memória utilizada */ 
			for(i = 0; i < rows; i++){
				free(img[i]);
				free(newImg[i]);
			}
			free(img);
			free(newImg);
		}
		
		else{
			MPI_Recv(&imgInfo, 3, MPI_INT, MASTER, 40*rank, MPI_COMM_WORLD, &status);

			/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de entrada*/
			img = (rgb**) malloc (imgInfo.size*sizeof(rgb*));

			/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de saída*/
			newImg = (rgb**) malloc (imgInfo.size*sizeof(rgb*));

			for(i = 0; i < imgInfo.size; i++){
				img[i] = (rgb*)malloc(imgInfo.columns*sizeof(rgb));

				newImg[i] = (rgb*)malloc(imgInfo.columns*sizeof(rgb));
			}
			/* recebendo dados do nó mestre */
			for(i = 0; i < imgInfo.size; i++){
				MPI_Recv(img[i], 3*imgInfo.columns, MPI_BYTE, MASTER, rank*10, MPI_COMM_WORLD, &status);
			}
			/* fazendo processamento da imagem para o efeito smooth */
			#pragma omp parallel for firstprivate(j) lastprivate(j)
			for(i = 0; i < imgInfo.size; i++){	
				for(j = 0; j < imgInfo.columns; j++){
					newImg[i][j].r = avg(img, j, i,imgInfo.columns,imgInfo.size,1);
					newImg[i][j].g = avg(img, j, i,imgInfo.columns,imgInfo.size,2);
					newImg[i][j].b = avg(img, j, i,imgInfo.columns,imgInfo.size,3);
				}
			}
			/* enviando mensagem de volta para o nó mestre */
			if(imgInfo.flag == 0){
				for(i = 0; i < imgInfo.size-2; i++){
					MPI_Send(newImg[i], 3*imgInfo.columns, MPI_BYTE, MASTER , rank*10 + 1, MPI_COMM_WORLD);
				}	
			}
			else if(imgInfo.flag == 1){ 
				for(i = 2; i < imgInfo.size-2; i++){
					MPI_Send(newImg[i], 3*imgInfo.columns, MPI_BYTE, MASTER , rank*10 + 1, MPI_COMM_WORLD);
				}
			}
			else{
				for(i = 2; i < imgInfo.size; i++){
					MPI_Send(newImg[i], 3*imgInfo.columns, MPI_BYTE, MASTER , rank*10 + 1, MPI_COMM_WORLD);
				}
			}
			/* liberando memória */
			for(i = 0; i < imgInfo.size; i++){
				free(img[i]);
				free(newImg[i]);
			}
			free(img);
			free(newImg);
		}
		MPI_Finalize();
		return 0;	
	}

	/* função que retorna a média de uma componente do pixel utilizando os valores da componente ao redor dela (numa sub matriz 5x5) */
	unsigned char avg (rgb **image, int x, int y, int cols, int rows, int flag){
		int i, j;
		unsigned char avg;
		int sum = 0, count = 0;
		for(i = y-2; i < y+2; i++){
			for(j = x-2; j < x+2; j++){
				if((j < 0 || j > cols-1) || (i < 0 || i > rows-1));
				else{		
					if (flag == 1)		
						sum += image[i][j].r;
					else if (flag == 2)		
						sum += image[i][j].g;
					else if (flag == 3)
						sum += image[i][j].b;
					count++;
				}
			}
		}
		avg = sum/count;
		return avg;
	}
