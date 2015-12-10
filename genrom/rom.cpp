#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
   
int main(){
	FILE *fp = fopen("fuente32.png", "rb");
	
	//Cada fila se mappea en una dirección distinta en la memoria
	//Se asume que la imagen es monocromática, si no lo es se hace un umbral en 128
	
	if(!fp) return -1;
	
	int w,h,comp;
	unsigned char *data = stbi_load_from_file(fp,&w,&h,&comp,0);
	
	printf("subtype tipoLinea is std_logic_vector(0 to %d);\n",w-1);
	printf("type memo is array(0 to %d) of tipoLinea;\n",h-1);
	printf("signal RAM: memo:= (\n");
		
	for(int y=0;y<h;y++){
		printf("%2d => \"", y);
		
		for(int x=0;x<w;x++){
			if(data[(x+y*w)*comp] > 128)
				printf("1");
			else 
				printf("0");
		}
		if(y == h-1)
			printf("\");");
		else
			printf("\",\t");					
		
		if(y%w == (w-1))
			printf("\n");
	}
	
	stbi_image_free(data);
	return 0;
}
