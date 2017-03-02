#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#pragma pack(1)				// "Compresse" la structure en m�moire, utile pour avoir la VRAIE taille du "header"
#include "photocheap.h"		// Header, contient les definitions des fonctions et structure


/***************************
*	 PIXEL MANIPULATION	   *
***************************/

	/****************
	 RECUPERER PIXEL
	****************/
void setPixel(BMP* bmp, int i, int j, Pixel p)
{
	assert(bmp && i>=0 && i<bmp->width && j>=0 && j<bmp->height);
	bmp->data[bmp->width*j+i] = p;
}

	/***************
	 ASSIGNER PIXEL
	***************/
Pixel getPixel(BMP* bmp, int i, int j)
{
	assert(bmp && i>=0 && i<bmp->width && j>=0 && j<bmp->height);
	return bmp->data[bmp->width*j+i];
}



/***************************
*	CREER CHARGER SAUVER   *
***************************/

	/************
	 CREER IMAGE
	************/
BMP* newBMP(int width, int height)
{
	BMP* bmp 	= malloc(sizeof(BMP));
	bmp->width 	= width;
	bmp->height = height;
	bmp->data 	= calloc(1, width*height*sizeof(Pixel*));
	return bmp;
}

	/**************
	 CHARGER IMAGE
	**************/
BMP* loadBMP(const char* filename)
{

	Header 			header;
	Pixel 			p;
	int 			i, j, padding;
	unsigned char 	bgrpix[3];
	char 			corr[4] = {0,3,2,1};
	BMP* 			bmp;

	FILE* bmp_input = fopen(filename, "rb");
	if(!bmp_input)
	{
		printf("\nOuverture du fichier \"%s\" impossible.", filename);
		exit(1);
	}
	fread(&header, sizeof(Header), 1, bmp_input);

	bmp 	= newBMP(header.InfoHeader.Width, header.InfoHeader.Height);
	padding = corr[(3*header.InfoHeader.Width)%4];		// Une ligne de pixel doit TOUJOURS a voir un nombre d'octet multiple de 4.

	fseek(bmp_input, header.Offset, SEEK_SET);

	for(j=0; j<bmp->height; j++)
	{
		for(i=0; i<bmp->width; i++)
		{
			fread(&bgrpix, 1, 3, bmp_input);
			p.Red 	= bgrpix[2];
			p.Green = bgrpix[1];
			p.Blue 	= bgrpix[0];
			setPixel(bmp, i, bmp->height-j-1, p);
		}
		fread(&bgrpix, 1, padding, bmp_input);
	}
	fclose(bmp_input);
	return bmp;
}

	/*************
	 SAUVER IMAGE
	*************/
int saveBMP(BMP* bmp, const char* filename)
{
	Header 			header;
	Pixel			p;
	int 			i, j, padding, dataSize;
	unsigned char 	bgrpix[3];
	char 			corr[4] = {0,3,2,1};

	FILE* bmp_output = fopen(filename, "wb");
	if(!bmp_output)
	{
		printf("\nCreation du fichier \"%s\" impossible.", filename);
		exit(1);
	}
	memset(&header, 0, sizeof(Header));

	header.Type[0] 				= 'B';
	header.Type[1] 				= 'M';
	header.Offset 				= sizeof(Header);
	header.InfoHeader.Size 		= sizeof(InfoHeader);
	header.InfoHeader.Width 	= bmp->width;
	header.InfoHeader.Height 	= bmp->height;
	header.InfoHeader.Planes 	= 1;
	header.InfoHeader.Bits 		= 24;
	padding 					= corr[(3*header.InfoHeader.Width)%4];
	dataSize 					= 3*header.InfoHeader.Height*header.InfoHeader.Width + header.InfoHeader.Height*padding;
	header.InfoHeader.ImageSize = dataSize;
	header.Size 				= header.Offset + header.InfoHeader.ImageSize;

	fwrite(&header, sizeof(Header), 1, bmp_output);

	for(j=0; j<bmp->height; j++)
	{
		for(i=0; i<bmp->width; i++)
		{
			p = getPixel(bmp, i, bmp->height-j-1);
			bgrpix[0] = p.Blue;
			bgrpix[1] = p.Green;
			bgrpix[2] = p.Red;
			fwrite(&bgrpix, 1, 3, bmp_output);
		}
		bgrpix[0] = bgrpix[1] = bgrpix[2] = 255;
		fwrite(&bgrpix, 1, padding, bmp_output);
	}
	fclose(bmp_output);
	return 0;
}



