/*
 * string_parser.c
 *
 *  Created on: Nov 25, 2020
 *      Author: gguan
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GNU_SOURCE

int count_token (char* buf, const char* delim)
{
	char *str, *freePtr;
	freePtr = str = strdup(buf);

	char *token, *saveptr;
	int tok_cnt = 0;

	for (int i = 1; ; i++, str = NULL) {
		token = strtok_r(str, delim, &saveptr);
		if (token == NULL)
			break;
		tok_cnt++;
	}
	free(freePtr);
	//return + 1 to allow space for the terminating NULL
	return (tok_cnt+1);
}

command_line str_filler (char* buf, const char* delim)
{
	command_line cl;

	//check for NULL string
	if(buf == NULL) {
		printf("ERROR: null string\n");
		cl.num_token = 0;
		cl.command_list = NULL;
		return cl;
	}

	//remove newline char
	strtok(buf, "\n");

	//pass into count_token
	cl.num_token = count_token(buf, delim);

	char *str, *freePtr;
	freePtr = str = strdup(buf);

	char *token, *saveptr;

	//need to allocate num_token + 1 b/c of the terminating NULL entry
	cl.command_list = (char **)malloc(sizeof(char *) * (cl.num_token));

	//initialize each entry to NULL first
	//need separate loop to implement the exit command:
	//if we get an exit token we want to make sure all subsequent
	//array entries are NULL so the logic in lab1.c works
	int i;
	for (i = 0; i < cl.num_token; i++) {
		cl.command_list[i] = NULL;
	}

	for (i = 0; ; i++, str = NULL){
		//tokenize
		token = strtok_r(str, delim, &saveptr);
		if (token == NULL)
			break;

		//dupe tokens into member array
		cl.command_list[i] = strdup(token);
	}

	//free the dup'd string
	free(freePtr);
	return cl;
}


void free_command_line(command_line* command)
{
	//TODO：
	/*
	*	#1.	free the array base num_token
	*/
	for (int i = 0; i < (command->num_token); i++) {
		free(command->command_list[i]);
	}
	free(command->command_list);
}
