#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>

/*
1--exit(build-in cmd)
2--illegal redirection
3--can't open file


*/
#define BUILD_IN_EXIT (1)
#define ILLEGAL_REDIRECTION (2)
#define CANNOT_OPEN_FILE (3)
#define REDIRECT_STDOUT_FAILED (4)


int isLegalRedirection(char *cmd)
{
	int index = 0;
	while(cmd[index] != '\0')
	{
		if(cmd[index] == ' ')
			return ILLEGAL_REDIRECTION;
	}
}

int main(int argc, char *argv[])
{
	int readFromFile = 0;// get input from stdin
	FILE *input = stdin;

	if(argc == 2)
		readFromFile = 1;// get input from file
	else if(argc > 2){
		printf("%s\n", "wish: wish [file]");//error
		exit(1);
	}
	
	if (readFromFile)
	{
		input = fopen(argv[i], "r");
		if (NULL == file)
		{
			printf("%s\n", "wish: cannot open file");
			exit(2);
		}
	}

	char *line = NULL;
	size_t len = 0;
	int terminate = 0;
	while(-1 != getline(&line, &len, input) && !terminate)
	{
		//strcmp(line, "exit") != 0;
		char lineArray[strlen(line)+1];
		strcpy(lineArray, line);
		char *delim = "&";
		char *token = strtok(lineArray, delim);
		while(token != NULL)
		{
			char segmentArray[strlen(segment)+1];
			strcpy(segmentArray, segment);
			char *delim = ">";
			char *cmd; = strtok(lineArray, delim);
			char *redirction = strtok(NULL, delim);
			if ((result = isLegalRedirection(redirction)) > 0)
			{
				return result;
			}
			if (NULL != redirection)
			{
				FILE *file = open(redirction, O_WRONLY|O_TRUNC|O_CREAT, 0777);
				if (NULL == file)
				{
					return CANNOT_OPEN_FILE;
				}
			}


			if (fork() == 0)
			{
				if (NULL != redirection)
				{
					if (dup2(file, stdout) == -1)
					{
						return REDIRECT_STDOUT_FAILED;
					}
					close(file);
				}


				char cmdArray[strlen(cmd)+1];
				strcpy(cmdArray, cmd);
				char *de = " \t";
				char *tokens[strlen(cmd)+1];
				char *token = strtok(lineArray, delim);

				

				
				int indexToken = 0;
				while()
				char *redirction = strtok(NULL, delim);




			}

			token = strtok(NULL, delim);
		}

		int status;
		pid_t wpid;
		while((wpid = wait(&status)) >0);
		//one line of cmd has been executed.
	}
}