/***************************
*		MODIFICATION	   *
***************************/

	/***************
	 NIVEAU DE GRIS
	***************/
BMP* greyScale(BMP* bmp)
{
	unsigned char grey;
	Pixel p;
	int i, j;

	for(i=0; i<bmp->width; i++)
	{
		for(j=0; j<bmp->height; j++)
		{
			p 		= getPixel(bmp, i, j);
			grey 	= p.Red*0.2125 + p.Green*0.587 + p.Blue*0.114;
			p.Blue 	= p.Red = p.Green = grey;
			setPixel(bmp, i, j, p);
		}
	}
	return bmp;
}


	/********
	 NEGATIF
	********/
BMP* invert(BMP* bmp)
{
	int i, j;
	Pixel p;

	for(i=0; i<bmp->width; i++)
	{
		for(j=0; j<bmp->height; j++)
		{
			p 		= getPixel(bmp, i, j);
			p.Red 	= 255-p.Red;
			p.Green = 255-p.Green;
			p.Blue 	= 255-p.Blue;
			setPixel(bmp, i, j, p);
		}
	}
	return bmp;
}


	/*****************
	 CONTOURS - SOBEL
	*****************/
BMP* sobel(BMP* bmp)
{
	bmp 		= greyScale(bmp);
	BMP* border = newBMP(bmp->width, bmp->height);
	Pixel 		p;
	int 		sobel_x[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
	int 		sobel_y[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};
	int 		i, j, px, py;

	for(i=1; i<bmp->width-1; i++)
	{
		for(j=1; j<bmp->height-1; j++)
		{
			px = 	(sobel_x[0][0] * getPixel(bmp,i-1,j-1).Red) + (sobel_x[0][1] * getPixel(bmp,i,j-1).Red) + (sobel_x[0][2] * getPixel(bmp,i+1,j-1).Red) 	+
					(sobel_x[1][0] * getPixel(bmp,i-1,j).Red)   + (sobel_x[1][1] * getPixel(bmp,i,j).Red)   + (sobel_x[1][2] * getPixel(bmp,i+1,j).Red) 	+
              		(sobel_x[2][0] * getPixel(bmp,i-1,j+1).Red) + (sobel_x[2][1] * getPixel(bmp,i,j+1).Red) + (sobel_x[2][2] * getPixel(bmp,i+1,j+1).Red);

			py = 	(sobel_y[0][0] * getPixel(bmp,i-1,j-1).Red) + (sobel_y[0][1] * getPixel(bmp,i,j-1).Red) + (sobel_y[0][2] * getPixel(bmp,i+1,j-1).Red) 	+
              		(sobel_y[1][0] * getPixel(bmp,i-1,j).Red)   + (sobel_y[1][1] * getPixel(bmp,i,j).Red)   + (sobel_y[1][2] * getPixel(bmp,i+1,j).Red) 	+
              		(sobel_y[2][0] * getPixel(bmp,i-1,j+1).Red) + (sobel_y[2][1] * getPixel(bmp,i,j+1).Red) + (sobel_y[2][2] * getPixel(bmp,i+1,j+1).Red);
			p.Red = p.Green = p.Blue = (unsigned char)sqrt((px * px) + (py * py));
			setPixel(border, i, j, p);
		}
	}
	return border;
}


	/******************
	 CONTOURS - PEWITT
	*******************/
BMP* pewitt(BMP* bmp)
{
	bmp 	= greyScale(bmp);
	BMP* 	border = newBMP(bmp->width, bmp->height);
	Pixel 	p;
	int 	pewitt_x[3][3] = {{-1,0,1},{-1,0,1},{-1,0,1}};
	int 	pewitt_y[3][3] = {{-1,-1,-1},{0,0,0},{1,1,1}};
	int 	i, j, px, py;

	for(i=1; i<bmp->width-1; i++)
	{
		for(j=1; j<bmp->height-1; j++)
		{
			px = 	(pewitt_x[0][0] * getPixel(bmp,i-1,j-1).Red) + (pewitt_x[0][1] * getPixel(bmp,i,j-1).Red) + (pewitt_x[0][2] * getPixel(bmp,i+1,j-1).Red) 	+
					(pewitt_x[1][0] * getPixel(bmp,i-1,j).Red)   + (pewitt_x[1][1] * getPixel(bmp,i,j).Red)   + (pewitt_x[1][2] * getPixel(bmp,i+1,j).Red) 	+
              		(pewitt_x[2][0] * getPixel(bmp,i-1,j+1).Red) + (pewitt_x[2][1] * getPixel(bmp,i,j+1).Red) + (pewitt_x[2][2] * getPixel(bmp,i+1,j+1).Red);

			py = 	(pewitt_y[0][0] * getPixel(bmp,i-1,j-1).Red) + (pewitt_y[0][1] * getPixel(bmp,i,j-1).Red) + (pewitt_y[0][2] * getPixel(bmp,i+1,j-1).Red) 	+
              		(pewitt_y[1][0] * getPixel(bmp,i-1,j).Red)   + (pewitt_y[1][1] * getPixel(bmp,i,j).Red)   + (pewitt_y[1][2] * getPixel(bmp,i+1,j).Red) 	+
              		(pewitt_y[2][0] * getPixel(bmp,i-1,j+1).Red) + (pewitt_y[2][1] * getPixel(bmp,i,j+1).Red) + (pewitt_y[2][2] * getPixel(bmp,i+1,j+1).Red);
			p.Red = p.Green = p.Blue = (unsigned char)sqrt((px * px) + (py * py));
			setPixel(border, i, j, p);
		}
	}
	return border;
}


/***************************
*		STATISTIQUES	   *
***************************/

	/************
	 HISTOGRAMME
	************/
void histogram(BMP* bmp)
{
	int 	i, j, k;
	int		MAXR=0, MAXG=0, MAXB=0, MAX=0;
	int		histo[3][256] = {{0}}; // ligne 0->red -- ligne 1->green -- ligne 3->blue
	Pixel 	p, greyp;
	BMP* 	histoR;
	BMP*	histoG;
	BMP*	histoB;
	BMP*	test;

	// Remplissage de l'histogramme et recherche du plus grand nombre de pixel de la meme couleur
	for(i=0; i<bmp->width; i++)
	{
		for(j=0; j<bmp->height; j++)
		{
			p = getPixel(bmp, i, j);

			histo[0][p.Red]++;
			histo[1][p.Green]++;
			histo[2][p.Blue]++;

			if( histo[0][p.Red]		> MAXR ) 	MAXR = histo[0][p.Red];
			if( histo[1][p.Green] 	> MAXG ) 	MAXG = histo[1][p.Green];
			if( histo[2][p.Blue] 	> MAXB ) 	MAXB = histo[2][p.Blue];
		}
	}

	// Cr�ation de l'image histogramme
	histoR = newBMP(256,200);
	histoG = newBMP(256,200);
	histoB = newBMP(256,200);

	FILE* f1 = fopen("histogram.txt", "w");

	for(i=0; i<256; i++)
	{
		fprintf(f1, "%d	%d	%d	%d\n",i, (histo[0][i]*200/MAXR), (histo[1][i]*200/MAXG), (histo[2][i]*200/MAXB) ) ;
		for(j=199; j>=0; j--) // rouge
		{
			if ( histo[0][i]*200/MAXR > j)
				setPixel(histoR, i, 199-j, RED);
			else
				setPixel(histoR, i, 199-j, WHITE);

			if ( histo[1][i]*200/MAXG > j)
				setPixel(histoG, i, 199-j, GREEN);
			else
				setPixel(histoG, i, 199-j, WHITE);

			if ( histo[2][i]*200/MAXB > j)
				setPixel(histoB, i, 199-j, BLUE);
			else
				setPixel(histoB, i, 199-j, WHITE);
		}
	}

	fclose(f1);
	saveBMP(histoR, "histogram_Red.bmp");
	saveBMP(histoG, "histogram_Green.bmp");
	saveBMP(histoB, "histogram_Blue.bmp");

}

int main()
{
	BMP* 	J = loadBMP("parrot.bmp");
	histogram(J);
	return 0;
}