#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>

#define SIZE 128
#define KEY 1234

short ok = 1;
short suspend_reader = 0;
short suspend_processor = 0;
short suspend_output = 0;

int pid_parent = -1;
int pid_processor = -1;
int pid_output = -1;

int job = -1; //1 = reader, 2 = processor, 3 = output

//Obsługa kolejki komunikatów

struct msgbuf {
	long mtype;
	char mtext[SIZE];
	int order; //1 - stop processing, 2 - stop outputting, 3 - start processing, 4 - start outputting
};

int msqid;
int msgflg;
key_t key;

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
		if (!suspend_reader)
		{
			if ((tmp = read(STDIN_FILENO, buffer, SIZE)) > 0 && !suspend_reader)
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
		if((tmp = read(fd_rp[0], buffer, SIZE)) > 0 && !suspend_processor)
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
		if(read(fd_po[0], &count, sizeof(count)) > 0 && !suspend_output)
		{
			if (!suspend_output) printf("Received %d characters...\n", count);
		}
	}
}

void send_message(int type)
{
	struct msgbuf sndbuffer = { 2, "Message queue", type };

	msqid = msgget(key, 0666 | IPC_CREAT);
	msgsnd(msqid, &sndbuffer, sizeof(struct msgbuf) - sizeof(long), 0);
}

//Hold execution
void usr1()
{
	printf("Received SIGUSR1 at PID %d\n", getpid());
	if (pid_parent == getpid())
	{
		send_message(1);
		send_message(2);

		//Let know derived processes
		kill(pid_processor, SIGCONT);
		kill(pid_output, SIGCONT);
		suspend_reader = !suspend_reader;
	}
	else
	{
		kill(pid_parent, SIGCONT);
	}
}

void usr2()
{
  printf("Received SIGUSR2 at PID %d\n", getpid());
	if (pid_parent == getpid())
	{
		send_message(3);
		send_message(4);
		kill(pid_processor, SIGCONT);
		kill(pid_output, SIGCONT);
		suspend_reader = !suspend_reader;
	}
	else
	{
		kill(pid_parent, SIGCONT);
	}
}

void ctrlc()
{
  printf("Received SIGINT at PID %d\n", getpid());
	exit(0);
}

void cont()
{
  printf("Received SIGCONT at PID %d\n", getpid());
	if (pid_parent == getpid()) //SIGUSR1 was sent
	{
		if (!suspend_reader)
		{
			send_message(1);
			send_message(2);
		}
		else
		{
			send_message(3);
			send_message(4);
		}
		kill(pid_processor, SIGCONT);
		kill(pid_output, SIGCONT);

		suspend_reader = !suspend_reader;
	}
	else
	{
		printf("Handling by subprocess %d\n", getpid());
		struct msgbuf rcvbuffer;
		msqid = msgget(key, 0666 | IPC_CREAT);
		msgrcv(msqid, &rcvbuffer, sizeof(struct msgbuf) - sizeof(long), 2, 0);
		printf("Rozkaz: %s\n", rcvbuffer.mtext);
		if (job == 2)
		{
			if (rcvbuffer.order == 1)
			{
				printf("Processing suspended\n");
				suspend_processor = 1;
			}
			else if (rcvbuffer.order == 3)
			{
				printf("Processing started\n");
				suspend_processor = 0;
			}
		}
		else if (job == 3)
		{
			if (rcvbuffer.order == 2)
			{
				printf("Output suspended\n");
				suspend_output = 1;
			}
			else if (rcvbuffer.order == 4)
			{
				printf("Output started\n");
				suspend_output = 0;
			}
		}

	}
}

//Call with kill -s. -n switch may break on some systems!
void register_signal_handlers()
{
  if(signal(SIGINT, ctrlc) == SIG_ERR || signal(SIGUSR1, usr1) == SIG_ERR || signal(SIGUSR2, usr2) == SIG_ERR || signal(SIGCONT, cont) == SIG_ERR)
	{
		printf("Unable to intercept the signal!\n");
  }
}



int main()
{
	pid_parent = getpid();
	register_signal_handlers();

	key = KEY;

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
			job = 2;
			process_data();
	}
	pid_processor = p_id;

	p_id = fork();
	if (p_id == -1)
	{
		printf("An error has occured while creating second derived process!\n");
	}
	else if (p_id == 0)
	{
		printf("Data output at PID %d\n", getpid());
		job = 3;
		output_data();
	}
	pid_output = p_id;

	printf("Data reader at PID %d\n", getpid());
	job = 1;
	read_data();
	return 0;
}
