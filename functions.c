#include "header.h"

extern struct sockaddr_in serverAddress,clientAddress;
extern struct clientState client[FD_SETSIZE];
extern fd_set rset,allset,wset,allset1;
extern int listenFd;
extern int clientIndex;
extern int ready;
extern int maxFd;
extern int pipehelper[2];
extern int pipehelper1[2];	
extern int len;

void checkForNewConnection(){
	if(FD_ISSET(listenFd,&rset)){
			int clientFd;
			clientFd = accept(listenFd,(struct sockaddr *)&clientAddress,&len);
			perror("accept");
			
			//making clientFd Non Blocking
			int fd_flags = fcntl(clientFd,F_GETFL);
			if(-1==fcntl(clientFd,F_SETFL,fd_flags | O_NONBLOCK)){
				printf("Error While FD Blocking\n");
			}
			perror("fcntl");
			
			int i;
			for(i = 0;i < FD_SETSIZE;i++){
					if(client[i].clientFd < 0){
						client[i].clientFd = clientFd;
						client[i].state = 0;				//Connection accepted
						break;
					}
			}
			
			
			FD_SET(clientFd,&allset);
			if(clientFd > maxFd){
				maxFd = clientFd;
			}
			
			if(i > clientIndex){
				clientIndex = i;
				printf("Client Connected with index %d\n",i);
			}
		}
}

void helperProcess(int pipefd[],int pipefd1[]){
	close(pipefd[1]);
	close(pipefd1[0]);
	while(1){
		int clientId = -1;
		read(pipefd[0],&clientId,sizeof(clientId));
		perror("helper process pipe read");
		printf("Data Read is %d\n", clientId);
		char buffer[100];
		read(pipefd[0],buffer,sizeof(buffer));
		printf("Data Read From Client Is %s\n",buffer);
		char fileBuffer[10000];
		printf("file is %s\n",buffer);
		int fileFd = open(buffer,O_RDONLY);
		perror("helper open");
		read(fileFd,fileBuffer,sizeof(fileBuffer));
		perror("helper read file");
		write(pipefd1[1],&clientId,sizeof(clientId));
		perror("helper process pipe write");
		printf("Data Written To Pipe\n");
	}
}


void handleCGI(int i,int pipefd[2]){
	close(pipefd[0]);
	dup2(pipefd[1],1);
	char *const parmList[] = {NULL};
	execv(client[i].fileName,parmList);
}

void exitClient(int i){
	if(client[i].fileFd!=-1){
		close(client[i].fileFd);
	}
	if(client[i].pipeReadFd!=-1){
		FD_CLR(client[i].pipeReadFd,&allset);
	}
	close(client[i].clientFd);
	FD_CLR(client[i].clientFd,&allset);
	if(client[i].state == 3){
		FD_CLR(client[i].clientFd,&allset1);
	}
	client[i].state = -1;
	client[i].clientFd = -1;
	client[i].fileFd = -1;
	client[i].reqSize = 0;
	client[i].responce = 0;
	client[i].size = 0;
	client[i].pipeReadFd = -1;
}

int checkFile(int i){
	int ret = -1;
	if(client[i].state==2){
		struct stat statbuf;
		int fileFd = open(client[i].fileName,O_RDONLY);
		perror("File open");
		if(fileFd==-1){
			ret = 0;
		}else{
			client[i].fileFd = fileFd;
			if(fstat(fileFd,&statbuf) < 0){
				printf("Fstat Error\n");
			}else{
				client[i].size = statbuf.st_size;
				printf("File Size is%d bytes\n",client[i].size);	
			}
			
			//Checking If The File Is In Memory
			void *address;
			unsigned char *vector;
			const size_t pageSize = sysconf(_SC_PAGESIZE);
						
			address = mmap(NULL,statbuf.st_size,PROT_READ,MAP_SHARED,fileFd,0);		
			int noOfPages = (statbuf.st_size + pageSize - 1) / pageSize;			
			vector = calloc(1,noOfPages);
			mincore(address,statbuf.st_size,vector);
			printf("Page Size Of System Is %lu Bytes\n",pageSize);
			printf("Total Size Of File  Is %lu Bytes\n",statbuf.st_size);	
			printf("Total Pages Of File Are %d \n",noOfPages);	
			printf("Cashed blocks of  : are\n");
			int count = 0;
			for(size_t i = 0;i <= statbuf.st_size / pageSize; ++i){
				if(vector[i] & 1){
					printf("%lu ",(unsigned long int)i);
					count++;
				}
			}
			printf("\n");
			int flag = -1;
			if(count==noOfPages){
				printf("All Pages Of The File Are In Main Memory\n");
				ret = 1;
				client[i].reqSize = read(fileFd,client[i].buffer,sizeof(client[i].buffer));
			}else{
				printf("All Pages Of The File Is Not In Memory\n");
				ret = 2;
			}
			free(vector);
			munmap(address,statbuf.st_size);
		}
	}else if(client[i].state==3){
		client[i].reqSize = client[i].reqSize + read(client[i].fileFd,client[i].buffer,sizeof(client[i].buffer));
		ret = 1;
	}else if(client[i].state==4){
		client[i].reqSize = read(client[i].fileFd,client[i].buffer,sizeof(client[i].buffer));
		client[i].state = 3;
	}
	else{
		ret = 0;
	}
	return ret;
}

