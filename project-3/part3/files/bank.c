/**** DUCK BANK PART 3****/
/**** Author: Brad Bailey ****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h> //mkdir
#include "account.h"
#include "string_parser.h"

/* GLOBALS */

account* accounts = NULL;
int n = 0;
long lines = 0;
long chunkSize = 0;
int count = 0;
int allDone = 0;

//barriers
//for starting workers from main
pthread_barrier_t startBarrier;
//for updating balances once count is reached
pthread_barrier_t workBarrier;
//for synching threads at the very end of all processing
pthread_barrier_t doneBarrier;

//mutexes
//use with count
pthread_mutex_t countLock;
//use with allDone
pthread_mutex_t doneLock;
//mutex array for accounts
pthread_mutex_t* accLock;

/* HELPER */

void fillAcc(char *lineBuf, size_t len, FILE *inFPtr) {
	for (int i = 0; i < n; i++) { //accounts
		//5 lines per account
		//line 1: index <i>, not needed
		getline (&lineBuf, &len, inFPtr);

		//line 2: account #
		getline (&lineBuf, &len, inFPtr);
		strtok(lineBuf, "\n");
		strncpy(accounts[i].account_number, lineBuf, 17);

		//line 3: password
		getline (&lineBuf, &len, inFPtr);
		strtok(lineBuf, "\n");
		strncpy(accounts[i].password, lineBuf, 9);

		//line 4: init balance
		getline (&lineBuf, &len, inFPtr);
		strtok(lineBuf, "\n");
		accounts[i].balance = atof(lineBuf);

		//line 5: reward rate
		getline (&lineBuf, &len, inFPtr);
		strtok(lineBuf, "\n");
		accounts[i].reward_rate = atof(lineBuf);

		//init tracker
		accounts[i].transaction_tracker = 0;

		//output file, "account*.txt" (one per account)
		snprintf(accounts[i].out_file, sizeof(accounts[i].out_file), "./output/account%d.txt", i);
	}
}

/* WORKER */

void* processTransaction(void* arg) {

	//grab and cast arg pointer
	command_line *cl = (command_line *)arg;

	//for console output
	pthread_t tid = pthread_self();

	//wait for signal from main
	pthread_barrier_wait(&startBarrier);

	int line;
	for (line = 0; line < chunkSize; line++)
	{
		//process transactions
		if (count < 5000)
		{
			//check password
			int accNum = -1;
			for (int i = 0; i < n; i++) {
				pthread_mutex_lock(&accLock[i]);
				if (strcmp(accounts[i].account_number, cl[line].command_list[1]) == 0) {
					if (strcmp(accounts[i].password, cl[line].command_list[2]) == 0) {
						accNum = i;
						pthread_mutex_unlock(&accLock[i]);
						break;
					}
				}
				pthread_mutex_unlock(&accLock[i]);
			}
			//if password checks out, process instruction
			if (accNum != -1)
			{
				//process instructions:
				//grab first letter
				char type = cl[line].command_list[0][0];
				switch(type) {

				case 'D' :

					//deposit
					pthread_mutex_lock(&accLock[accNum]);
					accounts[accNum].balance += atof(cl[line].command_list[3]);
					accounts[accNum].transaction_tracker += atof(cl[line].command_list[3]);
					pthread_mutex_unlock(&accLock[accNum]);
					//update count
					pthread_mutex_lock(&countLock);
					count++;
					pthread_mutex_unlock(&countLock);
					break;

				case 'W' :

					//withdrawal
					pthread_mutex_lock(&accLock[accNum]);
					accounts[accNum].balance -= atof(cl[line].command_list[3]);
					accounts[accNum].transaction_tracker += atof(cl[line].command_list[3]);
					pthread_mutex_unlock(&accLock[accNum]);
					pthread_mutex_lock(&countLock);
					count++;
					pthread_mutex_unlock(&countLock);
					break;

				case 'T' :

					//transfer
					for (int i = 0; i < n; i++) {
						if (strcmp(accounts[i].account_number, cl[line].command_list[3]) == 0) {
							//subtract from source account, add to tracker
							pthread_mutex_lock(&accLock[accNum]);
							accounts[accNum].balance -= atof(cl[line].command_list[4]);
							accounts[accNum].transaction_tracker += atof(cl[line].command_list[4]);
							pthread_mutex_unlock(&accLock[accNum]);
							//add to dest account, *DO NOT* add to tracker
							pthread_mutex_lock(&accLock[i]);
							accounts[i].balance += atof(cl[line].command_list[4]);
							pthread_mutex_unlock(&accLock[i]);
							break;
						}
					}
					//update count
					pthread_mutex_lock(&countLock);
					count++;
					pthread_mutex_unlock(&countLock);
					break;

				default:
					//check balance
					//essentially a no-op
					//as per project instructions, do not update count
					break;
				}
			}
			//take turns
			sched_yield();
		}
		else
		{
			//step line counter back
			line--;
			//print to console
			printf("%ld number reached, pausing.\n", tid);
			//signal bank
			pthread_barrier_wait(&workBarrier);
			//bank processing here
			//return from bank, continue processing
			pthread_barrier_wait(&workBarrier);
		}
	}//for
	printf("%ld is done.\n", tid);
	//update done count
	pthread_mutex_lock(&doneLock);
	allDone++;
	pthread_mutex_unlock(&doneLock);
	if(allDone == n) //last thread to finish, need to signal one more time
	{
		pthread_barrier_wait(&workBarrier);
	}
	while(allDone < n) //finished but other threads still working
	{
		pthread_barrier_wait(&workBarrier);
	}
	//finished, signal and exit
	pthread_barrier_wait(&doneBarrier);
	pthread_detach(tid);
}

/* BANK */

