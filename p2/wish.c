#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>

char *wishPath[128];


int newProcess(char *segment)
{
	int len = (int)strlen(segment)+1;

	char segmentArray[len];

	strcpy(segmentArray, segment);


	char *delim = ">";
	char *cmd = strtok(segmentArray, delim);
	delim = " >";

	//bad

	char *myRedirection[128];
	int redIndex = 0;
	while(NULL != (myRedirection[redIndex++] = strtok(NULL, delim)));
	myRedirection[redIndex] = NULL;
	// set output stream

	// execute cmd
	int cmdIndex = 0;
	char *myArgv[128];
	char cmdArray[strlen(cmd)+1];
	strcpy(cmdArray, cmd);
	delim = " \t\n";
	myArgv[cmdIndex++] = strtok(cmdArray, delim);
	while(NULL != (myArgv[cmdIndex++] = strtok(NULL, delim)));
	myArgv[cmdIndex] = NULL;


	if (0 == strcmp(myArgv[0], "exit"))
	{
		exit(0);
	}
	else if (0 == strcmp(myArgv[0], "cd"))
	{
		if(cmdIndex != 3 || 0 != chdir(myArgv[1]))
		{
			return -1;
		}
	}
	else if (0 == strcmp(myArgv[0], "path"))
	{
		int pathIndex = 0;
		int myIndex = 1;
		while(myArgv[myIndex] !=  NULL)
		{
			strcpy(wishPath[pathIndex++], myArgv[myIndex++]);
		}
		wishPath[pathIndex] = NULL;
	}
	else
	{
		if (fork() == 0)
		{

			int myIndex = 0;
			while(myRedirection[myIndex] != NULL)
			{
				close(STDOUT_FILENO);
				open(myRedirection[myIndex++], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
			}

			myIndex = 0;
			while(wishPath[myIndex] != NULL)
			{
				char myAbsCmd[128];
				strcat(myAbsCmd, wishPath[myIndex]);
				strcat(myAbsCmd, "/");
				strcat(myAbsCmd, myArgv[0]);
				if(0 == access(myAbsCmd, X_OK))
				{
					execv(myAbsCmd, myArgv);
				}
				myIndex++;
			}
			exit(1);
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char error_message[30] = "An error has occurred\n";
	int readFromFile = 0;// get input from stdin
	FILE *input = stdin;
	strcpy(wishPath[0], "/bin");
	wishPath[1] = NULL;
	if(argc == 2)
		readFromFile = 1;// get input from file
	else if(argc > 2){
		write(STDERR_FILENO, error_message, strlen(error_message)); 
		exit(1);
	}
	
	if (readFromFile)
	{
		input = fopen(argv[1], "r");
		if (NULL == input)
		{
			write(STDERR_FILENO, error_message, strlen(error_message)); 
			exit(1);
		}
	}

	char *line = NULL;
	size_t len = 0;
	int terminate = 0;
	printf("%s", "wish> ");
	while(-1 != getline(&line, &len, input) && !terminate)
	{
		char lineArray[strlen(line)+1];
		strcpy(lineArray, line);
		char *delim = "&\n";
		char *token = strtok(lineArray, delim);
		if(newProcess(token) != 0)
			write(STDERR_FILENO, error_message, strlen(error_message));
		while((token=strtok(NULL, delim)) != NULL)
		{
			if(newProcess(token) != 0)
				write(STDERR_FILENO, error_message, strlen(error_message));
		}
		int status;
		pid_t wpid;
		while((wpid = wait(&status)) >0)
		{
			if(WEXITSTATUS(status) != 0)
				write(STDERR_FILENO, error_message, strlen(error_message)); 
		}
		//one line of cmd has been executed.
		printf("%s", "wish> ");
	}
	return 0;
}