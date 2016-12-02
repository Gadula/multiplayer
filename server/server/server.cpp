#include <stdio.h>
#include <string>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE     1000
#define PORT_NUMBER 7890

int get_socket_number(char Message[], int MessageLength);

int main(int argc, char **argv)
{
	int          Port = PORT_NUMBER;
	WSADATA      WsaData;
	SOCKET       ServerSocket;
	SOCKADDR_IN  ServerAddr;

	unsigned int Index;
	int          ClientLen = sizeof(SOCKADDR_IN);
	SOCKET       ClientSocket;
	SOCKADDR_IN  ClientAddr;

	fd_set       ReadFds, TempFds;
	TIMEVAL      Timeout; // struct timeval timeout;

	char         Message[BUFSIZE];
	int          Return;

	char         MessageToSend[BUFSIZE];
	int          MessageToSendLength;

	SOCKET       ToSocket;

	if (2 == argc)
	{
		Port = atoi(argv[1]);
	}
	printf("Using port number : [%d]\n", Port);

	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
	{
		printf("WSAStartup() error!\n");
		return 1;
	}

	ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == ServerSocket)
	{
		printf("socket() error\n");
		return 1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr)))
	{
		printf("bind() error\n");
		return 1;
	}

	if (SOCKET_ERROR == listen(ServerSocket, 5))
	{
		printf("listen() error\n");
		return 1;
	}

	FD_ZERO(&ReadFds);
	FD_SET(ServerSocket, &ReadFds);

	while (true)
	{
		TempFds = ReadFds;
		Timeout.tv_sec = 5;
		Timeout.tv_usec = 0;

		if (SOCKET_ERROR == (Return = select(0, &TempFds, 0, 0, &Timeout)))
		{ // Select() function returned error.
			printf("select() error\n");
			return 1;
		}
		if (0 == Return)
		{ // Select() function returned by timeout.
			printf("Select returned timeout.\n");
		}
		else if (0 > Return)
		{
			printf("Select returned error!\n");
		}
		else
		{
			for (Index = 0; Index < TempFds.fd_count; Index++)
			{
				if (TempFds.fd_array[Index] == ServerSocket)
				{ // New connection requested by new client.
					SOCKET tempClient;

					tempClient = accept(ServerSocket, (SOCKADDR *)&ClientAddr, &ClientLen);
					ClientSocket = tempClient;
					sprintf_s(MessageToSend, "<Welcome! Your Session ID is %d>\n", tempClient);
					MessageToSendLength = strlen(MessageToSend);
					send(tempClient, MessageToSend, MessageToSendLength, 0);
					TempFds.fd_count++;
					for (int i = 1; i < ReadFds.fd_count; i++)
					{
						if (TempFds.fd_array[Index] == tempClient)
						{
							sprintf_s(MessageToSend, "<Already connected client with session ID %s>\n", ReadFds.fd_array[i]);
							MessageToSendLength = strlen(MessageToSend);
							send(tempClient, MessageToSend, MessageToSendLength, 0);
						}
					}

					for (int i = 1; i < ReadFds.fd_count; i++)
					{
						sprintf_s(MessageToSend, "<New client has connected.Session ID is %d>\n", tempClient);
						MessageToSendLength = strlen(MessageToSend);
						send(ReadFds.fd_array[i], MessageToSend, MessageToSendLength, 0);
					}

					FD_SET(tempClient, &ReadFds);

					printf("New Client Accepted : Socket Handle [%d]\n", tempClient);
				}
				else
				{ // Something to read from socket.
					Return = recv(TempFds.fd_array[Index], Message, BUFSIZE, 0);
					if (0 == Return)
					{ // Connection closed message has arrived.
						closesocket(TempFds.fd_array[Index]);
						printf("Connection closed :Socket Handle [%d]\n", TempFds.fd_array[Index]);
						FD_CLR(TempFds.fd_array[Index], &ReadFds);
					}
					else if (0 > Return)
					{ // recv() function returned error.
						closesocket(TempFds.fd_array[Index]);
						printf("Exceptional error :Socket Handle [%d]\n", TempFds.fd_array[Index]);
						FD_CLR(TempFds.fd_array[Index], &ReadFds);
					}
					else
					{
						std::string id;
						// Message recevied.
						if ('/' == Message[0] && 'w' == Message[1] && ' ' == Message[2])
						{
							for (int i = 3; i < BUFSIZE; i++)
							{
								if (Message[i] != ' ')
								{
									id += Message[i];
								}
								if (Message[i] == ' ')
								{
									break;
								}
							}

							int tempID;
							tempID = stoi(id);

							for (Index = 0; Index <= TempFds.fd_count; Index++)
							{
								if (TempFds.fd_array[Index] == tempID)
								{
									MessageToSendLength = strlen(Message);
									send(TempFds.fd_array[Index], Message, MessageToSendLength + 7, 0);
									/*ToSocket = get_socket_number(Message, Return);

									if (0 < ToSocket)
									{
									sprintf_s(MessageToSend, "Sender %d sent : %s>\n", TempFds.fd_array[Index], Message);
									MessageToSendLength = strlen(MessageToSend);
									if (0 >= send(ToSocket, MessageToSend, MessageToSendLength, 0))
									{
									sprintf_s(MessageToSend, "<Fail to send to %d>\n", ToSocket);
									MessageToSendLength = strlen(MessageToSend);
									}
									else
									{
									sprintf_s(MessageToSend, "<Message sent to %d>\n", ToSocket);
									MessageToSendLength = strlen(MessageToSend);
									}
									send(TempFds.fd_array[Index], MessageToSend, MessageToSendLength, 0);
									}*/
								}
							}
						}
						else
						{
							for (int i = 1; i < ReadFds.fd_count; i++)
							{
								send(ReadFds.fd_array[i], Message, Return, 0);
							}
						}
					}
				}
			}
		}
	}

	WSACleanup();

	return 0;
}

int get_socket_number(char Message[], int MessageLength)
{
	char TempBuffer[BUFSIZE];
	int i;
	int j;

	for (i = 1, j = 0; i < MessageLength; i++)
	{
		if ((' ' == Message[i]) || ('\n' == Message[i]) || ('\r' == Message[i]) || ('\0' == Message[i]))
		{
			TempBuffer[j] = '\0';
			break;
		}
		TempBuffer[j++] = Message[i];
	}

	return atoi(TempBuffer);
}