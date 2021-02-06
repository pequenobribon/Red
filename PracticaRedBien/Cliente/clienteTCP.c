#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "imagen.h"

#define PUERTO 5000
#define TAM_BUFFER 100
#define DIR_IP "192.168.100.117"

unsigned char * reservarMemoria(int nBytes);
void GrayToRGB2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void RGBToGray2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void GrayToRGB3(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void recibirImagen(int sfd, unsigned char *imagenGray, int longitud);

unsigned char *imagenRGB, *imagenGray;
	bmpInfoHeader info;

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in direccion_servidor;
	//char leer_mensaje[TAM_BUFFER];
	
/*	
 *	AF_INET - Protocolo de internet IPV4
 *  htons - El ordenamiento de bytes en la red es siempre big-endian, por lo que
 *  en arquitecturas little-endian se deben revertir los bytes
 */	
	memset (&direccion_servidor, 0, sizeof (direccion_servidor));
	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_port = htons(PUERTO);
/*	
 *	inet_pton - Convierte direcciones de texto IPv4 en forma binaria
 */	
	if( inet_pton(AF_INET, DIR_IP, &direccion_servidor.sin_addr) <= 0 )
	{
		perror("Ocurrio un error al momento de asignar la direccion IP");
		exit(1);
	}
/*
 *	Creacion de las estructuras necesarias para el manejo de un socket
 *  SOCK_STREAM - Protocolo orientado a conexión
 */
	printf("Creando Socket ....\n");
	if( (sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("Ocurrio un problema en la creacion del socket");
		exit(1);
	}
/*
 *	Inicia el establecimiento de una conexion mediante una apertura
 *	activa con el servidor
 *  connect - ES BLOQUEANTE
 */
	printf ("Estableciendo conexion ...\n");
	if( connect(sockfd, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor) ) < 0) 
	{
		perror ("Ocurrio un problema al establecer la conexion");
		exit(1);
	}
//RECIBIMOS IMAGEN DEL SERVIDOR
    if( read (sockfd, &info, sizeof(bmpInfoHeader)) < 0 )
	{
		perror ("Ocurrio algun problema al recibir datos del cliente");
		exit(1);
   	}
    

    imagenGray = reservarMemoria(info.width * info.height);
    recibirImagen(sockfd,imagenGray,info.width * info.height);
    imagenRGB = reservarMemoria(info.width * info.height * 3);
    GrayToRGB2(imagenRGB, imagenGray, info.width, info.height);
    guardarBMP( "fotoProcesada.bmp", &info, imagenRGB );

//TERMINA RECEPCION DE IMAGEN
	printf("Abriendo imagen... \n");
	
	imagenRGB = abrirBMP( "fotoProcesada.bmp", &info );
	displayInfo( &info );

	imagenGray = reservarMemoria( info.width*info.height );
	RGBToGray2( imagenRGB, imagenGray, info.width, info.height );

/*
 *	Inicia la transferencia de datos entre cliente y servidor
 */
	
	
	printf ("Cerrando la aplicacion cliente\n");
/*
 *	Cierre de la conexion
 */
//Aqui recibimos la imagen ya Procesada por el servidor

        
	close(sockfd);
	free( imagenRGB );
	free( imagenGray );

	return 0;
}

unsigned char *reservarMemoria(int nBytes){
	unsigned char *imagen;

	imagen = (unsigned char*)malloc(nBytes*sizeof(char));
	if(imagen == NULL){
		perror("Error al asignar memoria");
		exit(EXIT_FAILURE );
	}
	return imagen;
}

void GrayToRGB2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height){
	register int i;
	int nBytesImagen, indiceGray;

	indiceGray = 0;
	nBytesImagen = width*height*3;
	for(i = 0; i < nBytesImagen; i += 3, indiceGray++){
		imagenRGB[i] = imagenGray[indiceGray];
		imagenRGB[i+1] = imagenGray[indiceGray];
		imagenRGB[i+2] = imagenGray[indiceGray];
	}
}

void RGBToGray2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height){
	register int i;
	unsigned char nivelGris;
	int nBytesImagen, indiceGray;

	nBytesImagen = width*height*3;
	indiceGray = 0;
	for(i = 0; i < nBytesImagen; i += 3, indiceGray++){
		nivelGris = (imagenRGB[i] + imagenRGB[i+1] + imagenRGB[i+2]) / 3;
		//nivelGris = (((imagenRGB[i]*30) + (imagenRGB[i+1]*59) + (imagenRGB[i+2]*11))/100) / 3;
		imagenGray[indiceGray] = nivelGris;
	}
}

void recibirImagen(int sfd, unsigned char *imagenGray, int longitud){
   int bytesFaltantes = longitud;
   int readBytes;

   while( bytesFaltantes > 0 ){
      readBytes = read (sfd, imagenGray, bytesFaltantes);
      if( readBytes < 0 ){
         perror ("Ocurrio algun problema al recibir datos del cliente");
         exit(1);
      }
      printf("Bytes recibidos: %d\n", readBytes);
      bytesFaltantes = bytesFaltantes-readBytes;
      imagenGray = imagenGray+readBytes;
   }
}

void GrayToRGB3(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height){
   register int i;
   int nBytesImagen, indiceGray;

   indiceGray = 0;
   nBytesImagen = width*height*3;
   for(i = 0; i < nBytesImagen; i += 3, indiceGray++){
      imagenRGB[i] = imagenGray[indiceGray];
      imagenRGB[i+1] = imagenGray[indiceGray];
      imagenRGB[i+2] = imagenGray[indiceGray];
   }
}
