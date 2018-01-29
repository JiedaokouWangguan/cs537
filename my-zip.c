#include<stdio.h>
#include<stdlib.h>

typedef struct
{
	int count;
	char c;
}Node;

int main(int argc, char *argv)
{
	if (argc == 1)
	{
		printf("%s\n", "my-zip: file1 [file2 ...]\n");
		exit(1);
	}

	char *line = NULL;
	size_t len = 0;
	node_line *dummy = NULL;
	dummy = malloc(sizeof(node_line));
	node_line *current = dummy;

	for (int i = 0; i < argc; ++i)
	{
		FILE *file = fopen(argv[i], "r");
		if (NULL == file)
		{
			printf("%s\n", "my-grep: cannot open file");
			exit(1);
		}
		while(-1 != getline(&line, &len, file))
			RLE(line, len)
	}

	return 0;
}

void RLE(char* src, int len)
{
	Node *nodes = (Node *)malloc(sizeof(Node)*len);
	int nLen = 0;

	int i = 0;
	int j = 0;
	while(i < len)
	{
		Node n;
		n.c = src[i++];
		nLen = 1;

		while(i < len && src[i] == src[i-1])
		{
			nLen++;
			i++;
		}
		node.count = nLen;
		nodes[j++] = node;
	}

	fwrite(nodes, sizeof(Node), len, stdout);
	free(nodes);
	nodes = NULL;
}