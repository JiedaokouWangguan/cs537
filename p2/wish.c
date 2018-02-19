#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>

char *wishPath[128];


int newProcess(char *segment)
{
	int tmp11 = 0;
	while(wishPath[tmp11] != NULL)
	{
		printf("wishPath11111111%s\n", wishPath[tmp11++]);
	}

	printf("strlen%lu\n", strlen(segment)+1);

	int len = (int)strlen(segment)+1;

	char segmentArray[len];

	printf("wishPath111111112%s\n", wishPath[0]);
	printf("wishPath111111113%s\n", wishPath[0]);
	printf("wishPath111111114%s\n", wishPath[0]);

	int tmp111 = 0;
	while(wishPath[tmp111] != NULL)
	{
		printf("wishPath2222222%s\n", wishPath[tmp111++]);
	}

	strcpy(segmentArray, segment);

	int tmp1111 = 0;
	while(wishPath[tmp1111] != NULL)
	{
		printf("wishPath333333%s\n", wishPath[tmp1111++]);
	}

	char *delim = ">";
	char *cmd = strtok(segmentArray, delim);
	delim = " >";

	//bad

	char *myRedirection[128];
	int redIndex = 0;
	while(NULL != (myRedirection[redIndex++] = strtok(NULL, delim)));
	myRedirection[redIndex] = NULL;
	// set output stream


	//bad

	int tmp22 = 0;
	while(wishPath[tmp22] != NULL)
	{
		printf("wishPath4444%s\n", wishPath[tmp22++]);
	}
	// execute cmd
	int cmdIndex = 0;
	char *myArgv[128];
	char cmdArray[strlen(cmd)+1];
	strcpy(cmdArray, cmd);
	delim = " \t\n";
	myArgv[cmdIndex++] = strtok(cmdArray, delim);
	while(NULL != (myArgv[cmdIndex++] = strtok(NULL, delim)));
	myArgv[cmdIndex] = NULL;


	int tmp4 = 0;
	while(wishPath[tmp4] != NULL)
	{
		printf("wishPathBeforeIf%s\n", wishPath[tmp4++]);
	}

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
		int tmp1 = 0;
		while(wishPath[tmp1] != NULL)
		{
			printf("wishPathBefore%s\n", wishPath[tmp1++]);
		}

		int pathIndex = 0;
		int myIndex = 1;
		while(myArgv[myIndex] !=  NULL)
		{
			wishPath[pathIndex++] = myArgv[myIndex++];
		}

		tmp1 = 0;
		while(wishPath[tmp1] != NULL)
		{
			printf("wishPathAfter%s\n", wishPath[tmp1++]);
		}

	}
	else
	{
		int tmp2 = 0;
		while(wishPath[tmp2] != NULL)
		{
			printf("wishPathBeforeFork%s\n", wishPath[tmp2++]);
		}
		if (fork() == 0)
		{
			int tmp1 = 0;
			while(wishPath[tmp1] != NULL)
			{
				printf("wishPathInChild%s\n", wishPath[tmp1++]);
			}

			int myIndex = 0;
			while(myRedirection[myIndex] != NULL)
			{
				close(STDOUT_FILENO);
				open(myRedirection[myIndex++], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
			}

			myIndex = 0;
			while(wishPath[myIndex] != NULL)
			{
				printf("wishPath%s\n", wishPath[myIndex]);
				char myAbsCmd[128];
				strcat(myAbsCmd, wishPath[myIndex]);
				strcat(myAbsCmd, "/");
				strcat(myAbsCmd, myArgv[0]);

				printf("myAbsCmd%s\n", myAbsCmd);
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
	wishPath[0] = "/bin";
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