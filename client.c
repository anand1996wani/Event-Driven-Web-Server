#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

//Address Structure 
//specifies where to connect to

//Simple TCP Client Program

#define portno 8000


int main(int argc,char *argv[]){
	int clientSocket;
	clientSocket = socket(AF_INET,SOCK_STREAM,0);
	
	//Step 2 Structure for Server address
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portno);//htons converts int to appropriate data format
	serverAddress.sin_addr.s_addr = INADDR_ANY; //same as 0.0.0.0
	int connectionStatus = connect(clientSocket,(struct sockaddr * )&serverAddress,sizeof(serverAddress));
	
	if(connectionStatus == -1){
		printf("There Was An Error While Connecting To The Remote Server\n");
		perror("");
	}else{
		char serverResponce[100];
		//recv(clientSocket,&serverResponce,sizeof(serverResponce),0);//0 is for flags
		//printf("%s\n",serverResponce);
		
		char buffer[500];
		char command[500];
		strcpy(command,"GET /");
		int n;
		n = scanf("%s",buffer);
		strcat(command,buffer);
		printf("%s\n",command);
		strcat(command," HTTP/1.1\r\n\r\n");
		printf("%s\n",command);
		n = send(clientSocket,command,strlen(command),0);
		int count = 1;
		char recvbuffer[1];
		while(count==1){
			count = recv(clientSocket,recvbuffer,sizeof(recvbuffer),0);	//0 is for flags
			//printf("The Data Received From The Server Is %d Bytes\n",count);
			write(1,recvbuffer,count);
		}
		printf("\n");
	}
	close(clientSocket);
	return 0;
}
