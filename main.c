#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
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

void usr1()
{
	printf("Received SIGUSR1 at PID %d\n", getpid());
}

void usr2()
{
  printf("Received SIGUSR2 at PID %d\n", getpid());
}

void ctrlc()
{
  printf("Received SIGINT at PID %d\n", getpid());
}

void cont()
{
  printf("Received SIGCONT at PID %d\n", getpid());
}

//Call with kill -s. -n switch may break on some systems!
void register_signal_handlers()
{
  if(signal(SIGINT, ctrlc) == SIG_ERR || signal(SIGUSR1, usr1) == SIG_ERR || signal(SIGUSR2, usr2) == SIG_ERR || signal(SIGCONT, cont) == SIG_ERR)
	{
		printf("Nie można przechwycić sygnału\n");
  }
}



int main()
{
	register_signal_handlers();

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
			printf("Data output at PID %d\n", getpid());
			output_data();
		}
		else //OK -> derived process (P2)
		{
			printf("Data processing at PID %d\n", getpid());
			process_data();
		}
	}
	else //OK -> Parent process (P1)
	{
		printf("Data reader at PID %d\n", getpid());

		read_data();
	}
	return 0;
}
