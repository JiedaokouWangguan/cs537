#include<stdio.h>
#include<stdlib.h>


void decode(FILE* file);

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("%s\n", "my-unzip: file1 [file2 ...]");
		exit(1);
	}

	for (int i = 1; i < argc; ++i)
	{
		FILE *file = fopen(argv[i], "rb");
		if (NULL == file)
		{
			printf("%s\n", "my-grep: cannot open file");
			exit(1);
		}
		decode(file);
	}
	return 0;
}

void decode(FILE* file)
{
	int count;
	while(0!= fread(&count, sizeof(int), 1, file))
	{
		char c; 
		fread(&c, sizeof(char), 1, file);

		for (int i = 0; i < count; ++i)
		{
			printf("%c", c);
		}
	}
}
