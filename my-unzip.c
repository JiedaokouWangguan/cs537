#include<stdio.h>
#include<stdlib.h>

typedef struct
{
	int count;
	char c;
}Node;

typedef struct nodeList
{
	Node *node;
	struct nodeList *next;
}NList;

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("%s\n", "my-unzip: file1 [file2 ...]\n");
		exit(1);
	}

	for (int i = 0; i < argc; ++i)
	{
		FILE *file = fopen(argv[i], "rb");
		if (NULL == file)
		{
			printf("%s\n", "my-grep: cannot open file");
			exit(1);
		}
		RLE(file)
	}
	return 0;
}

void decode(FILE* file)
{
	Node node;
	NList dummy;
	NList *current = &dummy;
	int count = 0;
	while(0!= fread(&node, sizeof(Node), 1, file))
	{
		count += node.count;
		Node tmp;
		tmp.count = node.count;
		tmp.c = node.c;
		NList nl;
		nl.node = &tmp;
		current->next = &nl;
		current = current->next;
	}

	char result[count+1];
	NList* head = dummy.next;
	int index = 0;
	while(head != NULL)
	{
		for (int i = 0; i < head->count; ++i)
		{
			result[index++] = head->c;
		}
	}
	result[count] = '\0';
	printf("%s\n", result);
}