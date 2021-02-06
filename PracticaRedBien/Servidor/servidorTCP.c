#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "imagen.h"
#include <pthread.h>
#include <math.h>
#define NUM_HILOS 4

#define PUERTO 			5000	//Número de puerto asignado al servidor
#define COLA_CLIENTES 	5 		//Tamaño de la cola de espera para clientes
#define TAM_BUFFER 		100

unsigned char * reservarMemoria(int nBytes);
void GrayToRGB2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void RGBToGray2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void recibirImagen(int sfd, unsigned char *imagenGray, int longitud);
void calcularDetectorBordes(unsigned char *imagenGray,unsigned char *imagenYn, uint32_t width, uint32_t height,int iniBloque,int finBloque);
void *funHilo(void * arg);
unsigned char *imagenRGB, *imagenGray, *imagenYn;
int bloque;
bmpInfoHeader info;

int main(int argc, char **argv)
{
   	int sockfd, cliente_sockfd;
   	struct sockaddr_in direccion_servidor; //Estructura de la familia AF_INET, que almacena direccion
   //	char leer_mensaje[TAM_BUFFER];
    register int nh;
    int nhs[NUM_HILOS], *res_nh;
    pthread_t tids[NUM_HILOS];
    
      
     
/*	
 *	AF_INET - Protocolo de internet IPV4
 *  htons - El ordenamiento de bytes en la red es siempre big-endian, por lo que
 *  en arquitecturas little-endian se deben revertir los bytes
 *  INADDR_ANY - Se elige cualquier interfaz de entrada
 */	
   	memset (&direccion_servidor, 0, sizeof (direccion_servidor));	//se limpia la estructura con ceros
   	direccion_servidor.sin_family 		= AF_INET;
   	direccion_servidor.sin_port 		= htons(PUERTO);
   	direccion_servidor.sin_addr.s_addr 	= INADDR_ANY;

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
 *  bind - Se utiliza para unir un socket con una dirección de red determinada
 */
   	printf("Configurando socket ...\n");
   	if( bind(sockfd, (struct sockaddr *) &direccion_servidor, sizeof(direccion_servidor)) < 0 )
	{
		perror ("Ocurrio un problema al configurar el socket");
		exit(1);
   	}
/*
 *  listen - Marca al socket indicado por sockfd como un socket pasivo, esto es, como un socket
 *  que será usado para aceptar solicitudes de conexiones de entrada usando "accept"
 *  Habilita una cola asociada la socket para alojar peticiones de conector procedentes
 *  de los procesos clientes
 */
   	printf ("Estableciendo la aceptacion de clientes...\n");
	if( listen(sockfd, COLA_CLIENTES) < 0 )
	{
		perror("Ocurrio un problema al crear la cola de aceptar peticiones de los clientes");
		exit(1);
   }



/*
 *  accept - Aceptación de una conexión
 *  Selecciona un cliente de la cola de conexiones establecidas
 *  se crea un nuevo descriptor de socket para el manejo
 *  de la nueva conexion. Apartir de este punto se debe
 *  utilizar el nuevo descriptor de socket
 *  accept - ES BLOQUEANTE 
 */
   	printf ("En espera de peticiones de conexión ...\n");
   	if( (cliente_sockfd = accept(sockfd, NULL, NULL)) < 0 )
	{
		perror("Ocurrio algun problema al atender a un cliente");
		exit(1);
   	}

/*
 *	Inicia la transferencia de datos entre cliente y servidor
 */

    //Tomamos foto
    //Comando para tomar la foto
    char comando[60];
    //
    strcpy( comando, "raspistill -n -t 500 -e bmp -w 640 -h 480 -o foto.bmp");    
    system(comando);
    //Abrimos imagen
    imagenRGB = abrirBMP("foto.bmp",&info);
    displayInfo( &info );
    
    imagenGray = reservarMemoria(info.width * info.height);
    RGBToGray2(imagenRGB, imagenGray, info.width, info.height);
    
    imagenYn = reservarMemoria( info.width * info.height ); 
    bloque = info.height / NUM_HILOS;

    //Aquí se procesa la imagen 
    for( nh = 0; nh < NUM_HILOS; nh++){
        nhs[nh] = nh;
        pthread_create( &tids[nh], NULL, funHilo, (void *)&nhs[nh] );
    }

    for( nh = 0; nh < NUM_HILOS; nh++ )
    {
        pthread_join( tids[nh], (void **)&res_nh );
        printf("Hilo %d terminado\n", *res_nh);
    }

    GrayToRGB2(imagenRGB, imagenYn, info.width, info.height);
    guardarBMP("fotoFiltrada.bmp",&info, imagenRGB);
    //TERMINA PROCESAMIENTO DE LA IMAGEN
   	printf("Se aceptó un cliente, atendiendolo \n");
   	

   	printf("Concluimos la ejecución de la aplicacion Servidor \n");
/*
 *	Cierre de las conexiones
 */
    //Procedemos a transformar la imagen yn para enviarsela al cliente
    
    
    printf ("Enviando IMAGEN al cliente ...\n");
	
    if( write(cliente_sockfd, &info, sizeof(bmpInfoHeader)) < 0 )
	{
		perror("Ocurrio un problema en el envio de un mensaje al cliente");
		exit(1);
	}

	if( write(cliente_sockfd, imagenYn, sizeof(unsigned char)*info.width*info.height) < 0 )
	{
		perror("Ocurrio un problema en el envio de un mensaje al cliente");
		exit(1);
	}
    
      close (sockfd);
   	  close (cliente_sockfd);
      free( imagenYn );
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

void calcularDetectorBordes(unsigned char *imagenGray,unsigned char *imagenYn, uint32_t width, uint32_t height,int iniBloque, int finBloque){
  register int x,y,xb,yb;
  register int xc, yc;

  //Falta arreglar esta madre
  int hn1[9] = {1,0,-1,2,0,-2,1,0,-1} , hn2[9] = {-1,-2,-1,0,0,0,1,2,1};
  
  int i,j,k;
  int suma1 = 0,factor = 4, suma2 = 0, suma;
  int dimension = 3;
  int offset = dimension / 2;
  int p = 0;

  
    
  for(y = iniBloque; y < finBloque; y ++){
 
    for(x = 0; x < width; x ++){
         
         j = (y * width + x);

         for(yb = 0; yb < dimension; yb++){
             for(xb = 0; xb < dimension; xb++){
                
                k = (yb * dimension + xb);         
                xc = x + xb - offset;
                yc = y + yb - offset;

                if(xc>=0 && yc>=0 && xc<width && yc<height){
            	  	i = (yc)*width+(xc); 
    	          	suma1 += imagenGray[i]*hn1[k];
                    suma2 += imagenGray[i]*hn2[k];
    		        }
              }
                
           }

          suma1 /= factor;
          suma2 /= factor;

          p = pow(suma1,2) + pow(suma2,2);
          p = sqrt(p);
          suma = p;


	      suma=(suma<0)? 0:suma;

	      imagenYn[j]=(suma>255)? 255:suma;;

       }     
                
         
    }

}

void *funHilo(void * arg){
   int nh = *(int *)arg;

   int iniBloque;
   int finBloque;

    iniBloque = nh * bloque;
    finBloque = (nh + 1) * bloque ;
  
    calcularDetectorBordes(imagenGray,imagenYn, info.width, info.height, iniBloque,finBloque); 
    
    printf("Hilo %d ejecutado \n", nh);

    pthread_exit( arg );

}

