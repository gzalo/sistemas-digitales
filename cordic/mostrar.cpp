#include <fstream>
#include <algorithm>
#include <vector>
#include <set>
#include <cstring>
#include <iostream>
#include <cmath>
#include <SDL2/SDL.h>
using namespace std;

struct punto{
	double x,y,z;
};
struct punto16bits{
	int16_t x,y,z;
	
	const bool operator == ( const punto16bits &r ) const{
        return ( x == r.x && y == r.y && z==r.z );
    }
	const bool operator < ( const punto16bits &r ) const{
		if(x == r.x){
			if(y == r.y){
				return z < r.z;
			}else{
				return y < r.y;
			}
		}else{
			return x < r.x;
		}
    }
};
vector <punto> puntos;
vector <punto16bits> puntos_16;

punto rotar2D(punto p){
	return (punto){p.x*cos(p.z)+p.y*sin(p.z), -p.x*sin(p.z)+p.y*cos(p.z),0};
}

int16_t escalarCordic(int16_t e){
	return (e>>1)+(e>>4)+(e>>5);
}
#define LARGO_CORDIC 9
int16_t tablaCordic[LARGO_CORDIC];

punto16bits cordic16(punto16bits p, int iteracionesCordic){
	#if 0
		//Version Casteada
		float ang = (float)p.z*M_PI*2.0/512;
		return (punto16bits){(float)p.x*cos(ang)+(float)p.y*sin(ang), -(float)p.x*sin(ang)+(float)p.y*cos(ang),0};
	#endif
	
	int cuadrante = p.z >> 9;
	p.z &= 0b111111111;
	//9 bits de ángulo (->7 bits entre 0 y 90º)
	
	if(cuadrante == 0) p = p;
	if(cuadrante == 1) p = (punto16bits){p.y,-p.x,p.z};
	if(cuadrante == 2) p = (punto16bits){-p.x,-p.y,p.z};
	if(cuadrante == 3) p = (punto16bits){-p.y,p.x,p.z};
	
	punto16bits p_anterior = p;
	for(int i=0;i<iteracionesCordic;i++){	
		if(p_anterior.z<0){
			//D = -1
			p.x = p_anterior.x - (p_anterior.y >> (i));
			p.y = p_anterior.y + (p_anterior.x >> (i));
			p.z = p_anterior.z + tablaCordic[i];
		}else{
			p.x = p_anterior.x + (p_anterior.y >> (i));
			p.y = p_anterior.y - (p_anterior.x >> (i));
			p.z = p_anterior.z - tablaCordic[i];
		}
		#if 0
			cout << di << " " << i << " ";
			cout << di * tablaCordic[i] << "." << p.x << "." << p.y << "." << p.z << endl;
		#endif
	
		p_anterior = p;
	}
	return p;
}

punto16bits rotadorCordic16(int16_t *p_cuantiz, unsigned int *angulos, int iteracionesCordic){
	punto16bits c1 = cordic16((punto16bits){p_cuantiz[1],p_cuantiz[2],angulos[0]},iteracionesCordic);
	c1.x = escalarCordic(c1.x);
	c1.y = escalarCordic(c1.y);
	
	punto16bits c2 = cordic16((punto16bits){c1.y,p_cuantiz[0],angulos[1]},iteracionesCordic);
	c2.x = escalarCordic(c2.x);
	c2.y = escalarCordic(c2.y);
	
	punto16bits c3 = cordic16((punto16bits){c2.y,c1.x,angulos[2]},iteracionesCordic);
	c3.x = escalarCordic(c3.x);
	c3.y = escalarCordic(c3.y);
	
	return (punto16bits){c3.x,c3.y,c2.x};
}		

void calcularTablaCordic(){
	for(int j=0;j<LARGO_CORDIC;j++){
		tablaCordic[j] = round(4.0*512.0*atan(pow(2,-j))/(2.0*M_PI));
		printf("%d ", tablaCordic[j]);
	}
	cout << endl;
}

