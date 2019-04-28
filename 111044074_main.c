/**
 * ./$homeworkServer 5555 data.dat log.log
 * CSE344 System Programming Final Project - 04.06.2018
 * Sinan Elveren - Gebze Technical University - Computer Engineering
 */


//#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <math.h>
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
#include <netinet/in.h>
#include <netinet/ip.h> 	/* superset of previous */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

//#define NDBUG




#define MAX_PROVIDER_COUNT 500
#define MAX_JOB_COUNT 1000
#define MAX_NAME_LENGTH 64
#define MAX_LINE_LENGTH 128
//#define SOCKET_NAME "hwSocket"
#define HOST_NAME_MAX 64
#define PI 3.14159265



pthread_cond_t condProvider = PTHREAD_COND_INITIALIZER;
pthread_cond_t condCalc = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexMain = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCalc = PTHREAD_MUTEX_INITIALIZER;

int providerFlag[MAX_PROVIDER_COUNT];
int flag = -1;

int hileciCount = 0;
int     fd;

void initialize();




typedef struct Provider {
	int id;
	char name[MAX_NAME_LENGTH];
	int performance;
	int price;
	int duration;
} Provider;



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
	int hw;
	char providerName[MAX_NAME_LENGTH];
} CalcInfo;



typedef struct Job {
	int hw;
	int bestProviders[MAX_PROVIDER_COUNT];
	int providersCount;
} Job;






pid_t   parentPID = 0;
int 	socketID[MAX_PROVIDER_COUNT];
int 	server_fd = 0 ;
int		sock = 0;
int 	portNum = 0;
int 	tflag = 1;
int 	qflag = 1;
int 	cflag = 1;
CalcInfo  	calcInfo = {0.0, 0.0, 0.0, 0, 0, ""};
int 	bestProvidersQ[MAX_PROVIDER_COUNT];
int 	bestProvidersC[MAX_PROVIDER_COUNT];
int 	bestProvidersT[MAX_PROVIDER_COUNT];
int		bestProviders[MAX_PROVIDER_COUNT];


void finish(int exitNum);
pid_t myWait(int *status);
void signalCatcher(int signum);
void myAtexit(void);

int randomRange();
int readData(const char *fileName, Provider providers[], int *providersCount );

void printIntro(const char *fileName, Provider providers[], int providersCount);
void* providerWork(void *provider);
void findProviderForCalc(Provider providers[],int providersCount, char priority,  int bestProviders[]);
int setServer(int portNum,  Provider providers[], int providersCount);
void createJobThread(int hw, int bestProviders[], int providersCount);
void* providerJob(void * hw) ;


/* * * * * * * * * * * * * * * * * * * * * *_START_OF_MAIN_ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int main(int argc, char *argv[]) {

	if (atexit(myAtexit) != 0) {
		perror("atexit");
		return 1;
	}

	parentPID = getpid();

	Provider  	providers[MAX_PROVIDER_COUNT] = {0, "", 0, 0, 0};

	int			providersCount = 0;
	int			error = 0;




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
	if(argc != 4 || atoi(argv[1]) <= 0) {
		perror("Usage error");
		fprintf(stdout, "try to ./homeworkServer  <portNumber>  <dataFile>  <logFile>\n");
		exit(EXIT_FAILURE);
	}

	//fd = open(argv[4], OPEN_FILE_FLAGS_B, mode);


	/*initialize mutex and condition variable*/
	initialize();

	/*read data*/
	readData(argv[2], providers, &providersCount);
	pthread_t 	threadsProvider[providersCount];
	int 		thrProvID[providersCount];

	//portNum = atoi(argv[1]);


	/*create threads for providers*/
	for (int i = 0; i < providersCount; ++i) {
		thrProvID[i] = i;
		if(error = pthread_create(&threadsProvider[i], NULL, providerWork, (void *)&providers[i]) ){
			fprintf(stdout,"Failed to create thread: %s\n", strerror(error));
			exit(EXIT_FAILURE);
		}

	}


	usleep(500);		/*wait for create threads*/

	printIntro(argv[3], providers, providersCount);


	setServer(atoi(argv[1]), providers, providersCount);



	/*wait to poviders's therad*/
	for (int i = 0; i < providersCount; ++i) {
		if( error = pthread_join(threadsProvider[i], NULL))
			fprintf(stdout,"Failed to join thread: %s\n", strerror(error));

	}



	printf("\nI 'm not here\n");

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
		pthread_mutex_destroy(&mutexMain);
		pthread_mutex_destroy(&mutexCalc);

		if ( pthread_cond_destroy(&condProvider) == EBUSY) { //* still waiting condition variable *//*
			pthread_cond_signal(&condProvider);
		}
		if ( pthread_cond_destroy(&condCalc) == EBUSY) { //* still waiting condition variable *//*
					pthread_cond_signal(&condCalc);
		}


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