int getRequestParser(int i,char *buffer,int n){
	// 0 File Not Found 1 File Found 2 CGI Request
	int ret = 0;
	if(buffer[n-1]=='\n' && buffer[n-2]=='\r' && buffer[n-3]=='\n' && buffer[n-4]=='\r'){
		printf("Get Request Completed\n");
		char* token = strtok(buffer,"\n");
		char* token1 = strtok(token," ");
		char fileName[100];
		//printf("%s\n",token1);
		token1 = strtok(NULL," ");
		//printf("%s\n",token1);		
		if(strcmp(token1,"/")==0){
			strcpy(client[i].fileName,"index.html");
			ret = 1;
		}else{
			strcpy(client[i].fileName,token1+1);
			printf("file is %s\n",client[i].fileName);
			//Logic FOR FAST CGI FILE NAME PARSING
			char* token2;
			token2 = strtok(token1 + 1,".");
			//printf("%s\n",token2);
			token2 = strtok(NULL,".");
			//printf("%s\n",token2);
			if(token2!=NULL){
				if(strcmp(token2,"cgi")==0){
					printf("CGI Request\n");
					ret = 2;
				}else{
					ret = 1;
				}
			}else{
				ret = 1;
			}
		}
	}else{
		ret = 0;
	}
	return ret;
}