int main(int argc, char **args){
	if(argc != 2) return 1;
	
	calcularTablaCordic();
	fstream coords(args[1]/*"coordenadas.txt"*/);
	int pts;
	double prom[3];
	while(!coords.eof()){
		double x,y,z;
		coords >> x >> y >> z;
		
		prom[0] += x;
		prom[1] += x;
		prom[2] += x;
		pts ++;
		puntos.push_back((punto){x,y,z});
		puntos_16.push_back((punto16bits){x*32767.0/2.0,y*32767.0/2.0,z*32767.0/2.0});
	}
	set<punto16bits> puntos_16_s( puntos_16.begin(), puntos_16.end() );
	puntos_16.assign( puntos_16_s.begin(), puntos_16_s.end() );
	
	SDL_Init(SDL_INIT_VIDEO);

    SDL_Window * window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640*2, 480, 0);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640*2, 480);
	
	Uint32 *pixels = new Uint32[640*2 * 480];		
		
	bool fin = false;
	int tecX=0, tecY=0, tecZ=0;
	unsigned int angulos[3] = {0,0,0}; //Angulos iniciales
	int iteracionesCordic = LARGO_CORDIC;
	while(!fin){
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type == SDL_QUIT) fin = true;
			if(event.type == SDL_KEYDOWN){
				if(event.key.keysym.sym == SDLK_a) tecX = -1;
				if(event.key.keysym.sym == SDLK_d) tecX = +1;
				if(event.key.keysym.sym == SDLK_w) tecY = -1;
				if(event.key.keysym.sym == SDLK_s) tecY = +1;
				if(event.key.keysym.sym == SDLK_q) tecZ = -1;
				if(event.key.keysym.sym == SDLK_e) tecZ = +1;
				
				if(event.key.keysym.sym == SDLK_1) iteracionesCordic--;
				if(event.key.keysym.sym == SDLK_2) iteracionesCordic++;
				iteracionesCordic = min(LARGO_CORDIC, max(iteracionesCordic,1));
				
				if(event.key.keysym.sym == SDLK_ESCAPE) fin = true;
			}
			if(event.type == SDL_KEYUP){
				if(event.key.keysym.sym == SDLK_a) tecX = 0;
				if(event.key.keysym.sym == SDLK_d) tecX = 0;
				if(event.key.keysym.sym == SDLK_w) tecY = 0;
				if(event.key.keysym.sym == SDLK_s) tecY = 0;
				if(event.key.keysym.sym == SDLK_q) tecZ = 0;
				if(event.key.keysym.sym == SDLK_e) tecZ = 0;
				
				if(event.key.keysym.sym == SDLK_ESCAPE) fin = true;
			}
		}
		
		angulos[2] += 4*tecX;
		angulos[1] += 4*tecY;
		angulos[0] += 4*tecZ;
		angulos[0] &= 0b11111111111;
		angulos[1] &= 0b11111111111;
		angulos[2] &= 0b11111111111;
		
		char tit[128];
		sprintf(tit, "TP4 Sistemas Digitales - Ang:%3d %3d %3d It:%2d",angulos[0], angulos[1], angulos[2], iteracionesCordic);
		SDL_SetWindowTitle(window, tit);
		
		//Borrar pantalla
		memset(pixels, 255, 640*2 * 480 * sizeof(Uint32));
		
		//Rotacion ideal (floating point)
		for(int i=0;i<puntos.size();i++){
			punto p = puntos[i];
			
			int coords_f[2] = {128, 128};
			
			punto c1_f = rotar2D((punto){p.y,p.z,angulos[0]*M_PI*2.0/(512*4)});
			punto c2_f = rotar2D((punto){c1_f.y,p.x,angulos[1]*M_PI*2.0/(512*4)});
			punto c3_f = rotar2D((punto){c2_f.y,c1_f.x,angulos[2]*M_PI*2.0/(512*4)});
			
			coords_f[0] = (c3_f.y*160 + 320+640);
			coords_f[1] = (c2_f.x*160 + 240);
						
			if(coords_f[0] > 640+640-1) coords_f[0] = 640+640-1;
			if(coords_f[1] > 640+480-1) coords_f[1] = 640+480-1;
			if(coords_f[0] < 0) coords_f[0] = 0;
			if(coords_f[1] < 0) coords_f[1] = 0;
			
			pixels[coords_f[1] * 640*2 + coords_f[0]] = 0;
			
		}
		//Rotacion cordic
		for(int i=0;i<puntos_16.size();i++){			
			int16_t p_cuantiz[3] = {puntos_16[i].x,puntos_16[i].y,puntos_16[i].z};
			
			int coords[2] = {128, 128};
			
			//Rotacion cordic
			punto16bits rot = rotadorCordic16(p_cuantiz, angulos, iteracionesCordic);
			
			coords[0] = (rot.y*2*160/(32767) + 320);
			coords[1] = (rot.z*2*160/(32767) + 240);
						
			if(coords[0] > 640-1) coords[0] = 640-1;
			if(coords[1] > 480-1) coords[1] = 480-1;
			if(coords[0] < 0) coords[0] = 0;
			if(coords[1] < 0) coords[1] = 0;
			
			pixels[coords[1] * 640*2 + coords[0]] = 0;
		}
		
		SDL_Delay(16);
		SDL_UpdateTexture(texture, NULL, pixels, 640*2 * sizeof(Uint32));
		SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
	}
	
	SDL_DestroyWindow(window);
	SDL_Quit();
}

#if 0
	//TEST CORDIC NORMAL
	punto16bits ra = cordic16((punto16bits){0b0000000100000000,0b0000001000000000,0b00100000000});//45º ES 64
	printf("Final: %x %x %x\n", ra.x, ra.y, ra.z);
	
	//TEST CORDIC 3D
	int16_t vals[3] = {0x2133,0x1293,0x4827};
	unsigned int ang[3] = {0b00100000000,0b00100000000,0b00100000000};
	punto16bits r = rotador(vals, ang);
	printf("Final: %04x %04x %04x\n", r.x, r.y, r.z);
	
	//Clipping y mundo azul
	/*if(c3.x < 0)
	if(i<7988){
		pixels[coords[1] * 640 + coords[0]] = 0;
	}else{
		pixels[coords[1] * 640 + coords[0]] = 0x0000FF;
	}*/
	
#endif