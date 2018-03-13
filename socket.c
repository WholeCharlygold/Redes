//2E9BFB09C24D8859
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <sys/time.h>


struct ifreq setNombre();
void getIndex(struct ifreq interfaz, int ds);
void getMac(struct ifreq interfaz, int ds );
void getIp(struct ifreq interfaz, int ds);
void getMask(struct ifreq interfaz, int ds);
void enviarTrama(int ds, unsigned char *trama);
void crearTrama(int ds);
void imprimeTrama(unsigned char *trama, int tam_trama);
int recibirTrama(int ds, unsigned char *bufferTrama, int sub);
void imprimeIpMac(unsigned char *bufferTrama, int sub);

char nombre[15];
unsigned char mac[6], ip[4], mask[4];
int indice;
FILE *diccionario = NULL;

int main(){
	int packet_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	diccionario = fopen("diccionario.txt", "w");

	if( diccionario == NULL ){
		puts("HOlA");
		exit(1);
	}

  	if(packet_socket == -1)
    	perror("\nError al abrir el socket");
  	else{
    	struct ifreq interfaz = setNombre();
    	getIndex(interfaz, packet_socket);
    	getMac(interfaz, packet_socket);
    	getIp(interfaz, packet_socket);
    	getMask(interfaz, packet_socket);
  	}
  	crearTrama(packet_socket);

  	close(packet_socket);
  	fclose(diccionario);
  	return 0;
}

struct ifreq setNombre(){
	struct ifreq interfaz;
	puts("Introduce el nombre de la interfaz:");
  	gets(nombre);
  	strcpy( interfaz.ifr_name, nombre);
  	return interfaz;
}

void getIndex(struct ifreq interfaz, int ds){
  	if( ioctl( ds, SIOCGIFINDEX, &interfaz ) == -1 ){
    	perror("\nError al obtener el indice");
    	exit(0);
  	}
  	else
    	indice = interfaz.ifr_ifindex;
  	
  	printf("%i\n", indice );
}

void getMac(struct ifreq interfaz, int ds ){
  	if( ioctl(ds, SIOCGIFHWADDR, &interfaz) == -1 ){
    	perror("\nError al obtener la MAC");
  	}
  	else{
    	for(int i = 0; i < 6; i++)
      		mac[i] = interfaz.ifr_hwaddr.sa_data[i];
  	}

  	printf("La MAC:\n\n");
		for(int i = 0; i < 6; i++)
			(i==5)?printf("%02x", mac[i]):printf("%2x:", mac[i]);
	printf("\n\n");
}

void getIp(struct ifreq interfaz, int ds){
  	if(ioctl(ds, SIOCGIFADDR, &interfaz) ){
	    perror("\nError al obtener al address");
  	}
  	else{
    	for(int i = 0; i < 4; i++)
      		ip[i] = interfaz.ifr_addr.sa_data[i+2];
  	}
  	printf("La IP: \n\n");
		for(int i = 0; i < 4; i++)
			i == 3 ? printf("%i",ip[i]):printf("%i.",ip[i]);
    printf("\n\n");
}

void getMask(struct ifreq interfaz, int ds){
  	if(ioctl(ds, SIOCGIFNETMASK, &interfaz) ){
    	perror("\nError al obtener al address");
  	}
  	else{
    	for(int i = 0; i < 4; i++)
      	mask[i] = interfaz.ifr_netmask.sa_data[i+2];
  	}
  	printf("La mascara subred:\n\n");
		for(int i = 0; i < 4; i++)
			i == 3 ? printf("%i",mask[i]):printf("%i.",mask[i]);
  	printf("\n\n");
}

void enviarTrama(int ds, unsigned char *trama){
  	struct sockaddr_ll nic;
  	memset(&nic, 0x00, sizeof(nic));
  	nic.sll_family = AF_PACKET;
  	nic.sll_protocol = htons(ETH_P_ALL); 
  	nic.sll_ifindex = indice; 
  	int tam = sendto(ds, trama, 60, 0, (struct sockaddr *)&nic, sizeof(nic) );
  	if(tam == -1)
    	printf("Hubo un error");
  	//else
    	//printf("Se envio el mensaje\n\n\n\n\n");
}

void crearTrama(int ds){
	unsigned char trama[150];

	for(int i = 0; i < 6; i++)
    	trama[i] = 0xFF;

  	for(int i = 6; i < 12; i++)
    	trama[i] = mac[i-6];

	trama[12] = 0x08;
  	trama[13] = 0x06;

  	for(int i = 1; i < 255; i++){
	  	unsigned char mensaje[] = 	{0x00, 0x01, 0x08, 0x00, 6, 4, 0x00, 0x01, mac[0], mac[1], mac[2], mac[3],
	  								mac[4], mac[5], ip[0], ip[1], ip[2], ip[3], 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	  								ip[0], ip[1], ip[2], i };
		for(int i = 0; i < 28; i++){
			trama[i + 14] = mensaje[i];
		}
		unsigned char trama_rec[1514];
		struct timeval ti, tf;
    		double tiempo;

		//printf("\n\nIP: %i.%i.%i.%i\n\n",ip[0],ip[1],ip[2], i);

	  	enviarTrama(ds, trama);
	  	
	    	gettimeofday(&ti, NULL);   // Instante inicial

	  	do{
	  		if( recibirTrama(ds, trama_rec, i) == 0 )
	  			break;

	  		gettimeofday(&tf, NULL);   // Instante final
    		tiempo= (tf.tv_sec - ti.tv_sec)*1000 + (tf.tv_usec - ti.tv_usec)/1000.0;
	  	}while( tiempo < 500.0 );
  	}
  }

void imprimeTrama(unsigned char *trama, int tam_trama){
	printf("\n\n\nTrama: \n\n");
  	for(int i = 0; i < tam_trama; i++ ){
    	if(i % 16 == 0)
      		printf("\n");
    	printf("%.02x ", trama[i]);
  	}
}

int recibirTrama(int ds, unsigned char *bufferTrama, int sub){
  	int tam;
  	unsigned char arp[] = {8, 6};
  	unsigned char respuesta[] = {0, 2};
  	tam = recvfrom(ds, bufferTrama, 1514, 0, NULL, 0);
  	if(tam == -1){
		perror("Hubo un error al recibir trama\n\n\n");
		return 1;
  	}
  	else{
    	if( memcmp(bufferTrama + 0, mac, 6) == 0 && 
    		memcmp(bufferTrama + 12, arp, 2) == 0 && 
    		memcmp(bufferTrama + 20, respuesta, 2) == 0
    		){
    		//imprimeTrama(bufferTrama, tam);
      		imprimeIpMac(bufferTrama, sub);

      		return 0;
      	}
  	}
  	return 1;
}

void imprimeIpMac(unsigned char *trama, int sub){
	printf("IP: %i.%i.%i.%i \n",ip[0], ip[1], ip[2], sub );
	fprintf(diccionario,"IP: %i.%i.%i.%i \n",ip[0], ip[1], ip[2], sub );
	printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n\n", trama[6],trama[7],trama[8],trama[9],trama[10],trama[11]);
	fprintf(diccionario,"MAC: %02x:%02x:%02x:%02x:%02x:%02x\n\n", trama[6],trama[7],trama[8],trama[9],trama[10],trama[11]);
}
