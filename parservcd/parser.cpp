#include <cctype>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <vector>
#include <map>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
using namespace std;

#include "timing.h"

//Divisor resistivo 2k 1k 510, 75 ohm de carga, tensión blanco VGA 0,7v
//const uint8_t tablaColores8[8] = {0,35,71,107,140,175,211,247};
//const uint8_t tablaColores4[8] = {0,74,145,219};
//Con correccion gamma=1.2
const uint8_t tablaColores8[8] = {0,104,143,172,194,215,234,251};
const uint8_t tablaColores4[8] = {0,146,197,238};

//Convierte una tira de bits en su valor entero (MSB primero)
int bits(char *tira){
	int ret = 0;
	
	while(*tira)
		ret = ret<<1 | (*(tira++)=='1'?1:0);
	
	return ret;
}

const int anchoTotal = 800, altoTotal = 522;
char pantalla[anchoTotal*altoTotal*4];

char idR[4], idG[4], idB[4], idHS[4], idVS[4];

int timescale_val, timescale_unidad, modoRGB;

enum{
	TIMESCALE_FS=0,
	TIMESCALE_PS=1
};

void leerEncabezado(ifstream &in){
	bool finEncabezado = false;
	while(!finEncabezado){
		char v = in.get();
		
		if(isspace(v))
			continue;
		
		if(v=='$'){
			string nombre, contenido;
			
			in >> nombre;
			
			//Extraer caracteres hasta encontrar "$end"
			bool fin = false;
			char c;
			do{
				c = in.get();
				contenido += c;
				
				if(contenido.size()>3)
					if(contenido.substr(contenido.size()-4,4) == "$end") fin = true;
				
			}while(!fin);
			
			contenido = contenido.substr(0,contenido.size()-4);
			
			if(nombre == "var"){
				stringstream ss(contenido);
				
				int cantidadEspacios = count(contenido.begin(), contenido.end(), ' ');
				string tipo, nombre, identificador, indice;
				int largo;
				
				if(cantidadEspacios == 5){
					//reg 1 ! clk (Sintaxis GHDL)
					//wire 1 ! clk (Sintaxis Isim/Modelsim)
					ss >> tipo >> largo >> identificador >> nombre;
					if(nombre == "vs") strncpy(idVS, identificador.c_str(), 4);
					if(nombre == "hs") strncpy(idHS, identificador.c_str(), 4);

					if(nombre == "r[2:0]") strncpy(idR, identificador.c_str(), 4);
					if(nombre == "g[2:0]") strncpy(idG, identificador.c_str(), 4);
					if(nombre == "b[1:0]") strncpy(idB, identificador.c_str(), 4);
										
				}else if(cantidadEspacios == 6){
					//wire 1 ( r [2] (Sintaxis Isim/ModelSim)
					ss >> tipo >> largo >> identificador >> nombre >> indice;
					
					if(nombre == "r" && indice == "[2]") strncpy(idR, identificador.c_str(), 4);
					if(nombre == "g" && indice == "[2]") strncpy(idG, identificador.c_str(), 4);
					if(nombre == "b" && indice == "[1]") strncpy(idB, identificador.c_str(), 4);
					
					if(nombre == "r" || nombre == "g" || nombre == "b")
						modoRGB = 1;
				}
				
			}else if(nombre == "version"){
				
			}else if(nombre == "timescale"){
				stringstream ss(contenido);
				string unidad;
				ss >> timescale_val >> unidad;
				
				if(unidad == "fs")
					timescale_unidad = TIMESCALE_FS;
				else if(unidad == "ps")	
					timescale_unidad = TIMESCALE_PS;
				else 
					cerr << "Unidad no soportada ("<<unidad<<") != fs." << endl;
				
				
			}else if(nombre == "scope"){
			}else if(nombre == "upscope"){
			}else if(nombre == "comment"){
			}else if(nombre == "date"){
			}else if(nombre == "enddefinitions"){
				finEncabezado = true;
			}else{
				cerr << "Definicion no parseada: " << nombre << ":" << contenido << endl;
			}
		}else{
			cerr << "Caracter inesperado: " << v << endl;
		}
	}
	
	cout << "Identificadores encontrados: " << endl;
	cout << "VS:" << idVS << " HS:" << idHS << endl; 
	cout << "R:" << idR << " G:" << idG << " B:" << idB << endl; 
}