void initialize() {
	/* Initialize the mutex and condition variable*/
	pthread_mutex_init(&mutexMain, NULL);                 /*mutex for provider*/
	pthread_mutex_init(&mutexCalc, NULL);                 /*mutex for provider*/

	pthread_cond_init(&condProvider, NULL);               /*ccondtional variable for provider*/
	pthread_cond_init(&condCalc, NULL);               /*ccondtional variable for calc*/
}



/*Generate random number between 5 and 15 (closed)*/
int randomRange() {
	unsigned const range = 15 - 5 + 1;

	return  (5 + (int) (((double) range) * rand () / (RAND_MAX + 1.0)));
}



/* read *dat file 's content */
int readData(const char *fileName, Provider providers[], int *providersCount ) {
	char line[MAX_LINE_LENGTH];
	char temp[MAX_NAME_LENGTH];

	FILE *fp = NULL;

	fp = fopen(fileName, "rb");

	if (fp == NULL) {
		perror("Could not open dat* file");
		exit(EXIT_FAILURE);
	}

	/*read header*/
	fgets(line, MAX_LINE_LENGTH - 1 , fp);


	/*read providers info*/
	for (int i = 0; fgets(line, MAX_LINE_LENGTH - 1 , fp) != NULL  && i < MAX_PROVIDER_COUNT;  ++i) {
		/*7 minimum line length for correct provider info*/
		if (strlen(line) > 6) {
			if (4 == sscanf(line, "%s%*[^0123456789]%d%*[^0123456789]%d%*[^0123456789]%d",
							&(providers[i].name),
							&(providers[i].performance), &(providers[i].price), &(providers[i].duration))) {
				providers[i].id = i;
				(*providersCount)++;
			} else {
				perror("Data file content error");
				exit(EXIT_FAILURE);
			}
		}
		else {
			perror("file dat* : wrong file content");
			exit(EXIT_FAILURE);
		}

	}


	fclose(fp);

	return 0;
}




void printIntro(const char *fileName, Provider providers[], int providersCount){


	fprintf(stdout, "Logs kept at %s\n", fileName);
	fprintf(stdout, "%d provider threads created\n", providersCount);
	fprintf(stdout, "Name \tPerformance \tPrice \tDuration\n");

	for (int i = 0; i < providersCount; ++i) {
		fprintf(stdout, "%s \t%d \t\t%d \t%d\n",
				providers[i].name, providers[i].performance, providers[i].price, providers[i].duration);
	}

	/*initalize*/
	for (int i = 0; i < providersCount; ++i) {
		providerFlag[i] = -1;
	}

	fflush(stdout);
}





void* providerWork(void * provider){
	int rc = 0;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);


	rc = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (rc != 0) {
		perror("Error signal handler");
	}



	while (1) {


		/* Lock the mutex before accessing value. */

		pthread_mutex_lock(&mutexMain);

/*

		while (providerFlag[flag] < 0) {
			pthread_cond_wait(&condProvider, &mutexMain);
		}
*/



		if (providerFlag[flag] == ((Provider *) provider)->id) {
			fprintf(stdout, "Provider %s is processing task number 1: %d \n",
					((Provider *) provider)->name, calcInfo.hw );


			//provider is busy
			fflush(stdout);
			providerFlag[flag] = -1;
			flag = -1;
			//calculate();


			int random = randomRange();
			sleep(random);
			double ret = 0.0;

			/* Lock the mutex before accessing value. */

			//		pthread_mutex_lock(&mutexCalc);

			strcpy( calcInfo.providerName ,  ((Provider *) provider)->name);
			calcInfo.calc = cos(calcInfo.hw * (PI / 180.0));
			calcInfo.cost =   ((Provider *) provider)->price;
			calcInfo.time = random;
			calcInfo.totalTime = random + 1; 		/*actually not iplement*/

			//	pthread_cond_signal(&condCalc);
			//	pthread_mutex_unlock(&mutexCalc);


			/*calc*/

			fprintf(stdout, "Provider %s completed task number 1: cos(%d)=%.3f in %.2f seconds \n",
					((Provider *) provider)->name, calcInfo.hw, calcInfo.calc, calcInfo.totalTime );



		} else {
			fprintf(stdout, "Provider %s waiting for task* \n", ((Provider *) provider)->name);

			pthread_cond_wait(&condProvider, &mutexMain);
			//pthread_cond_broadcast(&condProvider);

		}

		pthread_mutex_unlock(&mutexMain);


	}

	pthread_exit(NULL);
}


