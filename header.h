#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

#define portNo 8000

#define FOUND "HTTP/1.0 200 OK\n\n"
#define NOTFOUND "HTTP/1.0 404 Not Found\n\n"

struct clientState{
	int clientFd;
	int fileFd;
	int state;	//0 connection accept, 1 get request accepted, 2 ready to send, 3 file sending started
	char fileName[100];
	char buffer[1000];
	int reqSize;
	int responce;
	int size;
	int pipeReadFd;
};

void helperProcess(int pipefd[],int pipefd1[]);

void handleCGI(int i,int pipefd[2]);

void exitClient(int i);

int checkFile(int i);

int getRequestParser(int i,char *buffer,int n);

void initializeServer();

void  checkForNewConnection();

void checkForPipeFromHelperProcess();

void checkForGetRequest();

void checkForHttpRespnce();

void checkCGIResponcePart();
