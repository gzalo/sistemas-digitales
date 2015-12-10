#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <stdio.h>      
#include <stdlib.h>
#include <string.h>     
#include <unistd.h>    
#include <fcntl.h>      
#include <errno.h>      
#include <cstdio>
using namespace std;

#ifdef WIN32
	#include <windows.h>
	
	HANDLE serialInit(const char *puerto){
	//Apertura de puerto
	HANDLE hSerial = CreateFile(puerto,GENERIC_READ | GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if(hSerial==INVALID_HANDLE_VALUE){
		if(GetLastError()==ERROR_FILE_NOT_FOUND){
			cerr << "Error abriendo puerto serie." << endl;
			return 0;
		}
		cerr << "Error de handle." << endl;
		return 0;
	}
	
	//Establecimiento de tasa, bits de stop y paridad
	DCB dcbSerialParams = {0};
	dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams)) {
		cerr << "Error obteniendo parametros." << endl;
		return 0;
	}
	dcbSerialParams.BaudRate=CBR_115200;	 	//Tasa de bits (115200bps)
	dcbSerialParams.ByteSize=8;
	dcbSerialParams.StopBits=ONESTOPBIT;
	dcbSerialParams.Parity=NOPARITY;
	if(!SetCommState(hSerial, &dcbSerialParams)){
		cerr << "Error estableciendo parametros." << endl;
		return 0;
	}
	
	//Establecimiento de timeouts
	COMMTIMEOUTS timeouts={0};
	timeouts.ReadIntervalTimeout=1000;
	timeouts.ReadTotalTimeoutConstant=1000;
	timeouts.ReadTotalTimeoutMultiplier=1000;
	if(!SetCommTimeouts(hSerial, &timeouts)){
		cerr << "Error estableciendo timeout." << endl;
		return 0;
	}
	
	return hSerial;
}
	
#else
	#include <termios.h>
    #define uint16_t unsigned short
	
int serialInit(char *name){
    int fd = open( name, O_RDWR| O_NOCTTY );
    
    struct termios tty;
    struct termios tty_old;
    memset (&tty, 0, sizeof tty);

    /* Error Handling */
    if ( tcgetattr ( fd, &tty ) != 0 ) {
       std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }

    /* Save old tty parameters */
    tty_old = tty;

    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)B115200);
    cfsetispeed (&tty, (speed_t)B115200);

    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS;           // no flow control
    tty.c_cc[VMIN]   =  1;                  // read doesn't block
    tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tty);

    /* Flush Port, then applies attributes */
    tcflush( fd, TCIFLUSH );
    if ( tcsetattr ( fd, TCSANOW, &tty ) != 0) {
       std::cout << "Error " << errno << " from tcsetattr" << std::endl;
    }
    
    return fd;
};

#define HANDLE int

#endif


struct punto16bits{
	int16_t x,y,z;

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
int main(int argc, char **args){
	ifstream coords("coordenadas.txt");
	
	vector <punto16bits> vecPuntos;
	set <punto16bits> setPuntos;
	
	int puntosTotales = 0;

	while(!coords.eof()){
		double x,y,z;
		coords >> x >> y >> z;
		
		//Conversión de (-1; 1) a (-32767; 32767)		
		x *= 32767.0;
		y *= 32767.0; //Flipear para que quede el mundo orientado correctamente
		z *= -32767.0;
		
		//Escalado extra (para evitar overflow en primera etapa CORDIC)
		x /= 3.15; 
		y /= 3.15;
		z /= 3.15;
		
		int16_t x_t=x, y_t=y, z_t=z;
		
		//Evitar que se detecte como final
		if(x_t == 0) x_t = 1;
		if(y_t == 0) y_t = 1;
		if(z_t == 0) z_t = 1;
		
		setPuntos.insert((punto16bits){x_t,y_t,z_t});
		puntosTotales++;
    }	
	vecPuntos.assign(setPuntos.begin(), setPuntos.end());
	//Punto para marcar finalización
	vecPuntos.push_back((punto16bits){0,0,0});
	
	ofstream out("coordsfinales.txt");
	for(size_t i=0;i<vecPuntos.size();i++)
		//if(rand()%8 == 0 || i == vecPuntos.size()-1) Descomentar para enviar solo 1/8 de todos los puntos
			out << (uint16_t)vecPuntos[i].x << endl << (uint16_t)vecPuntos[i].y << endl << (uint16_t)vecPuntos[i].z << endl;
	
	HANDLE hSerial;
	if((hSerial = serialInit("/dev/ttyS0")) == 0) return 1;
	
	cout << "Cargados " << puntosTotales << " puntos, " << vecPuntos.size() << " sin repetidos." << endl;
	
	cout << "Enviando puntos..." << endl;
	//Enviar los puntos via puerto serie (little endian, primero las partes bajas)
	//Xbajo Xalto Ybajo Yalto Zbajo Zalto
	#ifdef WIN32
		DWORD bytesEnviados;
		WriteFile(hSerial, vecPuntos.data(), vecPuntos.size()*sizeof(punto16bits), &bytesEnviados, NULL);
    #else
		int bytesEnviados = write( hSerial, vecPuntos.data(), vecPuntos.size()*sizeof(punto16bits));
	#endif
	
	cout << "Enviados " << bytesEnviados << " bytes." << endl;
	
	#ifdef WIN32
		CloseHandle(hSerial);
	#else
		close(hSerial);
	#endif
	
	return 0;
}
