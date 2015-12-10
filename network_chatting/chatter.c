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

#define MEMMAX 1024

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
void setUserPort(list* mylist, int sock, char port[]);
void getUserlist(list* mylist, char msghead[]);
void error_handling(char *message);
void sendMessagetoCli(list* mylist, char msg[]);
void closeAllClient(list* client_list, char to_clientbuf[]);
void printAllNode(list* mylist);
void setUserName(list* mylist, int sock, char name[], char port[]);
void display();
list* findUserforSock(list* mylist, int sock);

int main(int argc, char** argv){
	
	ClientInfo cli_info = { "", "", "",  -1 };
	ClientInfo wait_cli_info = { "", "", "", -1 };
	list client_list = { cli_info, NULL };
	list wait_list = { wait_cli_info, NULL };

	int my_sock;
	int server_sock=-1;
	int temp_sock=-1;

	struct sockaddr_in my_add;
	struct sockaddr_in server_add;
	struct sockaddr_in temp_add;

	int invit_count=0;
	int fd_num;
	int socket_fd = -1;
	int you_len;	
	int fd_max;
	
	fd_set mask,temp;

	char *delimiter = " \n";

	char* userid= NULL;
	char* ip = NULL;
	char* tcpport = NULL;		

	
	char* temp_connect_ip=NULL;
	char* temp_connect_port=NULL;

	char send_to_server[MEMMAX] = {0};
	char send_to_client[MEMMAX] = {0};
	char recive_from_client[MEMMAX] = {0};

	int is_chat = 0;

	char struserid[4];
	char strip[5];
	char strtcpport[4];	
	int loopback=1;


	if (argc >= 2){
		printf("Usage : %s", argv[0]);
		exit(1);
	}
	display();

	while(loopback==1){
		printf("enter the @login <userid> <ip> <tcpport>\n");
		char firstMsg[100];
		scanf("%s %s %s %s", &firstMsg, &struserid, &strip, &strtcpport);
		if(strcmp(firstMsg, "@login")==0){
			userid=struserid;
			ip=strip;
			tcpport=strtcpport;
			loopback=0;
		}
		else
			printf("please re");
	}

	my_sock=socket(PF_INET, SOCK_STREAM, 0);
	memset(&my_add, 0, sizeof(my_add));
	my_add.sin_addr.s_addr = inet_addr(ip);
	my_add.sin_family = AF_INET;
	my_add.sin_port = htons(atoi(tcpport));

	server_sock= socket(PF_INET, SOCK_STREAM, 0);
	memset(&server_add, 0, sizeof(server_add));
	server_add.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_add.sin_family = AF_INET;
	server_add.sin_port = htons(8080);

	if(bind(my_sock, (struct sockaddr*)&my_add, sizeof(my_add)) == -1)
		printf("bind error\n");

	if(listen(my_sock, 5) == -1)
		printf("listen error\n");

	if(connect(server_sock, (struct sockaddr*)&server_add, sizeof(struct sockaddr)) == -1){
		printf("Server Connect Error\n");
		exit(1);
	}

	memset(send_to_server, 0, sizeof(send_to_server));
	sprintf(send_to_server,"@LOGIN %s %s\n", userid, tcpport);
	
	write(server_sock, send_to_server, strlen(send_to_server));
	
	FD_ZERO(&mask);
	FD_SET(my_sock, &mask);
	FD_SET(0, &mask);
	FD_SET(server_sock, &mask);

	fd_max = my_sock;

	if (fd_max < server_sock){ 
		fd_max = server_sock; 
	}

	printf("\n@invite <username> : invite\n");
	printf("@table : see Session\n");
	printf("@leave : leave the chat\n");
	printf("@quit : exit program");
	printf("\n%s >>\n", userid);

	while(1)
	{
		int i=0;
		temp = mask;
		fd_num = select(fd_max+1, &temp, 0, 0, NULL);

		if(FD_ISSET(fileno(stdin), &temp)) //input from keyboard
		{
			char command[MEMMAX]={0};
			fgets(command, MEMMAX, stdin);
			fflush(stdin);

			FD_CLR(fileno(stdin), &temp);

			//tok command
			char command_temp[MEMMAX] = {0};
			strcpy(command_temp, command);
			char* p_command = strtok(command_temp, delimiter);

			//@invite <name>
			if(p_command && strcmp(p_command, "@invite")==0)//want to send @invite msg to other
			{
				char* id = strtok(NULL, delimiter);//name
				if (!strcmp(id, userid))
					printf("invite fail. you can't invite yourself\n");
				else if (server_sock != -1) //check subject <name> client is exist --> server checks this by @GET
				{
					memset(send_to_server, 0, sizeof(send_to_server));
					sprintf(send_to_server, "@GET %s\n", id);
					write(server_sock, send_to_server, strlen(send_to_server));
				}
				else
					printf("please connect to server\n");
			}			
			else if (p_command && (strcmp(p_command, "@leave") == 0 || strcmp(p_command, "@quit") == 0))
			{
				sprintf(send_to_client, "@LEAVE %s\n", userid);
				closeAllClient(&client_list, send_to_client);

				FD_ZERO(&mask);
				FD_SET(my_sock, &mask);
				FD_SET(0,&mask);
				fd_max = my_sock;

				if(server_sock != -1)
				{
					FD_SET(server_sock,&mask);

					if(fd_max < server_sock )
						fd_max = server_sock;
					
				}
				is_chat = 0;

				if (strcmp(p_command, "@quit") == 0)
				{
					write(server_sock, "@QUIT\n", sizeof("@QUIT\n"));
					close(server_sock);
					exit(0);
				}
			}
			else if (p_command && !strcmp(command, "@table\n"))
			{
				printAllNode(&client_list);
			}
			else if (p_command){ //send msg to all client

				memset(send_to_client, 0, sizeof(send_to_client));
				sprintf(send_to_client,"%s : %s", userid, command);

				sendMessagetoCli(&client_list, send_to_client);
			}
			fd_num--;
			printf("%s >>\n", userid);
		}

		if(FD_ISSET(my_sock, &temp)){//connent from new client
			you_len = sizeof(temp_add);
			temp_sock = accept(my_sock, (struct sockaddr *) &temp_add, &you_len);

			FD_SET(temp_sock, &mask);

			if (fd_max<temp_sock)
				fd_max = temp_sock;

			strcpy(cli_info.userIP, inet_ntoa(temp_add.sin_addr));
			cli_info.sock = temp_sock;

			insertData(&client_list, cli_info);

			FD_CLR(my_sock, &temp);				
			fd_num--;
		}

		//checks client msg
		for(i=0; i<=fd_max && fd_num >0; i++)
		{
			socket_fd=i;
			if(FD_ISSET(socket_fd,&temp))
			{
				char command_temp[MEMMAX]={0};
				char command[MEMMAX]={0};
				char *token = NULL;


				if(server_sock == socket_fd) //message from server
				{
					FD_CLR(server_sock, &temp);
					you_len = read(server_sock, command, MEMMAX-1);
					token = strtok(command, delimiter);
					if(token && strcmp(token, "@OK") == 0) //reply msg @inivite <name>. form : @OK <ip> <port>
					{ 	
						temp_connect_ip = strtok(NULL, delimiter); //ip
						temp_connect_port = strtok(NULL, delimiter); //port

						//try connection to client to invite
						temp_sock=socket(PF_INET, SOCK_STREAM, 0);
						FD_SET(temp_sock, &mask);
						memset(&temp_add, 0, sizeof(temp_add));
						temp_add.sin_family = AF_INET;
						temp_add.sin_addr.s_addr = inet_addr(temp_connect_ip);
						temp_add.sin_port = htons(atoi(temp_connect_port));

						if (connect(temp_sock, (struct sockaddr*)&temp_add, sizeof(temp_add)) == -1){
							printf("Server Connect Error\n");
							exit(1);
						}

						sprintf(send_to_client, "@INVITE %s %s\n", userid, tcpport);//send invitation msg to subject client. @INVITE <userid> <tcpport>
						write(temp_sock, send_to_client, strlen(send_to_client));


						wait_cli_info.sock = temp_sock;
						strcpy(wait_cli_info.userIP, temp_connect_ip);
						insertData(&wait_list, wait_cli_info);

						if (fd_max < temp_sock)
							fd_max = temp_sock;						
					}
					else if(token &&strcmp(token, "@FAIL") == 0 ){ //reply msg @inivite <name>. form : @FAIL <name>
						token = strtok(NULL, delimiter);
						printf("fail to find user : %s\n", token);
					}
					else if(token && strcmp(token, "@BAD") == 0 ){
						printf("BADREQUEST. Write Correct Request Header\n");
					}
					else if(!token || strcmp(token, "@SQUIT") == 0 ){ //when server shut down
						printf("Server shutdown\n",server_sock);
						close(server_sock);
						FD_CLR(server_sock,&mask);
						server_sock = -1;
					}
				}

				else //msg from client. not server
				{ 
					you_len = read(socket_fd, command, MEMMAX-1);
					strcpy(command_temp, command);
					token = strtok(command_temp, delimiter);

					if (token && strcmp(token, "@INVITE") == 0) //invitation request msg from other client
					{
						if (!is_chat)
						{
							token = strtok(NULL, delimiter); //userid
							printf("Invitation From Host %s, Type @yes  or @no : ", token);

							do{
								fgets(command_temp, MEMMAX, stdin);
								if (strcmp(command_temp, "@yes\n") == 0){

									setUserName(&client_list, socket_fd, token, strtok(NULL, delimiter));

									sprintf(send_to_client, "@CONOK %s %s\n", tcpport, userid); //send connection accept msg @CONOK <port> <name>
									write(socket_fd, send_to_client, strlen(send_to_client));
									is_chat = 1;
									printf("Join the Session\n");
									printf("%s >>\n", userid);
									break;
								}
								else if (strcmp(command_temp, "@no\n") == 0){
									sprintf(send_to_client, "@CONFAIL reject %s\n", userid); //send connection reject msg @CONFAIL reject <name>
									write(socket_fd, send_to_client, strlen(send_to_client));
									close(socket_fd);
									FD_CLR(socket_fd, &temp);
									FD_CLR(socket_fd, &mask);
									deleteData(&client_list, socket_fd);
									printf("%s >>\n", userid);
									break;
								}
								else{
									printf(" Write Correct Answer!(@yes or @no)\n");
								}
							}while (1);
						}
						else // this client is joining chatting room already. send reject msg @CONFAIL already <name>
						{
							sprintf(send_to_client, "@CONFAIL already %s\n", userid);
							write(socket_fd, send_to_client, strlen(send_to_client));
							close(socket_fd);
							FD_CLR(socket_fd, &temp);
							FD_CLR(socket_fd, &mask);
							deleteData(&client_list, socket_fd);
							break;
						}

					}
					else if (token && strcmp(token, "@CONOK") == 0) //reply msg for @INVITE <name> <port> form : @CONOK <port> <name>
					{
						token = strtok(NULL, delimiter);  //port
						memset(send_to_client, 0, sizeof(send_to_client));

						//send list of client who's joining chatting room  @LIST <name> <ip> <port>
						sprintf(send_to_client, "@LIST\n");
						getUserlist(&client_list, send_to_client);
						write(socket_fd, send_to_client, strlen(send_to_client));

						list* waittemp = findUserforSock(&wait_list, socket_fd);

						if (waittemp && waittemp->client_data.sock == socket_fd)
						{
							wait_cli_info.sock = socket_fd;
							strcpy(wait_cli_info.userPort, token); //port
							token = strtok(NULL, delimiter);
							strcpy(wait_cli_info.username, token); //name
							strcpy(wait_cli_info.userIP, waittemp->client_data.userIP);
							insertData(&client_list, wait_cli_info);
							deleteData(&wait_list, socket_fd);
							is_chat = 1;
							printf("Session Connected with %s\n", wait_cli_info.username);
						}
						else
							error_handling("find wait_list_sockerror");

					}
					else if(token && strcmp(token, "@CONFAIL") == 0 ) //reply msg for @INVITE <name> <port> form : @CONFAIL -option <name>
					{
						token = strtok(NULL, delimiter); //token option
						if (!strcmp(token, "already"))
							printf("Already join another chat : %s\n", strtok(NULL, delimiter));
						
						else
							printf("Rejected : %s\n", strtok(NULL, delimiter));
						
						close(socket_fd);
						FD_CLR(socket_fd, &temp);
						FD_CLR(socket_fd, &mask);

						deleteData(&wait_list, socket_fd);//delete possibility
					}

					else if(token && strcmp(token, "@LIST") == 0 ) // reply msg for @CONOK form : @LIST <name> <ip> <port>
					{ 
						token = strtok(NULL, delimiter); // name
						if (token)
						{
							strcpy(cli_info.username, token);
							token = strtok(NULL, delimiter); //ip
							strcpy(cli_info.userIP, token);
							token = strtok(NULL, delimiter); // port
							strcpy(cli_info.userPort, token);
						}
						while (token)
						{
							//connect with client who alreay joining in chatting room
							temp_sock = socket(PF_INET, SOCK_STREAM, 0);

							memset(&temp_add, 0, sizeof(temp_add));
							temp_add.sin_addr.s_addr = inet_addr(cli_info.userIP);
							temp_add.sin_family = AF_INET;
							temp_add.sin_port = htons(atoi(cli_info.userPort));

							FD_SET(temp_sock,&mask);

							if (fd_max < temp_sock)
								fd_max = temp_sock;

							if (connect(temp_sock, (struct sockaddr*)&temp_add, sizeof(struct sockaddr)) == -1){
								printf("Client Connect Error : 5289 \n");
								exit(1);
							}

							cli_info.sock = temp_sock;

							insertData(&client_list, cli_info);

							//send name and port. @SEND <name> <port>
							sprintf(send_to_client, "@SEND %s %s\n", userid, tcpport);
							write(temp_sock, send_to_client, strlen(send_to_client));

							token = strtok(NULL, delimiter); // name
							if (token)
							{
								strcpy(cli_info.username, token);
								token = strtok(NULL, delimiter); // ip
								strcpy(cli_info.userIP, token);
								token = strtok(NULL, delimiter); // port
								strcpy(cli_info.userPort, token);
							}
						}
					}
					else if(token && strcmp(token, "@SEND") == 0 ) // get new client who is joining chatting room. form : @SEND <name> <port>
					{
						token = strtok(NULL, delimiter); // name
						setUserName(&client_list, socket_fd, token, strtok(NULL, delimiter));
					}
					else if(token && strcmp(token, "@LEAVE") == 0 )
					{
						printf("%s leaves the chating room\n", strtok(NULL, delimiter));
						close(socket_fd);
						FD_CLR(socket_fd, &temp);
						FD_CLR(socket_fd, &mask);
						deleteData(&client_list, socket_fd);

						if (!client_list.next)
						{
							printf("All users leave the chat room. The chat room is broken\n");
							is_chat = 0;
						}
					}
					else if(token)
					{ 
						printf("%s\n",command);
					}
					else
					{
						deleteData(&client_list, socket_fd);
						close(socket_fd);
						printf("Connection Closed socket %d\n", socket_fd);	
						FD_CLR(socket_fd, &mask);
						socket_fd = -1;
					}
				}
				if (--fd_num<= 0)
					break;
			}
		}
	}
	close(my_sock);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void insertData(list* mylist, ClientInfo newClient) //insert new client info
{
	list* node = mylist;
	list* newNode=NULL;
	list* prenode = NULL;
	int i = 0;
	while (node && node->client_data.sock < newClient.sock)  //find end of list
	{
		prenode = node;
		node = node->next;
	}
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
		free(node);
		return ret_sock;
	}	
	else if (!node || (node == mylist))
	{
		printf("(deleteData)not exist socket %d\n", sock);
		return -1;
	}
	else
		return -1;
}


