/*
 * main.c
 *
 *  Created on: 27 févr. 2012
 *      Author: mica
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define SERVEURNAME "127.0.0.1"
#define SERVEURSock 443

int to_server_socket = -1;

struct netMsg {
	int msgSize;
	char * data;
};

int sendDFrame(unsigned long long int stimestamp, int ssensorId, int sdata);
int sendNetMsg(int len, char * msg);
int transmit(int socket, char * buff, int size);
int sendSFrame(unsigned long long int stimestamp, char infoType, int ssensorId,
		char typeCapteur);
void purger(void);

int main(void) {

	char *server_name = SERVEURNAME;
	struct sockaddr_in serverSockAddr;
	struct hostent *serverHostEnt;
	long hostAddr;

	bzero(&serverSockAddr, sizeof(serverSockAddr));
	hostAddr = inet_addr(server_name);
	if ((long) hostAddr != (long) -1)
		bcopy(&hostAddr, &serverSockAddr.sin_addr, sizeof(hostAddr));
	else {
		serverHostEnt = gethostbyname(server_name);
		if (serverHostEnt == NULL) {
			printf("Prblm gethost\n");
			exit(0);
		}
		bcopy(serverHostEnt->h_addr, &serverSockAddr.sin_addr,
				serverHostEnt->h_length);
	}
	serverSockAddr.sin_port = htons(SERVEURSock);
	serverSockAddr.sin_family = AF_INET;

	printf("creation socket client\n");

	/* creation de la socket */
	if ((to_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Prblm creation socket client\n");
		exit(0);
	}

	printf("demande de connection\n");
	/* requete de connexion */
	if (connect(to_server_socket, (struct sockaddr *) &serverSockAddr,
			sizeof(serverSockAddr)) < 0) {
		perror("connect");
		printf("Prblm demande de connection\n");
		exit(0);
	}


	/*Capteur initiaux*/
	sendSFrame(42, 'A', 2, 'T');
	sendDFrame(42, 2, 28);
	sendSFrame(42, 'A', 3, 'T');
	sendDFrame(42, 3, 22);
	sendSFrame(43, 'A', 1, 'E');
	sendDFrame(43, 1, 0);


	while (1) {
		int id = 0;
		int value = -1;
		char c = 'Z';
		char typecapteur = 'Z';

		printf("Which type of frame to send (a or o)? \n");
		scanf("%c", &c);

		printf("Choose %c\n",c);
		switch (c) {
		case 'a':
			printf("id? \n");
			scanf("%d", &id);
			printf("id %d \n",id);
			purger();
			if (id == 0) {
				break;
			}
			printf("type (T,H,P,C,E,I)?\n ");
			scanf("%c", &typecapteur);
			purger();
			if (typecapteur == '0') {
				break;
			}
			sendSFrame(42, 'A', id, typecapteur);
			break;
		case 'o':
			printf("id? \n");
			scanf("%d", &id);
			purger();
			if (id == 0) {
				break;
			}
			printf("value? \n");
			scanf("%d", &value);
			purger();
			if (value == -1) {
				break;
			}
			sendDFrame(42, id, value);
			break;
		default:
			break;
		}

	}

	/* fermeture de la connection */
	shutdown(to_server_socket, 2);
	close(to_server_socket);
	return 0;
}

int sendDFrame(unsigned long long int stimestamp, int ssensorId, int sdata) {
	int i;
	char buff[20];
	uint64_t timestamp;
	int sensorId;
	int data;
	printf("Sending O Frame\n");
	timestamp = htobe64(stimestamp);
	sensorId = htobe32(ssensorId);
	data = htobe32(sdata);
	memcpy(buff, &timestamp, sizeof(long long));
	buff[sizeof(long long)] = 'D';
	memcpy(&buff[9], &sensorId, sizeof(int));
	memcpy(buff + sizeof(long long) + 1 + sizeof(int), &data, sizeof(int));
	buff[17] = '\n';
	sendNetMsg(17, buff);
	for (i = 0; i < 17; i++) {
		printf("%hhx \n", buff[i]);
	}
	return 0;
}

int sendSFrame(unsigned long long int stimestamp, char infoType, int ssensorId,
		char typeCapteur) {
	int i;
	char buff[20];
	uint64_t timestamp;
	int sensorId;
	printf("Sending S Frame\n");
	timestamp = htobe64(stimestamp);
	sensorId = htobe32(ssensorId);
	memcpy(buff, &timestamp, sizeof(long long));
	buff[sizeof(long long)] = 'S';
	buff[sizeof(long long) + 1] = infoType;
	memcpy(&buff[10], &sensorId, sizeof(int));
	buff[sizeof(long long) + 2 + sizeof(int)] = typeCapteur;
	buff[15] = '\n';
	sendNetMsg(15, buff);
	for (i = 0; i < 15; i++) {
		printf("%hhx \n", buff[i]);
	}
	return 0;
}

int sendNetMsg(int len, char * msg) {
	struct netMsg newMsg = { .msgSize = len, };

	//Allocate enough memory to copy the msg,
	//so that the original buffer can be freed after this function call.
	//this memory will be freed once the message is read.
	newMsg.data = malloc(len);
	memcpy(newMsg.data, msg, len);
	if (transmit(to_server_socket, newMsg.data, newMsg.msgSize)) {
		printf("mq_send dispatch request failed\n");
		return -1;
	}
	return 0;
}

int transmit(int socket, char * buff, int size) {
	int i;
	for (i = 0; i < size; i++) {
		printf("%hhx \n", buff[i]);
	}
	if (write(to_server_socket, buff, size) == -1) {
		printf("send on socket failed\n");
		return -1;
	}
	return 0;
}

void purger(void)
{
   char c;
   while((c=getchar()) != '\n' && c != EOF)
   {
   }
}
