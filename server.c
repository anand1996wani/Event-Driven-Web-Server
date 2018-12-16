#include "header.h"

struct sockaddr_in serverAddress,clientAddress;
struct clientState client[FD_SETSIZE];
fd_set rset,allset,wset,allset1;
int listenFd=-1;		
int clientIndex = -1;
int ready = -1;
int maxFd = -1;
int pipehelper[2];
int pipehelper1[2];	
int len = sizeof(clientAddress);


int main(int argc, char *argv[]){
	struct timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	initializeServer();
	
	for( ; ; ){
		rset = allset;
		wset = allset1;
		//select with 0 wait time
		//ready = select(maxFd + 1,&rset,&wset,NULL,&time);
		ready = select(maxFd + 1,&rset,&wset,NULL,0);
		perror("select");
		
		//Checking For new Connection
		checkForNewConnection();
		
		// Reading from helper process
		checkForPipeFromHelperProcess();

		// Accepting Client GET Request
		checkForGetRequest();

		// HTTP Responce Part
		checkForHttpRespnce();

		// CGI RESPONCE Part
		checkCGIResponcePart();
	}
	return 0;
}
