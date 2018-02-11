#include<stdio.h>
#include<stdlib.h>

typedef struct
{
	int count;
	char c;
}Node;

int main(int argc, char *argv[])
{
	

	Node n1;
	n1.count = 10;
	n1.c = 'a';
	

	Node n2;
	n2.count = 11;
	n2.c = 'b';
	
	Node n3;
	n3.count = 12;
	n3.c = 'c';

	int	len = 3;

	Node *nodes = malloc(sizeof(Node)*(len));
	nodes[0] = n1;
	nodes[1] = n2;
	nodes[2] = n3;
	fwrite(nodes, sizeof(Node), len, stdout);

	return 0;
}
