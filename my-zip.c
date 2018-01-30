#include<stdio.h>
#include<stdlib.h>

#define MAX_BUFFER_SIZE (1024)
typedef struct
{
	int count;
	char c;
} __attribute__((packed)) Node;


int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("%s\n", "my-zip: file1 [file2 ...]");
		exit(1);
	}
	char *line = NULL;
	size_t len = 0;
	int bufferIndex = 0;
	Node nodes[MAX_BUFFER_SIZE];
	int nLen = 0;
	char lastC = '\0';
	for (int j = 1; j < argc; ++j)
	{
		FILE *file = fopen(argv[j], "r");
		if (NULL == file)
		{
			printf("%s\n", "my-grep: cannot open file");
			exit(1);
		}
		int read = 0;
		while(-1 != (read=getline(&line, &len, file)))
		{	

			int i = 0;
			while(i < read)
			{
				if (line[i] == lastC)
				{
					nLen++;
					i++;
				}
				else if(lastC != '\0')
				{
					Node n;
					n.count = nLen;
					n.c = lastC;
					nodes[bufferIndex++] = n;
					if (bufferIndex == MAX_BUFFER_SIZE)
					{
						fwrite(nodes, sizeof(Node), MAX_BUFFER_SIZE, stdout);
						bufferIndex = 0;
					}
					lastC = line[i];
					nLen = 1;
					i++;
				}
				else
				{
					lastC = line[i];
					nLen = 1;
					i++;	
				}


				while(i < read && line[i] == lastC)
				{
					nLen++;
					i++;
				}
				if (i < read)
				{
					Node n;
					n.count = nLen;
					n.c = lastC;
					nodes[bufferIndex++] = n;
					if (bufferIndex == MAX_BUFFER_SIZE)
					{
						fwrite(nodes, sizeof(Node), MAX_BUFFER_SIZE, stdout);
						bufferIndex = 0;
					}
					lastC = line[i];
					nLen = 1;
					i++;
				}
			}
		}
	}
	if (nLen != 0)
	{
		Node n;
		n.count = nLen;
		n.c = lastC;
		nodes[bufferIndex++] = n;
	}
	fwrite(nodes, sizeof(Node), bufferIndex, stdout);
	return 0;
}

