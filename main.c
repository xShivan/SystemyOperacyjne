#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

char[128] buffer;

void read_data()
{

}

void process_data()
{
}

void output_data()
{
}

int main()
{
	int p_id = fork();
	if (p_id == -1) //Shit happens
	{
		printf("An error has occured while creating first derived process!\n");
	}
	else if (p_id == 0) //OK -> Derived process
	{
		int dp_id = fork(); //dp = derived process
		if (dp_id == -1) //oh boy!
		{
			printf("An error has occured while creating second derived process!\n");
		}
		else if (dp_id == 0) //OK -> derived process of derived process (P3)
		{
			output_data();
		}
		else //OK -> derived process (P2)
		{
			process_data();
		}
	}
	else //OK -> Parent process (P1)
	{
		read_data();
	}
	return 0;
}
