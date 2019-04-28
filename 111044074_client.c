/**
 * $./clientApp <clientName> <priority> <int-hw> <serverAddress> <port>
 * CSE344 System Programming Final Project - 04.06.2018
 * Sinan Elveren - Gebze Technical University - Computer Engineering
 */
//#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>

//#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/ip.h> 	/* superset of previous */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAX_NAME_LENGTH 64



typedef struct Client {
	int hw;
	int port;
	char name[MAX_NAME_LENGTH];
	char priority;
	char address[MAX_NAME_LENGTH];
} Client;



typedef struct CalcInfo {
	double time;
	double totalTime;
	double calc;
	int cost;
	char providerName[MAX_NAME_LENGTH];
} CalcInfo;








//#define NDBUG



pid_t   parentPID = 0;
int 	sockFD = 0 ;


void finish(int exitNum);
pid_t myWait(int *status);
void signalCatcher(int signum);
void myAtexit(void);


int client_fd = 0;




/* * * * * * * * * * * * * * * * * * * * * *_START_OF_MAIN_ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int main(int argc, char *argv[]) {

	if (atexit(myAtexit) != 0) {
		perror("atexit");
		return 1;
	}

	parentPID = getpid();

	struct 		sockaddr_in socketClient;
	Client  	client = {0, 0, "", '\0', ""};
	CalcInfo  	calcInfo = {0.0, 0.0, 0.0, 0, ""};


	/**Signal**/
	struct sigaction newact;
	newact.sa_handler = signalCatcher;
	/* set the new handler */
	newact.sa_flags = 0;

	/*install sigint*/
	if ((sigemptyset(&newact.sa_mask) == -1) || (sigaction(SIGINT, &newact, NULL) == -1)){
		perror("Failed to install SIGINT signal handler");
		return EXIT_FAILURE;
	}


	/*usage check*/
	if(argc != 6 || strlen(argv[2]) != 1 ||  atoi(argv[5])<=0 ||
			(argv[2][0] != 'C' && argv[2][0] != 'Q' && argv[2][0] != 'T') ) {
		perror("Usage error");
		fprintf(stdout, "try to ./clie  <portNumber>  <dataFile>  <logFile>\n");
		exit(EXIT_FAILURE);
	}



	strcpy(client.name, argv[1]);
	client.priority = argv[2][0];
	client.hw = atoi(argv[3]);
	strcpy(client.address, argv[4]);
	client.port = atoi(argv[5]);


	//struct hostent *hostC;


	if((client_fd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
		perror("Error socket client_fd");
		exit(EXIT_FAILURE);
	}
	//hostC = gethostbyname(argv[1]);


	bzero((char *) &socketClient, sizeof(socketClient)); //clear memory
	socketClient.sin_family = AF_INET;
	//bcopy((char *)hostC->h_addr,(char *)&socketClient.sin_addr.s_addr,hostC->h_length);
	socketClient.sin_port = htons((short)atoi(argv[5]));


	if(connect(client_fd, (struct sockaddr *)&socketClient, sizeof(socketClient))<0) {
		perror("Error : Connect socketClient");
		close(client_fd);
		exit(EXIT_FAILURE);
	}


	fprintf(stdout, "Client %s is requesting %c %d from server %s:%d \n",
					client.name, client.priority, client.hw, client.address/*socketClient.sin_addr*/, client.port);
	fflush(stdout);

	write(client_fd,&client,sizeof(Client));	/*Send client infrmation to server*/

	read(client_fd, &calcInfo, sizeof(CalcInfo));


	fprintf(stdout, "%s’s task completed by %s in %.2f seconds, cos(%d)=%.3f, cost is %dTL, total time spent %.2f seconds.\n",
			client.name,
			calcInfo.providerName,
			calcInfo.time,
			client.hw,
			calcInfo.calc,
			calcInfo.cost,
			calcInfo.totalTime);
	fflush(stdout);


	close(client_fd);

	return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * *_END_OF_MAIN_ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */



pid_t myWait(int *status) {
	pid_t rtrn;

	while (((rtrn = wait(status)) == -1) && (errno == EINTR));

	return rtrn;
}




void myAtexit(void){

	while (myWait(NULL ) > 0);


	/*if there are any fault*/
	if( errno != 0 ) {


		fprintf(stdout, "\n    [%d]PARENT: I'm going to exit\n", getpid());
		fflush(stdout);


		/*free and destroy is here*/
/*		pthread_mutex_destroy(&mutexF);

		if ( pthread_cond_destroy(&cv) == EBUSY) { *//* still waiting condition variable *//*
			pthread_cond_signal(&cv);
		}
	*/

		fprintf(stdout, "-> has been free/destroyed succesfuly\n");
		fflush(stdout);


		fprintf(stdout, "\n    [%d]Parent Process has been exit succesfuly\n", getpid());
		fflush(stdout);


	}
	return;
}



void signalCatcher(int signum) {

	/*get ready to exit*/
	fflush(stdout);

	/*children are leaving here*/
/*
	if(getpid() != parentPID)
		return;
*/




	switch (signum) {
		case SIGUSR1: puts("\ncaught SIGUSR1");
			break;
		case SIGUSR2: puts("\ncaught SIGUSR2");
			break;
		case SIGINT:
			fprintf(stderr,"\n\n[%ld]SIGINT:Ctrl+C signal detected, exit code : (%d) \n", (long)getpid(), signum);
			finish(-1);
			break;
		default:
			fprintf(stderr,"Catcher caught unexpected signal (%d)\n", signum);

			finish(1);
			break;
	}
}


/* or exit funcs */
void finish(int exitNum) {
	/*  myAtexit(); */
	exit(exitNum);
}



/* * * * * * * * * * * * * * * * * * * * * * * _END_OF_HANDLERs_ * * * * * * * * * * * *  * * * * * * * * * * * * * * */