/*server - create socket,connect and do job*/
int setServer(int portNum, Provider providers[], int providersCount) {

	struct	 	sockaddr_in server;
	Client  	client = {0, 0, "", '\0', ""};
	int 		provider = 0;
	int 		count = 0;


	if((sock = socket(AF_INET, SOCK_STREAM, 0))< 0) {
		perror("Error socket sock server");
		exit(EXIT_FAILURE);
	}
	bzero((char *) &server, sizeof(server)); //clear the memory for server address

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons((short) portNum);

	bind(sock, (struct sockaddr *) &server, sizeof(server));

	if (listen(sock, 10) == -1) {
		perror("Failed to listen server:");
		close(sock);
		exit(EXIT_FAILURE);
	}
	int i = 0;

	while(1) {
		fprintf(stdout,"Server is waiting for client connections at port %d\n", portNum);
		fflush(stdout);

		server_fd = accept(sock, (struct sockaddr *) NULL, NULL);


		read(server_fd, &client, sizeof(Client));

		fprintf(stdout, "Client %s (%c %d) connected, forwarded to provider %s\n",
				client.name, client.priority, client.hw, providers[provider].name);
		fflush(stdout);



		/*find best provider to post job*/
		findProviderForCalc(providers,providersCount,client.priority, bestProviders);
		/*create thread for best1and best2 providers*/

		createJobThread(client.hw, bestProviders, providersCount);

	}


	//unreachable
	close(sock);
}


/*find best providers (2 pieces)*/
void findProviderForCalc(Provider providers[], int providersCount,char priority, int bestProviders[]){
	int best = 1000000;			//worst
	int quality = 0;
	int count = 0;


	for (int i = 0; i < providersCount; ++i) {
		if(priority == 'C' && cflag == 1){
			for (int j = 0; j < providersCount; ++j) {
				if(providers[j].price < best){

					bestProvidersC[i] = j;
					best = providers[j].price;
				}
			}
			best = 1000000;
			providers[bestProvidersC[i]].price = 1000000;
		}

		if(priority == 'Q' && qflag == 1){
			for (int j = 0; j < providersCount; ++j) {
				if(providers[j].performance > quality){
					bestProvidersQ[i] = j;
					quality = providers[j].performance;
				}
			}
			quality = 0;
			providers[bestProvidersQ[i]].performance = 0;

		}

		if(priority == 'T' && tflag == 1){
			for (int j = 0; j < providersCount; ++j) {
				if(providers[j].duration < best){
					bestProvidersT[i] = j;
					best = providers[j].duration;
				}
			}
			best = 1000000;
			providers[bestProvidersT[i]].duration = 1000000;
		}

	}

	if(priority == 'T') {
		for (int i = 0; i < providersCount; ++i) {
			bestProviders[i] = bestProvidersT[i];
		}
		tflag = -1;

	}

	if(priority == 'Q') {
		for (int i = 0; i < providersCount; ++i) {
			bestProviders[i] = bestProvidersQ[i];
		}
		qflag = -1;

	}

	if(priority == 'C' ) {
		for (int i = 0; i < providersCount; ++i) {
			bestProviders[i] = bestProvidersC[i];
		}
		cflag = -1;

	}


}


/*create new thread and spost the provider*/
void createJobThread(int hw, int bestProviders[], int providersCount){
	pthread_t	threadsWork[MAX_JOB_COUNT];
	int			error = 0;
	int			count = 0;

	/* //best providers
	for (int i = 0; i < providersCount; ++i) {
		fprintf(stdout, "%d  %d :",
				bestProviders[i], i);
	}
	*/

	Job job;
	job.hw = hw;
	job.providersCount=providersCount;
	for (int i = 0; i < providersCount; ++i) {
		job.bestProviders[i] = bestProviders[i];
	}


	if(error = pthread_create(&threadsWork[count++], NULL, providerJob, (void *)&job) ){
		fprintf(stdout,"Failed to create thread: %s\n", strerror(error));
		exit(EXIT_FAILURE);
	}


	//sleep(1);
}



void* providerJob(void * hw) {
	int rc = 0;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);


	rc = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (rc != 0) {
		perror("Error signal handler");
	}




	/*calc info*/
//	pthread_mutex_lock(&mutexCalc);
	calcInfo.hw =   ((Job *)hw)->hw;

	//pthread_cond_broadcast(&condCalc);
//	pthread_mutex_unlock(&mutexCalc);



	/*provider*/

	pthread_mutex_lock(&mutexMain);
	if(providerFlag[((Job *)hw)->bestProviders[0]] ==-1){
		flag = ((Job *)hw)->bestProviders[0];
		providerFlag[((Job *)hw)->bestProviders[0]] = ((Job *)hw)->bestProviders[0];
	}


	pthread_cond_broadcast(&condProvider);

	pthread_mutex_unlock(&mutexMain);




	/*write second server thread*/

//	pthread_mutex_lock(&mutexCalc);


/*
	while (calcInfo.time <= 0) {
		pthread_cond_wait(&condCalc, &mutexMain);
	}
*/


	write(server_fd,&calcInfo,sizeof(calcInfo));	/*Send calc infrmation to client*/


//	pthread_mutex_unlock(&mutexCalc);



}