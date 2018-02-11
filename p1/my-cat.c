#include<stdio.h>
#include<stdlib.h>
#define BUFFER_SIZE (256)

int main(int argc, char* argv[])
{
	if (argc == 1)
		return 0;
	for (int i = 1; i < argc; ++i)
	{
		FILE *file = fopen(argv[i], "r");
		if (file == NULL)
		{
			printf("my-cat: cannot open file\n");
			exit(1);
		}
		char buffer[BUFFER_SIZE];
		while(NULL != fgets(buffer, BUFFER_SIZE, file))
			printf("%s", buffer);
	}
	return 0;

}
