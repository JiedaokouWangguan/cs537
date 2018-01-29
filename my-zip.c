#include<stdio.h>
#include<stdlib.h>

typedef struct
{
	int count;
	char c;
}Node;

void RLE(char* src, int len);

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("%s\n", "my-zip: file1 [file2 ...]\n");
		exit(1);
	}

	char *line = NULL;
	size_t len = 0;

	for (int i = 1; i < argc; ++i)
	{
		FILE *file = fopen(argv[i], "r");
		if (NULL == file)
		{
			printf("%s\n", "my-grep: cannot open file");
			exit(1);
		}
		int read = 0;
		while(-1 != (read=getline(&line, &len, file)))
		{	
			RLE(line, read);
		}
	}

	return 0;
}

void RLE(char* src, int len)
{

	Node *nodes = (Node *)malloc(sizeof(Node)*len);
	int nLen = 0;
	
	int count1 = 0;

	int i = 0;
	int j = 0;
	while(i < len)
	{
		Node n;
		n.c = src[i++];
		nLen = 1;
		count1++;

		while(i < len && src[i] == src[i-1])
		{
			nLen++;
			i++;
		}
		n.count = nLen;
		nodes[j++] = n;
	}

	fwrite(nodes, sizeof(Node), j, stdout);
	free(nodes);
	nodes = NULL;
}