int main(int argc,char**argv){	
	if(argc != 2)
		return 0;
	
	uint64 t0 = GetTimeMs64();
	
	ifstream in(argv[1]);
	
	in.seekg(0, ios::end);
	size_t largo = in.tellg();
	in.seekg(0, ios::beg);
	
	leerEncabezado(in);
	size_t leido = in.tellg();
	
	vector<char> buffer(largo-leido);
	in.read(buffer.data(), largo-leido); //Leer los datos propiamente dichos
	
	in.close();
	
	cout << "Lectura: " << GetTimeMs64()-t0 << endl;
	t0 = GetTimeMs64();
	
	unsigned long long tiempo = 0;
	char vsPrev = '0', hsPrev = '0';
	
	unsigned long long tiempoLineaActual = 0;
	int lineaActual = 0;
	
	unsigned char colorActual[3] = {0,0,0};
	
	for(int i=0;i<anchoTotal*altoTotal;i++)
		pantalla[i*4+3] = 0xFF; //Pixeles opacos
	
	char v;
	int byte = 0;
	int imagenOut = 0;
	while(byte < largo-leido){
		v = buffer[byte++];
		
		if(isspace(v)){
		}else if(isdigit(v) || v=='b' || v=='Z' || v=='U'){
			if(v != 'b'){				
				int l = 0;
				while(!isspace(buffer[byte+l]))
					l++;
				
				char variable[4];
				strncpy(variable, (const char *)&buffer[byte], l);
				variable[l] = 0;
				byte += l;
				
				if(!strcmp(variable, idHS)){
					if(v == '0' && hsPrev == '1'){
						lineaActual++;
						//cout << "Comienzo de linea " << lineaActual << endl;
						tiempoLineaActual = tiempo;
					}
					hsPrev = v;
				}else if(!strcmp(variable, idVS)){
					if(v == '0' && vsPrev == '1' && lineaActual >= 520){
						cout << "Fin de pantalla (" << lineaActual << " lineas anteriores)" << endl;
						lineaActual = -1;
						char filename[32];
						snprintf(filename, 32, "salida%2d.png", imagenOut++);
						stbi_write_png(filename, anchoTotal, altoTotal, 4, pantalla, anchoTotal*4);
					}
					vsPrev = v;
				}else if(!strcmp(variable, idR) && modoRGB){
					colorActual[0] = v=='1'?tablaColores8[7]:tablaColores8[0];
				}else if(!strcmp(variable, idG) && modoRGB){
					colorActual[1] = v=='1'?tablaColores8[7]:tablaColores8[0];
				}else if(!strcmp(variable, idB) && modoRGB){
					colorActual[2] = v=='1'?tablaColores4[3]:tablaColores4[0];
				}
			}else{
				int l = 0;
				while(!isspace(buffer[byte+l]))
					l++;
			
				char valores[16];
				strncpy(valores, (const char *)&buffer[byte], l);
				valores[l] = 0;
				byte += l;
				
				byte++; //Salteo al espacio
								
				l = 0;
				while(!isspace(buffer[byte+l]))
					l++;
							
				char variable[4];
				strncpy(variable, (const char *)&buffer[byte], l);
				variable[l] = 0;
				byte += l;
				
				if(!strcmp(variable,idR)){
					colorActual[0] = tablaColores8[bits(valores)];
				}else if(!strcmp(variable,idG)){
					colorActual[1] = tablaColores8[bits(valores)];
				}else if(!strcmp(variable,idB)){
					colorActual[2] = tablaColores4[bits(valores)];
				}
			}
		}else if(v=='#'){		
			//Pintar el pixel que acaba de terminar
			int x;
			if(timescale_unidad == TIMESCALE_FS)
				x = (tiempo-tiempoLineaActual)/40000000; //Cada pixel son 40ns (40000000 femtosegundos)
			else 
				x = (tiempo-tiempoLineaActual)/40000; //Cada pixel son 40ns (40000 picosegundos)
			
			int idx = (lineaActual*anchoTotal+x)*4;
				
			if(idx > anchoTotal*altoTotal*4 || idx < 0){
				//cout << "Fuera de rango: " << x << ":" << lineaActual << endl;
			}else{
				pantalla[idx+0] = colorActual[0];
				pantalla[idx+1] = colorActual[1];
				pantalla[idx+2] = colorActual[2];
			}
			//Cargar el nuevo tiempo
			tiempo = 0;
			while(!isspace(buffer[byte])){
				tiempo = tiempo*10 + (buffer[byte]-'0');
				byte++;
			}
			byte++;
			
		}else{
			cout << v;
		}
	}	
	cout << "Final: " << GetTimeMs64()-t0 << endl;
	system("pause");
	return 0;
}