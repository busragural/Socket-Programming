#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define MAX_USERS 100
#define ALERT_MSSG 250

typedef struct Message
{
	char senderid[50], receiverid[50];
	char content[250];
} Message;

typedef struct User
{
	char userid[50];
	char name[50], surname[50], phone[50];

} User;

void sendUserId(int client_socket, char *userid);
void sendChoice(int client_socket, int choice);
void recieveAlertMessage(int client_socket, char *response_message);

void login(int client_socket, char *userid);
void addFriend(int client_socket, char *userid);
void listContacts(int client_socket, char *userid);
void deleteFriend(int client_socket, char *userid);
void sendMessage(int client_socket, char *userid);
void checkMessages(int client_socket, char *userid);
void displayMessageHistory(int client_socket, char *userid);

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <userid>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int client_socket;
	struct sockaddr_in server_address;

	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}


	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);

	// localhost -> 127.0.0.1
	if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
	{
		perror("Error setting server address");
		exit(EXIT_FAILURE);
	}


	if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
	{
		perror("Error connecting to server");
		printf("Connect error: %d\n", errno); 
		exit(EXIT_FAILURE);
	}

	login(client_socket, argv[1]);
	int choice;
	do
	{
		printf("\nMenu:\n");
		printf("1.List Contacts\n2.Add Friend\n3.Delete Friend\n4.Send Message\n5.Check Message\n6.Display Message History\n");

		printf("Enter your choice: ");
		scanf("%d", &choice);

		switch (choice)
		{
		case 1:
			listContacts(client_socket, argv[1]);
			break;
		case 2:
			addFriend(client_socket, argv[1]);
			break;
		case 3:
			deleteFriend(client_socket, argv[1]);
			break;
		case 4:
			sendMessage(client_socket, argv[1]);
			break;
		case 5:
			checkMessages(client_socket, argv[1]);
			break;
		case 6:
			displayMessageHistory(client_socket, argv[1]);
			break;
		case 0:
			printf("Exiting...\n");
			break;

		default:
			break;
		}

	} while (choice != 0);
}

void sendChoice(int client_socket, int choice)
{
	if (send(client_socket, &choice, sizeof(int), 0) == -1)
	{
		perror("Error sending choice");
		exit(EXIT_FAILURE);
	}
}

void sendUserId(int client_socket, char *userid)
{
	if (send(client_socket, userid, strlen(userid) + 1, 0) == -1)
	{
		perror("Error sending userid");
		exit(EXIT_FAILURE);
	}
}

void recieveAlertMessage(int client_socket, char *response_message)
{

	if (recv(client_socket, response_message, ALERT_MSSG * sizeof(char), 0) == -1)
	{
		perror("Error receiving server response");
		exit(EXIT_FAILURE);
	}
	printf("Message:\n%s\n", response_message);
}

void login(int client_socket, char *userid)
{
	int choice = 1;
	int found;
	User *user = (User *)malloc(sizeof(User));

	sendChoice(client_socket, choice);
	sendUserId(client_socket, userid);

	if (recv(client_socket, &found, sizeof(int), 0) == -1)
	{
		perror("Error receiving choice");
		exit(EXIT_FAILURE);
	}

	if (!found)
	{
		printf("Enter your name: ");
		fgets(user->name, sizeof(user->name), stdin);
		user->name[strcspn(user->name, "\n")] = '\0'; // Remove 'enter'

		printf("Enter your surname: ");
		fgets(user->surname, sizeof(user->surname), stdin);
		user->surname[strcspn(user->surname, "\n")] = '\0';

		printf("Enter your phone number: ");
		fgets(user->phone, sizeof(user->phone), stdin);
		user->phone[strcspn(user->phone, "\n")] = '\0';

		if (send(client_socket, user, sizeof(User), 0) == -1)
		{
			perror("Error sending information");
			exit(EXIT_FAILURE);
		}
	}
	char message[ALERT_MSSG];
	recieveAlertMessage(client_socket, message);
}

void listContacts(int client_socket, char *userid)
{
	int choice = 3;
	sendChoice(client_socket, choice);
	sendUserId(client_socket, userid);
	char response_message[ALERT_MSSG];
	recieveAlertMessage(client_socket, response_message);
}

void addFriend(int client_socket, char *userid)
{
	char friendid[50];
	int choice = 2;

	sendChoice(client_socket, choice);
	sendUserId(client_socket, userid);

	printf("Enter friend's ID: ");
	scanf("%s", friendid);

	sendUserId(client_socket, friendid);

	char response_message[ALERT_MSSG];
	recieveAlertMessage(client_socket, response_message);
}

void deleteFriend(int client_socket, char *userid)
{
	char friendid[50];
	int choice = 4;

	sendChoice(client_socket, choice);
	sendUserId(client_socket, userid);

	printf("Enter friend's ID: ");
	scanf("%s", friendid);

	sendUserId(client_socket, friendid);

	char response_message[ALERT_MSSG];
	recieveAlertMessage(client_socket, response_message);
}

void sendMessage(int client_socket, char *userid)
{

	int choice = 5;
	Message *message = (Message *)malloc(sizeof(Message));

	sendChoice(client_socket, choice);

	printf("Who will you send a message to? Enter: ");
	scanf("%s", message->receiverid);

	printf("Enter the message: ");
	scanf(" %[^\n]", message->content);

	strcpy(message->senderid, userid);
	if (send(client_socket, message, sizeof(Message), 0) == -1)
	{
		perror("Error sending message");
		exit(EXIT_FAILURE);
	}

	char response_message[ALERT_MSSG];
	recieveAlertMessage(client_socket, response_message);
}

void checkMessages(int client_socket, char *userid)
{
	int choice = 6, i;
	char messages[ALERT_MSSG];
	char ids[200][200];
	char idBuffer[200 * 200];

	sendChoice(client_socket, choice);
	sendUserId(client_socket, userid);

	recieveAlertMessage(client_socket, messages);

	if (recv(client_socket, idBuffer, sizeof(idBuffer), 0) == -1)
	{
		perror("Error receiving messages buffer");
		exit(EXIT_FAILURE);
	}

	size_t receivedCount = sizeof(idBuffer) / sizeof(ids[0]);

	memcpy(ids, idBuffer, sizeof(ids));

	for (i = 0; i < receivedCount; i++)
	{
		if (ids[i][0] != '\0')
		{
			printf("ID: %s\n", ids[i]);
		}
	}

	char pickOne[50];
	scanf("%s", pickOne);
	sendUserId(client_socket, pickOne);
	recieveAlertMessage(client_socket, messages);
}

void displayMessageHistory(int client_socket, char *userid)
{
	int choice = 7;
	char friendid[50];

	sendChoice(client_socket, choice);
	sendUserId(client_socket, userid);

	printf("Enter an userid to see message history: ");
	scanf("%s", friendid);

	sendUserId(client_socket, friendid);

	char message_history[1024];
	if (recv(client_socket, message_history, sizeof(message_history), 0) == -1)
	{
		perror("Error receiving message history");
		exit(EXIT_FAILURE);
	}

	printf("Message History:\n%s\n", message_history);
}
