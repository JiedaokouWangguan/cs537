#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		printf("%s\n", "my-grep: searchterm [file ...]\n");
		exit(1);
	}
	
	char *line = NULL;
	size_t len = 0;
	if(argc == 2)
	{
		while(-1 != getline(&line, &len, stdin))
			if(NULL != strstr(line, argv[1]))
				printf("%s", line);
	}
	else
	{
		for(int i = 2;i<argc;i++)
		{
			FILE *file = fopen(argv[i], "r");
			if(NULL == file)
			{
				printf("%s\n", "my-grep: cannot open file");
				exit(1);
			}
			while(-1 != getline(&line, &len, file))
				if(NULL != strstr(line, argv[1]))	
					printf("%s", line);
		}
	}
	return 0;
}