void setUserPort(list* mylist, int sock, char port[])
{
	//printf("\nsetusername\n");
	list* node = mylist;
	list* prenode = NULL;
	while (node && node->client_data.sock != sock)
		node = node->next;

	if (node)
		strcpy(node->client_data.userPort, port);
	else
		printf("(setUserPort)fail to find sock# %d\n", sock);
}

list* findUserforSock(list* mylist, int sock)
{
	list* node = mylist;
	while (node && (node->client_data.sock != sock))
		node = node->next;
	return node;
}


void getUserlist(list* mylist, char msghead[]) 
{
	list* node = mylist->next;
	char tempbuf[MEMMAX] ={0};
	while(node)
	{
		memset(tempbuf, 0, sizeof(tempbuf));
		sprintf(tempbuf,"%s %s %s\n", node->client_data.username, node->client_data.userIP, node->client_data.userPort);
		strcat(msghead, tempbuf);
		node = node->next;
	} 
}

void sendMessagetoCli(list* mylist, char msg[])
{
	list* node = mylist->next;
	while(node)
	{
		write(node->client_data.sock, msg, strlen(msg));
		node = node->next;
	}
}


void closeAllClient(list* client_list, char to_clientbuf[])
{
	list* node = client_list->next;

	while (node)
	{
		write(node->client_data.sock, to_clientbuf, strlen(to_clientbuf));
		close(node->client_data.sock);
		deleteData(client_list, node->client_data.sock);
		node = node->next;
	}
}

void printAllNode(list* mylist)
{
	list* node = mylist->next;
	int i = 0;
	printf("ID  NAME         IP      PORT SOCK\n");
	printf("----------------------------------\n");
	while (node)
	{
		printf("%d   %s    %s    %s  %d\n", i++, node->client_data.username, node->client_data.userIP, node->client_data.userPort, node->client_data.sock);
		node = node->next;
	}
	printf("----------------------------------\n");
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

void display()
{
	printf("20123434 hwang jung dam \n");
	printf("20123347 kim ji seon \n");
}
