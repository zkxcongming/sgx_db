#include <sys/stat.h>
#include <iostream> 
#include <fcntl.h>
#include <cstdio> 
#include <errno.h>
#include <cstring>
#include <netdb.h>
#include "stdio.h" 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h>
using namespace std; 
#define SERVER_PORT 7000
 
/*
���ӵ��������󣬻᲻ͣѭ�����ȴ����룬
����quit�󣬶Ͽ��������������
*/
 
int main() 
{ 
	//�ͻ���ֻ��Ҫһ���׽����ļ������������ںͷ�����ͨ�� 
	int clientSocket;
 
	//������������socket 
	struct sockaddr_in serverAddr;
 
	char sendbuf[200];
	char recvbuf[200]; 
	int iDataNum;
 
	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
 		return 1;
 	}
 
	serverAddr.sin_family = AF_INET;
 	serverAddr.sin_port = htons(SERVER_PORT);
	 
	//ָ���������˵�ip�����ز��ԣ�127.0.0.1
	//inet_addr()�����������ʮ����IPת���������ֽ���IP
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
 
	if(connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
 		perror("connect");
 		return 1;
 	}
 
	printf("waiting...\n");
 
	while(1){ 
		printf("SEND:");
		cin.getline(sendbuf,200);	
		printf("\n");	 
		send(clientSocket, sendbuf, strlen(sendbuf), 0);
		 
		if(strcmp(sendbuf,"quit")==0)
			break;
		 
		printf("RECV:");	 
		recvbuf[0] = '\0';	 
		iDataNum = recv(clientSocket, recvbuf, 200, 0);	 
		recvbuf[iDataNum] = '\0';	 
		printf("%s\n", recvbuf);	 
	}
 
	close(clientSocket);
 	return 0; 
}

