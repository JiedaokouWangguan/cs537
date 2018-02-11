#include<stdio.h>
#include<stdlib.h>

typedef struct
{
	int count;
	char c;
}Node;

void decode(FILE* file);

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("%s\n", "my-unzip: file1 [file2 ...]\n");
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
	Node node;
	while(0!= fread(&node, sizeof(Node), 1, file))
	{

		for (int i = 0; i < node.count; ++i)
		{
			printf("%c", node.c);
		}
	}
}