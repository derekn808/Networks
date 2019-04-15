//Derek Nakamura
//COEN146
//Lab 1

#include <stdio.h>

int main(int argc, char*argv[])
{
	int buffer[10];
	int c;
	FILE *src, *dest;
	if(argc>1)
	{
		src=fopen(argv[1], "rb");
		dest=fopen(argv[2], "wb");
	}
	if(src==NULL)
	{
		printf("Error opening file.\n");
		return -1;
	}
	while(c=fread(buffer, sizeof(buffer), 1,src))
	{
		fwrite(buffer, sizeof(buffer), 1, dest);
	}
	fclose(src);
	fclose(dest);
	return 0;
}
