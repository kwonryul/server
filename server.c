#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define REG_LEN 10
#define BUF_LEN 256

int flag = 0;

void sig_handler(int signo)
{
	flag = 1;
}

int main()
{
/* Declaration */
	unsigned char reg[REG_LEN];
	char buf[BUF_LEN];

	pid_t pid;
	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t len;

	int log_fd;

	int file_fd;
	int validity;

	int msg_size;

/* Fork */
	pid = fork();

/* Parent : print pid */
	if(pid != 0)
	{
		reg[0] = 0;

		while(1)
		{
			buf[reg[0]] = '0' + pid % 10;
			if(pid < 10)
			{
				break;
			}
			pid /= 10;
			reg[0]++;
		}

		while(1)
		{
			write(1, buf + reg[0], 1);
			if(reg[0] == 0)
			{
				break;
			}
			reg[0]--;
		}

		return 0;
	}

/* Child : daemon with siganl handler */
	setsid();

	signal(SIGINT, sig_handler);

	log_fd = open("./log.txt", O_WRONLY|O_CREAT|O_TRUNC|O_APPEND);

/* Making socket */
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		write(log_fd, "\n\tsocket error\n", 15);
		close(log_fd);
		return 0;
	}

	memset(&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		write(log_fd, "\n\tbind error\n", 13);
		close(server_fd);
		close(log_fd);
		return 0;
	}

	if(listen(server_fd, 5) < 0)
	{
		write(log_fd, "\n\tlisten error\n", 15);
		close(server_fd);
		close(log_fd)
		return 0;
	}

/* Action per new client */
	while(1)
	{
		/* Accept */
		if((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len)) < 0)
		{
			write(log_fd, "\n\taccept error\n", 15);
			continue;
		}

		write(log_fd, "\n\tNEW CLIENT\n", 13);
		write(client_fd, "HTTP/1.1 ", 9);

		/* Read till GET */
		while(1)
		{
			buf[2] = buf[1];
			buf[1] = buf[0];
			read(client_fd, buf, 1);

			write(log_fd, buf, 1);

			if(buf[2] == 'G'&&buf[1] == 'E'&&buf[0] =='T')
			{
				break;
			}
		}	

		/* Ignore blanks */
		while(1)
		{
			read(client_fd, reg, 1);
			write(log_fd, reg, 1);

			if(reg[0] == ' ')
			{
				continue;
			}

			break;
		}

		/* Get file directory */
		reg[4] = 0;

		while(1)
		{
			reg[3] = reg[2];
			reg[2] = reg[1];
			reg[1] = reg[0];
			read(client_fd, reg, 1);

			write(log_fd, reg, 1);

			if(reg[3] == 'H'&&reg[2] == 'T'&&reg[1] == 'T'&&reg[0] == 'P')
			{
				buf[reg[4] - 4] = '\0';
				break;
			}

			if(reg[0] == '?')
			{
				buf[reg[4]] = '\0';
				break;
			}

			buf[reg[4]] = reg[0];
			reg[4]++;
			if(reg[4] == BUF_LEN - 1)
			{
				write(log_fd, "\n\tBUF_LEN error\n", 15);
				write(client_fd, "414\n", 4);
				validity = 1;
				break;
			}
		}

		/* Valiate file directory */
		if(validity == 0)
		{
			if((file_fd = open(buf, O_RDONLY)) < 0)
			{
				write(log_fd, "\n\tfile error\n", 13);
				write(client_fd, "404\n", 4);
				validity = 1;
			}
		}

		/* Complete reception */
		while(1)
		{
			msg_size = read(client_fd, buf, BUF_LEN);
			write(log_fd, buf, msg_size);

			if(msg_size < BUF_LEN)
			{
				break;
			}
		}

		/* Send header */
		write(client_fd, "Content-Type: text/html; charset=UTF-8\n", 39);
		write(client_fd, "Content-Encoding: UTF-8\n", 24);
		write(client_fd, "Server: Paper Co.\n", 18);
		write(client_fd, "Conection: close", 16);
		write(client_fd, "\n", 1);

		/* Validity error */
		if(validity == 1)
		{
			write(client_fd, "error", 5);
			close(client_fd);
			continue;
		}

		/* Send file */
		while(1)
		{
			msg_size = read(file_fd, buf, BUF_LEN);
			write(client_fd, buf, msg_size);
			if(msg_size < BUF_LEN)
			{
				break;
			}
		}

		/* Close client socket */
		close(file_fd);

		close(client_fd);
	}

/* End */
	close(server_fd);
	write(log_fd, "\n\tport closed", 13);
	close(log_fd);
	return 0;
}
