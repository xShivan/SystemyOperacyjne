#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SIZE 128
#define QUEUE_KEY 696

struct message
{
  long mtype; //1 - Terminate all processes
  int length;
  char text[SIZE];
};
int queue_id = 0;

short ok = 1;
short suspend = 0;

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
    if (!suspend)
    {
  		if ((tmp = read(STDIN_FILENO, buffer, SIZE)) > 0 && !suspend)
  		{
  			write(fd_rp[1], buffer, tmp);
  		}
      else
      {
        fflush(stdin);
      }
    }
    else sleep(1);
	}
}

void process_data()
{
	int count = 0 , tmp, i;
	close(fd_rp[1]);
	close(fd_po[0]);
	while (ok)
	{
    if (!suspend)
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
    else sleep(1);
	}
}

void output_data()
{
	int count = 0;
	close(fd_po[1]);
	while (ok)
	{
    if (!suspend)
    {
  		if(read(fd_po[0], &count, sizeof(count)) > 0)
  		{
  			printf("Received %d characters...\n", count);
  		}
    }
    else sleep(1);
	}
}

void usr1()
{
  printf("Received SIGUSR1 at PID %d...\n", getpid());
  suspend = 1;
  printf("Execution paused!\n");
}



void usr2()
{
  printf("Received SIGUSR2 at PID %d...\n", getpid());
  suspend = 0;
  printf("Execution resumed!\n");
}

void ctrlc()
{
  int pid_current = getpid();
  printf("Received SIGINT at PID %d...\n", pid_current);
  exit(0);
}

void cont()
{
  printf("Received SIGCONT at PID %d...\n", getpid());
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
  //pid_parent = getpid();
	register_signal_handlers();

	pipe(fd_rp);
	pipe(fd_po);

	int p_id = fork();
	if (p_id == -1)
	{
		printf("An error has occured while creating first derived process!\n");
	}
	else if (p_id == 0)
	{
			printf("Data processing at PID %d\n", getpid());
			process_data();
	}
  //pid_processor = p_id;

  p_id = fork();
  if (p_id == -1)
  {
    printf("An error has occured while creating second derived process!\n");
  }
  else if (p_id == 0)
  {
    printf("Data output at PID %d\n", getpid());
    output_data();
  }
  //pid_output = p_id;

  printf("Data reader at PID %d\n", getpid());
  read_data();
	return 0;
}