void initializeServer(){
	pipe(pipehelper);
	perror("pipe helper");
	pipe(pipehelper1);
	perror("pipe helper1");
	
	pid_t pid;
	pid = fork();//Helper Process
	if(pid==0){
		helperProcess(pipehelper,pipehelper1);
	}
	close(pipehelper[0]);
	close(pipehelper1[1]);

	listenFd = socket(AF_INET,SOCK_STREAM,0);
	perror("Server Socket");
	
	bzero(&serverAddress,sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(portNo);
	
	bind(listenFd,(struct sockaddr *)&serverAddress,sizeof(serverAddress));
	perror("bind");
	
	listen(listenFd,1000);
	perror("server listen");
	
	for(int i = 0;i < FD_SETSIZE;i++){
		client[i].clientFd = -1;
		client[i].fileFd = -1;
		client[i].state = -1;
		client[i].reqSize = 0;
		client[i].responce = 0;
		client[i].size = 0;
		client[i].pipeReadFd = -1;
	}
	
	FD_ZERO(&allset);
	FD_ZERO(&rset);
	
	FD_ZERO(&allset1);
	FD_ZERO(&wset);
	
	FD_SET(listenFd,&allset);
	if(listenFd > maxFd){
		maxFd = listenFd; 
	}
	
	FD_SET(pipehelper1[0],&allset);
	if(pipehelper1[0] > maxFd){
		maxFd = pipehelper1[0]; 
	}	
}

void checkForPipeFromHelperProcess(){
	if(FD_ISSET(pipehelper1[0],&rset)){
		int clientId = -1;
		read(pipehelper1[0],&clientId,sizeof(clientId));
		perror("Read client id from pipe");
		FD_SET(client[clientId].clientFd,&allset1);
		client[clientId].state = 4;
		checkFile(clientId);
		
	}
}

void checkForGetRequest(){
	for(int i=0;i <= clientIndex;i++){
			int sockfd;
			if((sockfd = client[i].clientFd) < 0){
				continue;
			}else{
				if(FD_ISSET(sockfd,&rset)){
					char buffer[500];
					int n;
					n = recv(sockfd,buffer,sizeof(buffer),MSG_DONTWAIT);
					perror("recv");
					if(n==0){
						/* Received FIN ie Connection Closed By Client*/
						printf("Closing Client Connection Received Fin\n");
						exitClient(i);
					}
					else{
						buffer[n] = '\n';
						printf("The data received from client is of %s %d bytes\n",buffer,n);
						client[i].state = 1;		//Client Get Request Received
						
						int k;
						for(k = client[i].reqSize;k < client[i].reqSize + n;k++){
							client[i].buffer[k] = buffer[k - client[i].reqSize];
						}
						client[i].reqSize = k;
						
						int flag = getRequestParser(i,client[i].buffer,client[i].reqSize);
						if(flag==0){
							printf("Client GET Request Is not Complete\n");
							continue;
						}
						else if(flag==1){
							//Normal File Request
							client[i].state = 2;
							strcpy(client[i].buffer,FOUND);
							send(client[i].clientFd,client[i].buffer,strlen(client[i].buffer),0);
							perror("send Header");
							
							flag = checkFile(i);
							
							if(flag==0){
								strcpy(client[i].buffer,NOTFOUND);
								send(client[i].clientFd,client[i].buffer,strlen(client[i].buffer),0);
								perror("send Responce");
								exitClient(i);
							}else if(flag==1){
								FD_SET(client[i].clientFd,&allset1);
							}else if(flag==2){
								write(pipehelper[1],&i,sizeof(i));
								write(pipehelper[1],client[i].fileName,sizeof(client[i].fileName));
								perror("pipe write");
								//write(pipefd[1],&client[i].fileName,sizeof(client[i].fileName));
							}
						}else if(flag==2){
							//CGI Request
							client[i].state = 2;
							strcpy(client[i].buffer,FOUND);
							send(client[i].clientFd,client[i].buffer,strlen(client[i].buffer),0);
							perror("send Header");
							int pipefd[2];
							flag = checkFile(i);
							if(flag==0){
								strcpy(client[i].buffer,NOTFOUND);
								send(client[i].clientFd,client[i].buffer,strlen(client[i].buffer),0);
								perror("send Responce");
								exitClient(i);
							}else{
								pipe(pipefd);
								perror("pipe");
								pid_t pid = fork();//FAST CGI PROCESS
								perror("fork");
								if(pid == 0){
									//CGI PROCESS
									handleCGI(i,pipefd);
								}
								close(pipefd[1]);
								client[i].pipeReadFd = pipefd[0];
								FD_SET(pipefd[0],&allset);
								if(pipefd[0] > maxFd){
									maxFd = pipefd[0];
								}
							}
						}
					}
					if(--ready < 0){
						break;
					}
				}
			}	
		}
}

void checkForHttpRespnce(){
	for(int i=0;i <= clientIndex;i++){
			int sockfd;
			if((sockfd = client[i].clientFd) < 0){
				continue;
			}else{
				if(FD_ISSET(sockfd,&wset)){
					if(client[i].state == 2 || client[i].state == 3){
						if(client[i].responce < client[i].reqSize){
							client[i].responce = client[i].responce + send(client[i].clientFd,client[i].buffer + client[i].responce%(sizeof(client[i].buffer)),client[i].reqSize - client[i].responce,MSG_DONTWAIT);
							perror("send Responce");
							client[i].state = 3;
						}else if(client[i].reqSize < client[i].size){
							checkFile(i);
						}else{
							printf("Exit client client responce%d\n",client[i].responce);
							exitClient(i);
						}
					}
					if(--ready < 0){
						break;
					}
				}
			}
		}
}

void checkCGIResponcePart(){
	for(int i=0;i <= clientIndex;i++){
			int pipefd;
			char buffer[1000];
			if((pipefd = client[i].pipeReadFd) < 0){
				continue;
			}else{
				if(FD_ISSET(pipefd,&rset)){
					int n = read(pipefd,buffer,sizeof(buffer));
					perror("read pipe");
					send(client[i].clientFd,buffer,n,0);
					perror("send responce");
					exitClient(i);
					if(--ready < 0){
						break;
					}
				}
			}
		}
}
