/*
20123434 hwang jung dam
20123347 kim ji seon
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUF 1024
#define myDatatype ClientInfo


typedef struct ClientInfo
{
	char username[32];
	char userIP[16];
	char userPort[8];
	int sock;
}ClientInfo;

typedef struct list
{
	ClientInfo client_data;
	struct list* next;
}list;

void insertData(list* mylist, ClientInfo newClient);
int deleteData(list* mylist, int sock);
void setUserName(list* mylist, int sock, char name[], char port[]);
list* findUserforName(list* mylist, char name[]);
void writeAllClient(list* client_list, char to_clientbuf[]);

void printAllNode(list* client_list);

void display();
void error_handling(char *message);

int clientSocket = -1;

int main(int argc, char **argv)
{
	//servers
	int Serv_sock;    //tcp server socket number

	ClientInfo cli_info = { "", "", "", -1 };
	list client_list = { cli_info, NULL };
	int i = 0;

	struct sockaddr_in Server_addr;
	struct sockaddr_in Client_addr;

	int clnt_len;     //client_len
	fd_set reads, temps;  //fd_set
	int fd_max;

	char command[MAXBUF];   //1024byte

	char *userid = "server";
	char *tcpport = "8080";

	// NEED TO ADD SOME VARIABLES 
	char to_clientbuf[MAXBUF];
	char buf[MAXBUF];
	int str_len;

	if (argc >= 2){
		printf("Usage : %s", argv[0]);
		exit(1);
	}
	display();

	// NEED TO CREATE A SOCKET FOR TCP SERVER
	if ((Serv_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		error_handling("server socket error");

	// NEED TO bind   --  bind server socket
	memset(&Server_addr, 0, sizeof(Server_addr));
	Server_addr.sin_family = AF_INET;
	Server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	Server_addr.sin_port = htons(atoi(tcpport));

	if (bind(Serv_sock, (struct sockaddr*) &Server_addr, sizeof(Server_addr)) == -1)  //server bind
		error_handling("bind() error");

	// NEED TO listen
	if (listen(Serv_sock, 5) == -1)         //linten server socket
		error_handling("listen() error");

	// initialize the select mask variables and set the
	// mask with stdin and the tcp server socket

	FD_ZERO(&reads);
	FD_SET(Serv_sock, &reads);  //mask server sockfile and client sockfile
	FD_SET(0, &reads);
	fd_max = Serv_sock;//set max

	printf("%s> \n", userid);

	printf("@table : see Session\n");
	printf("@quit : exit program");
	printf("\n%s >>\n", userid);

	while (1)
	{
		int nfound;
		temps = reads;
		nfound = select(fd_max + 1, &temps, 0, 0, NULL);

		if (FD_ISSET(fileno(stdin), &temps))//input from keyboard
		{
			memset(command, 0, sizeof(command));
			fgets(command, sizeof(command), stdin);
			FD_CLR(fileno(stdin), &temps);

			if (!strcmp(command, "@table\n"))
			{
				printAllNode(&client_list);
			}
			//@quit
			else if (!strcmp(command, "@quit\n"))
			{
				sprintf(to_clientbuf, "@SQUIT\n");
				writeAllClient(&client_list, to_clientbuf);
				exit(0);
			}
			else
			{
				printf("unknown command\n");
			}

			printf("%s> \n", userid);
		}
		else if (FD_ISSET(Serv_sock, &temps)) //input from server : connection from client
		{
			char temp_buf[8];
			clnt_len = sizeof(Client_addr);
			clientSocket = accept(Serv_sock, (struct sockaddr *) &Client_addr, &clnt_len);
			FD_SET(clientSocket, &reads);

			if (fd_max < clientSocket)
				fd_max = clientSocket;

			strcpy(cli_info.userIP, inet_ntoa(Client_addr.sin_addr));
			sprintf(temp_buf, "%d", ntohs(Client_addr.sin_port));
			strcpy(cli_info.userPort, temp_buf);
			cli_info.sock = clientSocket;
			printf("connection from host %s, port %d, socket %d\n", cli_info.userIP, ntohs(Client_addr.sin_port), clientSocket);
			insertData(&client_list, cli_info);
			FD_CLR(Serv_sock, &temps);
			nfound--;
		}

		//check up msg and manage client
		for (i = 0; i <= fd_max && nfound > 0; ++i)
		{
			clientSocket = i;
			if (FD_ISSET(clientSocket, &temps))
			{
				memset(buf, 0, MAXBUF);

				if ((str_len = read(clientSocket, buf, MAXBUF - 1)) <= 0) //connection closed
				{
					deleteData(&client_list, clientSocket);
					close(clientSocket);
					printf("Connection Closed %d\n", clientSocket);
					FD_CLR(clientSocket, &reads);
				}
				else //connection alives
				{
					char* token;
					char* deli = " \n";
					memset(to_clientbuf, 0, sizeof(to_clientbuf));

					//@GET <username> : checks client<username> is connected on server. return @OK or @FAIL
					token = strtok(buf, deli);
					if (token && !strcmp("@GET", token))
					{
						token = strtok(NULL, deli); // username

						list* findUser = findUserforName(&client_list, token);
						if (findUser)
							sprintf(to_clientbuf, "@OK %s %s %s\n", findUser->client_data.userIP, findUser->client_data.userPort);
						else
							sprintf(to_clientbuf, "@FAIL %s\n", token);

						write(clientSocket, to_clientbuf, strlen(to_clientbuf));
					}

					//@LOGIN <username> <port>\n
					else if (token && !strcmp("@LOGIN", token))
					{
						char *username = strtok(NULL, deli); // username
						char *userport = strtok(NULL, deli); // port
						setUserName(&client_list, clientSocket, username, userport);

					}
					//@QUIT
					else if (token && !strcmp("@QUIT", token))
					{
						deleteData(&client_list, clientSocket);
						close(clientSocket);
						printf("Connection Closed %d\n", clientSocket);
						FD_CLR(clientSocket, &reads);
					}

					//BADREQUEST
					else
					{
						sprintf(to_clientbuf, "@BAD\n");
						write(clientSocket, to_clientbuf, strlen(to_clientbuf));
					}
				}
				FD_CLR(clientSocket, &temps);
				if (--nfound <= 0)
					break;
			}
		}

	}//while End
	close(Serv_sock);
	return 0;
}//main End


void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}



void insertData(list* mylist, ClientInfo newClient)
{
	list* node = mylist;
	list* newNode = NULL;
	list* prenode = NULL;
	int i = 0;
	while (node && node->client_data.sock < newClient.sock)  //find end of list
	{
		prenode = node;
		node = node->next;
	}
	//printf("\ninsert sock OK\n");
	newNode = (list*)malloc(sizeof(list));
	memcpy(&newNode->client_data, &newClient, sizeof(newClient));
	newNode->next = prenode->next;
	prenode->next = newNode;
}

int deleteData(list* mylist, int sock)
{
	list* node = mylist;
	list* p = NULL;
	int ret_sock;
	while (node && node->client_data.sock != sock)
	{
		p = node;
		node = node->next;
	}
	if ((node != mylist) && node)
	{
		p->next = node->next;
		ret_sock = node->client_data.sock;
		printf("DELETE %s\n", node->client_data.username);
		free(node);
		return ret_sock;
	}
	else if (!node || (node == mylist))
	{
		return -1;
	}
	else
		return -1;
}


void setUserName(list* mylist, int sock, char name[], char port[])
{
	//printf("\nsetusername\n");
	list* node = mylist;
	list* prenode = NULL;
	while (node && node->client_data.sock != sock)
		node = node->next;
	if (node)
	{
		strcpy(node->client_data.username, name);
		strcpy(node->client_data.userPort, port);
	}
	else
		printf("(setUserName)fail to find sock# %d\n", sock);
}

list* findUserforName(list* mylist, char name[])
{
	//printf("\nfindusername\n");
	list* node = mylist->next;
	while (node && strcmp(node->client_data.username, name))
		node = node->next;
	return node;
}

void printAllNode(list* mylist)
{
	list* node = mylist->next;
	int i = 0;
	printf("ID        NAME         IP           PORT  SOCK\n");
	printf("----------------------------------------------\n");
	while (node)
	{
		printf("%3d  %10s      %s    %7s  %3d\n", i++, node->client_data.username, node->client_data.userIP, node->client_data.userPort, node->client_data.sock);
		node = node->next;
	}
	printf("----------------------------------------------\n");
}


void writeAllClient(list* client_list, char to_clientbuf[])
{
	list* node = client_list->next;

	while (node)
	{
		write(node->client_data.sock, to_clientbuf, strlen(to_clientbuf));
		close(node->client_data.sock);
		node = node->next;
	}
}

void display()
{
	printf("20123434 hwang jung dam \n");
	printf("20123347 kim ji seon \n");
}
