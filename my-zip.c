#include<stdio.h>
#include<stdlib.h>

int main(int argc, char *argv)
{
	if (argc == 1)
	{
		printf("%s\n", "my-zip: file1 [file2 ...]\n");
		exit(1);
	}
}