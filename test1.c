#include<stdio.h>
#include<stdlib.h>

typedef struct{
	int count;
	char c;
}Node;

int main(int argc, char *argv[])
{
	FILE *f = fopen("re.txt", "rb");
	Node *nodes = malloc(sizeof(Node)*10);
	int len = 3;
	fread(nodes, sizeof(Node), len, f);
	printf("n1.count=%d, n1.c=%c.\n", nodes[0].count, nodes[0].c);
	printf("n2.count=%d, n2.c=%c.\n", nodes[1].count, nodes[1].c);
	printf("n3.count=%d, n3.c=%c.\n", nodes[2].count, nodes[2].c);
	return 0;
}
