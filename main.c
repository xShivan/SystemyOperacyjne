#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#define SIZE 128
short ok = 1;
 
//String buffer
char buffer[SIZE];
 
//Data flow control
int fd_rp[2]; //read-proc
int fd_po[2]; //proc-out
 
void read_data()
{
	int tmp;
	close(fd_rp[0]);
	while (ok)
	{
		if ((tmp = read(STDIN_FILENO, buffer, SIZE)) > 0)
		{
			write(fd_rp[1], buffer, tmp);
		}
	}
}
 
void process_data()
{
	int count = 0 , tmp, i;
	close(fd_rp[1]);
	close(fd_po[0]);
	while (ok)
	{
		if((tmp = read(fd_rp[0], buffer, SIZE)) > 0)
		{		   
			for(i = 0; i < tmp; i++)
			{
				if(buffer[i] == '\n')
				{
					write(fd_po[1], &count, sizeof(count));
					count = 0;
				}
				else count++;
			}		
		}
	}
}
 
void output_data()
{
	int count = 0;
	close(fd_po[1]);
	while (ok)
	{
		if(read(fd_po[0], &count, sizeof(count)) > 0)
		{
			printf("Received %d characters...\n", count);
		}
	}
}
 
int main()
{
	   
	pipe(fd_rp);
	pipe(fd_po);
   
	int p_id = fork();
	if (p_id == -1) //AWW! Shit happens
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