void* updateBalance(void *arg) {

	//open output files, n for "account*.txt" and +1 for "outout.txt"
	FILE *out[n+1];
	for(int i = 0; i < n; i++) {
		out[i] = fopen(accounts[i].out_file, "w");
		//first line of "account*.txt"
		fprintf(out[i], "Account %d:\n", i);
	}
	out[n] =  fopen("./output/outout.txt", "w");

	//for console output
	pthread_t tid = pthread_self();

	//tracks # of updates
	int times = 0;

	start:

	//wait for workers
	printf("bank %ld waiting now, update time is %d.\n", tid, times);
	pthread_barrier_wait(&workBarrier);
	printf("bank %ld signal received.\n", tid);

	//update balances
	for (int i = 0; i < n; i++) {
		pthread_mutex_lock(&accLock[i]);
		accounts[i].balance += (accounts[i].transaction_tracker * accounts[i].reward_rate);
		//reset tracker
		accounts[i].transaction_tracker = 0;
		pthread_mutex_unlock(&accLock[i]);
		//add to account*.txt
		fprintf(out[i], "Update #%d - Current balance:\t%f\n", times, accounts[i].balance);
	}
	//increment counter
	times++;

	//if all workers are done, finsh up
	if(allDone == n) goto end;
	//reset count
	pthread_mutex_lock(&countLock);
	count = 0;
	pthread_mutex_unlock(&countLock);
	//signal workers
	pthread_barrier_wait(&workBarrier);
	//slightly redundant, but makes sense and works (could probably drop the if() statement)
	if(allDone < n) goto start;

end:

	printf("Total updates: %d\n", times);
	//print file balances to outout.txt
	for (int i = 0; i < n; i++)
	{
		fprintf(out[n], "%d balance:\t%f\n", i, accounts[i].balance);
	}
	//close output file descriptors
	for(int i = 0; i < (n+1); i++)
	{
		fclose(out[i]);
	}
	//finished, signal and exit
	pthread_barrier_wait(&doneBarrier);
	pthread_detach(tid);
}

int main(int argc, char **argv) {

	//check for command line argument
	if (argc != 2)	{
		printf ("Usage ./bank input.txt\n");
	}

	//open file to read
	FILE *inFPtr;
	inFPtr = fopen (argv[1], "r");

	//declear line_buffer
	size_t len = 256;
	char* lineBuf = malloc (len);

	//get number of accounts
	getline (&lineBuf, &len, inFPtr);
	n = atoi(lineBuf);

	//create output directory
	char *outDir = "./output";
	mkdir(outDir, S_IRWXU);

	//parse account information (first n * 5 lines) into account struct array
	accounts = malloc(n * sizeof(account));
	fillAcc(lineBuf, len, inFPtr);

	//count number of lines and declare array accordingly
	//fill array with lines
	//pass chunks of array to separate threads
	while (getline (&lineBuf, &len, inFPtr) != -1) {
		lines++;
	}
	fclose(inFPtr);
	//set chunk size
	chunkSize = lines / n;

	//array to hold parsed instructions
	command_line* transact;
	transact = (command_line*)malloc(lines * sizeof(command_line));

	//re-open file, skip account info, and parse instructions
	inFPtr = fopen (argv[1], "r");
	for (int i = 0 ; i < ((n * 5) + 1); i++) {
		getline (&lineBuf, &len, inFPtr);
	}
	lines = 0;
	while (getline (&lineBuf, &len, inFPtr) != -1) {
		transact[lines] = str_filler(lineBuf, " ");
		lines++;
	}

	//init barriers
	//n+1 for MAIN and workers
	pthread_barrier_init(&startBarrier, NULL, n+1);
	//n+1 for BANK and workers
	pthread_barrier_init(&workBarrier, NULL, n+1);
	//n+2 for main, bank, and workers
	pthread_barrier_init(&doneBarrier, NULL, n+2);

	//init mutexes
	pthread_mutex_init(&countLock, NULL);
	pthread_mutex_init(&doneLock, NULL);
	//mutex array, one per account
	accLock = malloc(n * sizeof(pthread_mutex_t));
	for(int i = 0; i < n; i++) {
		pthread_mutex_init(&accLock[i], NULL);
	}

	//spawn workers and bank
	pthread_t tid[n + 1];

	//default attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//worker threads
	for (int i = 0; i < n; i ++) {
		//each thread works on a chunk of size: (total instructions)/(number of accounts)
		pthread_create(&tid[i], &attr, processTransaction, (void *)&transact[chunkSize * i]);
	}
	//bank thread
	pthread_create(&tid[n], &attr, updateBalance, NULL);

	//signal threads to start
	printf("Workers starting...\n");
	pthread_barrier_wait(&startBarrier);

	//wait for all threads to finish
	pthread_barrier_wait(&doneBarrier);
	printf("All threads completed!\n");

	//clean up
	//join threads
	//skip this step, using pthread_detach instead due to memory errors
	/*
	for (int i = 0; i < (n+1); i ++) {
		pthread_join(tid[i], NULL);
	}
	*/
	//destroy pthread objects
	pthread_barrier_destroy(&startBarrier);
	pthread_barrier_destroy(&workBarrier);
	pthread_barrier_destroy(&doneBarrier);
	pthread_mutex_destroy(&countLock);
	pthread_mutex_destroy(&doneLock);
	for(int i = 0; i < n; i++) {
		pthread_mutex_destroy(&accLock[i]);
	}
	pthread_attr_destroy(&attr);

	for (int i = 0; i < lines; i++) {
		free_command_line(&transact[i]);
	}
	free(accLock);
	free(transact);
	free(accounts);
	free(lineBuf);
	fclose(inFPtr);
	//pthread_exit(0);
	return 0;
}

