/*** DUCK BANK PART 1***/
/*** Author: Brad Bailey ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "account.h"
#include "string_parser.h"

//globals: account array and number of accounts
account* accounts = NULL;
int n = 0;

void processTransaction(command_line *cl) { //worker threads

	//get account number and check that pw matches
	//if yes record the number of the account as an index into the array of accounts
	int accNum = -1;
	for (int i = 0; i < n; i++) {
		if (strcmp(accounts[i].account_number, cl->command_list[1]) == 0){
			if (strcmp(accounts[i].password, cl->command_list[2]) == 0) {
				accNum = i;
				break;
			}
		}
	}
	if (accNum != -1) {
		//process instruction
		char type = cl->command_list[0][0];
		switch(type) {
			case 'D' :
				//lock mutex and update balance
				accounts[accNum].balance += atof(cl->command_list[3]);
				accounts[accNum].transaction_tracker += atof(cl->command_list[3]);
				//unlock
				break;
			case 'W' :
				//lock mutex and update balance
				accounts[accNum].balance -= atof(cl->command_list[3]);
				accounts[accNum].transaction_tracker += atof(cl->command_list[3]);
				//unlock
				break;
			case 'T' :
				for (int i = 0; i < n; i++) {
					if (strcmp(accounts[i].account_number, cl->command_list[3]) == 0) {
						//lock mutexes and update balances
						accounts[accNum].balance -= atof(cl->command_list[4]);
						accounts[accNum].transaction_tracker += atof(cl->command_list[4]);
						accounts[i].balance += atof(cl->command_list[4]);
						//unlock
						break;
					}
				}
				break;
			default:
				//check balance
				break;
		}//switch
	}//if
}//void

void updateBalance(void *arg) { //bank thread
	//loop through accounts
	//multiply each tracker by 1+reward rate
	//print to output file
	for (int i = 0; i < n; i++) {
		accounts[i].balance += (accounts[i].transaction_tracker * accounts[i].reward_rate);
		printf("%d balance: %f\n", i, accounts[i].balance);
	}
}

void fillAcc(char *lineBuf, size_t len, FILE *inFPtr) { //need account** maybe?
	for (int i = 0; i < n; i++) { //accounts
		//5 lines per account
		//line 1
		getline (&lineBuf, &len, inFPtr); //index <i>, not needed (I think?)

		//line 2
		getline (&lineBuf, &len, inFPtr); //account #
		strtok(lineBuf, "\n");
		strncpy(accounts[i].account_number, lineBuf, 17);

		//line 3
		getline (&lineBuf, &len, inFPtr); //password
		strtok(lineBuf, "\n");
		strncpy(accounts[i].password, lineBuf, 9);

		//line 4
		getline (&lineBuf, &len, inFPtr); //init balance
		strtok(lineBuf, "\n");
		accounts[i].balance = atof(lineBuf);

		//line 5
		getline (&lineBuf, &len, inFPtr); //reward rate
		strtok(lineBuf, "\n");
		accounts[i].reward_rate = atof(lineBuf);

		accounts[i].transaction_tracker = 0;

		//TODO: out file and mutex lock
		//could return # of lines/transactions
	}
}

int main(int argc, char **argv) {

	//checking for command line argument
	if (argc != 2)	{
			printf ("Usage ./bank input.txt\n");
	}
	//opening file to read
	FILE *inFPtr;
	inFPtr = fopen (argv[1], "r");

	//declear line_buffer
	size_t len = 256;
	char* lineBuf = malloc (len);

	//get number of accounts
	getline (&lineBuf, &len, inFPtr);
	n = atoi(lineBuf);

	//parse account information into account struct array
	accounts = malloc(n * sizeof(account));
	fillAcc(lineBuf, len, inFPtr);

	//parse transaction instructions
	//TODO: spin up threads, pass command line struct

	command_line transact;
	while (getline (&lineBuf, &len, inFPtr) != -1) {
		transact = str_filler(lineBuf, " ");

		processTransaction(&transact);

		//reset transact
		free_command_line(&transact);
		memset (&transact, 0, 0);
	}

	updateBalance(NULL);

	free(accounts);
	free(lineBuf);
	fclose(inFPtr);
}
