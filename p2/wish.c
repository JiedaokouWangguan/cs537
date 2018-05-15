#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
char *wishPath[128]; // first of all
int newProcess(char *segment){ // believe me
	char segmentArray[strlen(segment)+1]; // it's readable
	strcpy(segmentArray, segment); // for sure!!
	int hasRed = 1;
	if(NULL == strchr(segment, '>'))hasRed = 0; //check if there is a redirection
	char *delim = ">";
	char *cmd = strtok(segmentArray, delim); // get the cmd
	delim = " >";
	char *myRedirection[128];
	int redIndex = 0;
	while(NULL != (myRedirection[redIndex++] = strtok(NULL, delim))); // get all the output files(if not only one)
	myRedirection[redIndex] = NULL;
	if((redIndex == 1 && hasRed) || redIndex > 2) return -1; // if there a '>' but no output file or if there are multiple output files
	int cmdIndex = 0;
	char *myArgv[128];
	char cmdArray[strlen(cmd)+1];
	strcpy(cmdArray, cmd);
	delim = " \t\n";
	myArgv[cmdIndex++] = strtok(cmdArray, delim); // get arguments
	while(NULL != (myArgv[cmdIndex++] = strtok(NULL, delim)));
	myArgv[cmdIndex] = NULL;
	if(myArgv[0] == NULL) return 0; // check if the cmd(myArgv[0]) is NULL
	if (0 == strcmp(myArgv[0], "exit")){ // first compare the cmd with the three build-in cmd
		if(cmdIndex != 2) // if the cmd exit has arg
			return -1;
		exit(0);
	}
	else if (0 == strcmp(myArgv[0], "cd")){
		if(cmdIndex != 3 || 0 != chdir(myArgv[1])) // if cd has zero or more than one args or if the directory is bad
			return -1;
	}
	else if (0 == strcmp(myArgv[0], "path")){
		int pathIndex = 0;
		while(wishPath[pathIndex] != NULL){ // free the old path first
			free(wishPath[pathIndex]);
			wishPath[pathIndex++] = NULL;
		}
		pathIndex = 0;
		int argIndex = 1; // build the new path
		while(myArgv[argIndex] !=  NULL){
			wishPath[pathIndex] = (char *)malloc(sizeof(char) * (strlen(myArgv[argIndex]) +1));
			strcpy(wishPath[pathIndex++], myArgv[argIndex++]);
		}
		wishPath[pathIndex] = NULL;
	}
	else{
		int myIndex = 0; // if the cmd is not a build-in cmd
		while(wishPath[myIndex] != NULL)
		{
			char myAbsCmd[128];
			strcpy(myAbsCmd, wishPath[myIndex]);
			strcat(myAbsCmd, "/");
			strcat(myAbsCmd, myArgv[0]); // concatenate the cmd
			if(0 == access(myAbsCmd, X_OK)){ // if it's accessible
				if (fork() == 0){
					if(hasRed){
						close(STDOUT_FILENO); // if there is a redirection
						open(myRedirection[0], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
					}
					execv(myAbsCmd, myArgv);
				}
				else{return 0;}
			}
			myIndex++;
		}
		return -1;
	}
	return 0;
}
int main(int argc, char *argv[]){
	char error_message[30] = "An error has occurred\n";
	int readFromFile = 0;// get input from stdin
	FILE *input = stdin;
	wishPath[0] = (char *)malloc(sizeof(char) * 5);
	strcpy(wishPath[0], "/bin");
	wishPath[1] = NULL;
	if(argc == 2)
		readFromFile = 1;// get input from file
	else if(argc > 2){
		write(STDERR_FILENO, error_message, strlen(error_message)); 
		exit(1);
	}
	if (readFromFile){
		input = fopen(argv[1], "r"); // if we are going to read cmd from file, then we open the file
		if (NULL == input){
			write(STDERR_FILENO, error_message, strlen(error_message)); 
			exit(1);
		}
	}
	char *line = NULL;
	size_t len = 0;
	if(!readFromFile){	
		printf("%s", "wish> ");
		fflush(stdout); // print prompt
	}
	while(-1 != getline(&line, &len, input)){
		char *tokens[128];
		int tokenIndex = 0;
		char lineArray[strlen(line)+1];
		strcpy(lineArray, line);
		char *delim = "&\n";
		tokens[tokenIndex++] = strtok(lineArray, delim);
		while((tokens[tokenIndex++]=strtok(NULL, delim)) != NULL);
		tokenIndex = 0;
		while(tokens[tokenIndex]!= NULL){ // split the line with "&" to implement "parallel"
			if(newProcess(tokens[tokenIndex++]) != 0)
				write(STDERR_FILENO, error_message, strlen(error_message));
		}
		int status;
		pid_t wpid;
		while((wpid = wait(&status)) >0); // wait for all the child processes
		if(!readFromFile){	
			printf("%s", "wish> "); // print prompt
			fflush(stdout);
		}
	}
	return 0;
}
