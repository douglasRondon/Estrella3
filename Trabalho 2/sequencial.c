#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
}rgb;
unsigned char avg (unsigned char **image, int x, int y, int cols, int rows);

int main (){
	int bla = 0;
	FILE *file;
	int i, j, rows, columns, max;
	unsigned char **r, **g, **b, **newR, **newG, **newB ;
    clock_t cInit, cFinal;

	file = fopen("gray/ppm/3.ppm", "rb");
	fseek(file, 2, SEEK_SET);
	fscanf(file, "%d", &columns);
	fscanf(file, "%d", &rows);
	/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de entrada*/
	r = (unsigned char**) malloc (rows*sizeof(unsigned char*));
	g = (unsigned char**) malloc (rows*sizeof(unsigned char*));
	b = (unsigned char**) malloc (rows*sizeof(unsigned char*));

	/* alocando memória para a matriz que irá armazenar as componentes r,g e b da imagem de saída*/
	newR = (unsigned char**) malloc (rows*sizeof(unsigned char*));
	newG = (unsigned char**) malloc (rows*sizeof(unsigned char*));
	newB = (unsigned char**) malloc (rows*sizeof(unsigned char*));

	for(i = 0; i < rows; i++){
		r[i] = (unsigned char*)malloc(columns*sizeof(unsigned char));
		g[i] = (unsigned char*)malloc(columns*sizeof(unsigned char));
		b[i] = (unsigned char*)malloc(columns*sizeof(unsigned char));

		newR[i] = (unsigned char*)malloc(columns*sizeof(unsigned char));
		newG[i] = (unsigned char*)malloc(columns*sizeof(unsigned char));
		newB[i] = (unsigned char*)malloc(columns*sizeof(unsigned char));
	}
	
	fscanf(file,"%d\n",&max);
	/* lendo a imagem do arquivo de entrada para a matriz */
	for(i = 0; i < rows; i++){
		for(j = 0; j < columns; j++){
			fread(&r[i][j],sizeof(unsigned char),1,file);
			fread(&g[i][j],sizeof(unsigned char),1,file);
			fread(&b[i][j],sizeof(unsigned char),1,file);
		}
	}
    cInit = clock();    
	/* fazendo cálculo de cada novo pixel para r, g e b */
	for(i = 0; i < rows; i++){
		for(j = 0; j < columns; j++){
			newR[i][j] = avg(r, j, i,columns,rows);
		}
	}
	for(i = 0; i < rows; i++){
		for(j = 0; j < columns; j++){
			newG[i][j ]= avg(g, j, i,columns,rows);
		}
	}
	for(i = 0; i < rows; i++){
		for(j = 0; j < columns; j++){
			newB[i][j] = avg(b, j, i,columns,rows);
		}
	}
    cFinal = clock();
    printf("Tempo: %lf segundos\n", (double)(cFinal - cInit) / CLOCKS_PER_SEC);
	fclose(file);
	/* criando arquivo de saída e escrevendo nele a nova imagem */
	file = fopen("out.ppm", "wb");
	fprintf(file, "P6\n");
	fprintf(file, "%d %d\n",columns,rows);
	fprintf(file, "%d\n",max);
	for(i = 0; i < rows; i++){
		for(j = 0; j < columns; j++){
			fwrite(&newR[i][j] ,sizeof(unsigned char),1,file);
			fwrite(&newG[i][j] ,sizeof(unsigned char),1,file);
			fwrite(&newB[i][j] ,sizeof(unsigned char),1,file);
		}
	}
	fclose(file);
	/* liberando a memória utilizada */ 
	for(i = 0; i < rows; i++){
		free(r[i]);
		free(g[i]);
		free(b[i]);
		free(newR[i]);
		free(newG[i]);
		free(newB[i]);
	}
	free(r);
	free(g);
	free(b);
	free(newR);
	free(newG);
	free(newB);
}

/* função que retorna a média de uma componente do pixel utilizando os valores da componente ao redor dela (numa sub matriz 5x5) */
unsigned char avg (unsigned char **image, int x, int y, int cols, int rows){
	int i, j;
	unsigned char avg;
	int sum = 0, count = 0;
	for(i = y-2; i < y+2; i++){
		for(j = x-2; j < x+2; j++){
			if((j < 0 || j > cols-1) || (i < 0 || i > rows-1));
			else{				
				sum += image[i][j];
				count++;
			}
		}
	}
	avg = sum/count;
	return avg;
}
