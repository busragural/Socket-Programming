#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <pthread.h>

#define PORT 8080

#define MAX_USERS 100
#define MAX_LENGTH 50
#define ALERT_MSSG 250
#define MAX_PATH 150
#define USERS_DIRECTORY "./SocketApp/Users"
#define ALL_USERS_FILE "./SocketApp/allUsers.csv"

typedef struct User
{
    char userid[50];
    char name[50], surname[50], phone[50];
} User;

typedef struct Message
{
    char senderid[50], receiverid[50];
    char content[250];
} Message;

void *handle_client(void *arg);
void saveUserToCSV(User *user);
void handleLogin(int client_socket);
void handleAddFriend(int client_socket);
void handleListContacts(int client_socket);
void handleDeleteFriend(int client_socket);
void handleSendMessage(int client_socket);
void handleCheckMessage(int client_socket);
void sendAlertMessages(int client_socket, char *message);
void deleteMessageFromBuffer(char *userId, char *selectedId);
void handleDisplayMessageHistory(int client_socket);
int searchUser(char *userid);
void createAppDirectory();

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Socket file descriptor
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }


    if (listen(server_socket, 5) == -1)
    {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1)
    {

        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len)) == -1)
        {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0)
        {
            perror("Error creating thread");
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}

void *handle_client(void *arg)
{
    int client_socket = *((int *)arg);
    int choice;
    do
    {
        if (recv(client_socket, &choice, sizeof(int), 0) == -1)
        {
            perror("Error receiving choice");
            exit(EXIT_FAILURE);
        }
        printf("Received choice from client: %d\n", choice);
        switch (choice)
        {
        case 1:
            handleLogin(client_socket);
            break;
        case 2:
            handleAddFriend(client_socket);
            break;
        case 3:
            handleListContacts(client_socket);
            break;
        case 4:
            handleDeleteFriend(client_socket);
            break;
        case 5:
            handleSendMessage(client_socket);
            break;
        case 6:
            handleCheckMessage(client_socket);
            break;
        case 7:
            handleDisplayMessageHistory(client_socket);
            break;
        case 0:
            printf("Client requested exit.\n");
            break;
        default:
            fprintf(stderr, "Invalid choice.\n");
            break;
        }
    } while (choice != 0);

    close(client_socket);
    pthread_exit(NULL);
}

void createAppDirectory() {
    struct stat st = {0};
    if (stat("SocketApp", &st) == -1) {
        if (mkdir("SocketApp", 0777) == -1) {
            perror("Error creating SocketApp directory");
            exit(EXIT_FAILURE);
        } else {
            printf("SocketApp directory created.\n");
        }
    }

    char usersPath[100];
    snprintf(usersPath, sizeof(usersPath), "SocketApp/Users");

    if (stat(usersPath, &st) == -1) {
        if (mkdir(usersPath, 0777) == -1) {
            perror("Error creating users directory");
            exit(EXIT_FAILURE);
        } else {
            printf("users directory created inside SocketApp.\n");
        }
    }
}

void recieveUserId(int client_socket, char *userid)
{
    if (recv(client_socket, userid, MAX_LENGTH * sizeof(char), 0) == -1)
    {
        perror("Error receiving user ID");
        return;
    }
}

void sendAlertMessages(int client_socket, char *message)
{
    if (send(client_socket, message, ALERT_MSSG * sizeof(char), 0) == -1)
    {
        perror("Error sending warning message.");
    }
}

void handleListContacts(int client_socket)
{
    char userid[MAX_LENGTH];
    recieveUserId(client_socket, userid);

    char friendsFilePath[MAX_PATH];
    snprintf(friendsFilePath, sizeof(friendsFilePath), "%s/%s/friends.csv", USERS_DIRECTORY, userid);

    FILE *friendsFile = fopen(friendsFilePath, "r"); // Read friends.csv
    if (friendsFile == NULL)
    {
        perror("Error opening friends.csv file");
        return;
    }

    char contactsBuffer[1024];
    // 1 byte is read at a time and the read data is recorded.
    size_t bytesRead = fread(contactsBuffer, 1, sizeof(contactsBuffer), friendsFile);
    fclose(friendsFile);

    if (send(client_socket, contactsBuffer, bytesRead, 0) == -1)
    {
        perror("Error sending contacts");
        return;
    }
}

void saveUserToCSV(User *user)
{
    char csvFilePath[MAX_PATH];
    snprintf(csvFilePath, sizeof(csvFilePath), "./SocketApp/allUsers.csv");

    FILE *csvFile = fopen(csvFilePath, "a"); // append mode
    if (csvFile == NULL)
    {
        perror("Error opening CSV file");
        return;
    }
    fprintf(csvFile, "%s,%s,%s,%s\n", user->userid, user->name, user->surname, user->phone);

    fclose(csvFile);
}

int searchUser(char *userid)
{
    int found = 0;
    int quit = 1;
    char *tmpId = (char *)malloc(MAX_LENGTH * sizeof(char));
    char *line = (char *)malloc(256 * sizeof(char));

    FILE *allUsersFile = fopen(ALL_USERS_FILE, "r");
    if (allUsersFile == NULL)
    {
        perror("Error opening allUsers.csv");
        return;
    }

    while (fgets(line, sizeof(line), allUsersFile) != NULL && quit)
    {
        sscanf(line, "%49[^,]", tmpId); // 49 characters and comma
        if (strcmp(tmpId, userid) == 0)
        {
            found = 1;
            quit = 0;
        }
    }
    if (found)
    {
        return 1;
    }
    return 0;
}

void handleLogin(int client_socket)
{
    int found = 0;
    int quit = 1;
    char *line = (char *)malloc(256 * sizeof(char));
    char *tmpId = (char *)malloc(MAX_LENGTH * sizeof(char));
    User *user = (User *)malloc(sizeof(User));

    char userid[MAX_LENGTH];
    recieveUserId(client_socket, userid);

    found = searchUser(userid); // found -> Checks whether the user has already registered

    
    if (send(client_socket, &found, sizeof(int), 0) == -1)
    {
        perror("Error sending choice");
        exit(EXIT_FAILURE);
    }

    if (!found) // the user has not registered yet
    {
        createAppDirectory();
        char userFolder[MAX_PATH];
        snprintf(userFolder, sizeof(userFolder), "%s/%s", USERS_DIRECTORY, userid);
        if (mkdir(userFolder, 0777) == -1)
        {
            perror("Error creating user's folder");
        }
        else
        {
            printf("User's folder created: %s\n", userFolder);
        }

        char infoFilePath[300];
        snprintf(infoFilePath, sizeof(infoFilePath), "%s/info.csv", userFolder);

        if (access(infoFilePath, F_OK) == -1)  //If the file does not exist 
        {
            if (recv(client_socket, user, sizeof(User), 0) == -1)
            {
                perror("Error receiving additional information");
                return;
            }

            FILE *infoFile = fopen(infoFilePath, "w");
            if (infoFile == NULL)
            {
                perror("Error opening info file");
                return;
            }

            strcpy(user->userid, userid);
            fprintf(infoFile, "%s\n%s\n%s\n%s\n", userid, user->name, user->surname, user->phone);
            fclose(infoFile);

            printf("User information saved to %s\n", infoFilePath);
            saveUserToCSV(user);
        }

        char message[] = "User login successful.";
        sendAlertMessages(client_socket, message);
    }
    else
    {
        char message[] = "Logged in.";
        sendAlertMessages(client_socket, message);
        return;
    }
}

void handleAddFriend(int client_socket)
{
    User *friend = (User *)malloc(sizeof(User));
    char userid[MAX_LENGTH];
    recieveUserId(client_socket, userid);

    char friendId[MAX_LENGTH];
    recieveUserId(client_socket, friendId);

    int friendFound = 0;
    int quit = 1;
    // char *line = (char *)malloc(256 * sizeof(char));
    char line[256];

    FILE *allUsersFile = fopen(ALL_USERS_FILE, "r");
    if (allUsersFile == NULL)
    {
        perror("Error opening allUsers.csv");
        return;
    }

    while (fgets(line, sizeof(line), allUsersFile) != NULL && quit)
    {
        sscanf(line, "%49[^,],%49[^,],%49[^,],%19[^\n]", friend->userid, friend->name, friend->surname, friend->phone);
        if (strcmp(friend->userid, friendId) == 0)
        {
            friendFound = 1;
            quit = 0;
        }
    }

    fclose(allUsersFile);

    if (!friendFound) // Friend not found
    {
        char message[] = "Friend not found. Please check the friend's ID.";
        sendAlertMessages(client_socket, message);
        return;
    }

    // Friend is found, add friendId to the user's friends.csv
    char userFolder[MAX_PATH];
    snprintf(userFolder, sizeof(userFolder), "%s/%s", USERS_DIRECTORY, userid);

    char friendsFilePath[MAX_PATH];
    snprintf(friendsFilePath, sizeof(friendsFilePath), "%s/friends.csv", userFolder);

    FILE *friendsFile = fopen(friendsFilePath, "a");
    if (friendsFile == NULL)
    {
        perror("Error opening friends.csv file");
        return;
    }

    fprintf(friendsFile, "%s,%s,%s,%s\n", friend->userid, friend->name, friend->surname, friend->phone);
    fclose(friendsFile);

    char message[] = "Friend added successfully.";
    sendAlertMessages(client_socket, message);
}

void handleDeleteFriend(int client_socket)
{

    char userid[MAX_LENGTH];
    char friendId[MAX_LENGTH];
    char *line = (char *)malloc(256 * sizeof(char));
    char *tmpId = (char *)malloc(50 * sizeof(char));
    int deleted = 0;

    recieveUserId(client_socket, userid);
    recieveUserId(client_socket, friendId);

    char userFolder[MAX_PATH];
    snprintf(userFolder, sizeof(userFolder), "%s/%s", USERS_DIRECTORY, userid);

    char friendsFilePath[MAX_PATH];
    snprintf(friendsFilePath, sizeof(friendsFilePath), "%s/friends.csv", userFolder);

    FILE *friendsFile = fopen(friendsFilePath, "r");
    if (friendsFile == NULL)
    {
        perror("Error opening friends.csv file");
        return;
    }

    char tmpFilePath[150]; // temporary file for writing
    snprintf(tmpFilePath, sizeof(tmpFilePath), "%s/friends_temp.csv", userFolder);
    
    FILE *tmpFile = fopen(tmpFilePath, "w");
    if (tmpFile == NULL)
    {
        perror("Error creating temporary file");
        fclose(friendsFile);
        return;
    }

    while (fgets(line, sizeof(line), friendsFile) != NULL)
    {
        sscanf(line, "%49[^,]", tmpId);
        if (strcmp(tmpId, friendId) == 0)
        {
            deleted = 1;
        }
        else
        {
            fprintf(tmpFile, "%s", line); // Writes lines (excluding the one to be deleted) to the temporary file
        }
    }

    fclose(friendsFile);
    fclose(tmpFile);

    if (rename(tmpFilePath, friendsFilePath) == -1)
    {
        perror("Error updating friends.csv file");
        return;
    }

    if (deleted)
    {
        char successMessage[] = "Friend deleted successfully.";
        sendAlertMessages(client_socket, successMessage);
    }
    else
    {
        char failureMessage[] = "Friend not found. Unable to delete.";
        sendAlertMessages(client_socket, failureMessage);
    }
}

void handleSendMessage(int client_socket)
{
    Message *message = (Message *)malloc(sizeof(Message));
    char *line = (char *)malloc(256 * sizeof(char));
    char *tmpId = (char *)malloc(50 * sizeof(char));
    int quit = 1;
    int found = 0;

    if (recv(client_socket, message, sizeof(struct Message), 0) == -1)
    {
        perror("Error receiving message");
        free(message);
        return;
    }

    char userFolder[MAX_PATH];
    snprintf(userFolder, sizeof(userFolder), "%s/%s", USERS_DIRECTORY, message->senderid);

    char friendsFilePath[MAX_PATH];
    snprintf(friendsFilePath, sizeof(friendsFilePath), "%s/friends.csv", userFolder);

    FILE *friendsFile = fopen(friendsFilePath, "r");
    if (friendsFile == NULL)
    {
        perror("Error opening friends.csv file");
        return;
    }

    while (fgets(line, sizeof(line), friendsFile) != NULL && quit) // Checks if the sender and receiver are friends
    {
        sscanf(line, "%49[^,]", tmpId);
        printf("nemiss: %s", tmpId);
        if (strcmp(tmpId, message->receiverid) == 0)
        {
            found = 1;
            quit = 0;
        }
    }
    if (found) // If they are friends,  send the message
    {
        char userFolder[100];
        snprintf(userFolder, sizeof(userFolder), "%s/%s", USERS_DIRECTORY, message->senderid);
        mkdir(userFolder, 0777);

        char senderFilePath[250];
        snprintf(senderFilePath, sizeof(senderFilePath), "%s/%s-%s.csv", userFolder, message->senderid, message->receiverid);

        FILE *senderFile = fopen(senderFilePath, "a");
        if (senderFile == NULL)
        {
            perror("Error opening sender-receiver CSV file");
            free(message);
            return;
        }

        fprintf(senderFile, "0, %s\n", message->content);

        fclose(senderFile);

        char bufferFilePath[150];
        snprintf(bufferFilePath, sizeof(bufferFilePath), "%s/%s/buffer.csv", USERS_DIRECTORY, message->receiverid);

        FILE *bufferFile = fopen(bufferFilePath, "a");
        if (bufferFile == NULL)
        {
            perror("Error opening receiver buffer file");
            free(message);
            return;
        }

        fprintf(bufferFile, "%s, %s\n", message->senderid, message->content);

        fclose(bufferFile);

        char successMessage[] = "Message sent successfully.";
        sendAlertMessages(client_socket, successMessage);

        free(message);
    }
    else
    {
        char failMessage[] = "You are not friends.";
        sendAlertMessages(client_socket, failMessage);
    }
}

void handleCheckMessage(int client_socket)
{
	 int i;
    char userid[MAX_LENGTH];
    recieveUserId(client_socket, userid);

    char bufferFilePath[MAX_PATH];
    snprintf(bufferFilePath, sizeof(bufferFilePath), "%s/%s/buffer.csv", USERS_DIRECTORY, userid);

    FILE *bufferFile = fopen(bufferFilePath, "r");
    if (bufferFile == NULL)
    {
        perror("Error opening buffer.csv file");
        return;
    }

    char messages_buffer[1024];
    size_t bytesRead = fread(messages_buffer, 1, sizeof(messages_buffer), bufferFile); //Reads the contents of the buffer file into a buffer
    fclose(bufferFile);

    char ids[200][200]; //stores id
    char messages[200][200]; //stores messages

    int idIndex = 0;
    int messageIndex = 0;

    char *token = strtok(messages_buffer, "\n");

    while (token != NULL) // extract IDs and messages
    {

        char *comma_position = strchr(token, ',');
        if (comma_position != NULL)
        {

            *comma_position = '\0';
            strncpy(ids[idIndex], token, 200);
            ids[idIndex][200 - 1] = '\0';

            strncpy(messages[messageIndex], comma_position + 1, 200);
            messages[messageIndex][200 - 1] = '\0';

            idIndex++;
            messageIndex++;
        }

        token = strtok(NULL, "\n");
    }
   
    for ( i = 0; i < idIndex; i++)
    {
        printf("ID: %s, Message: %s\n", ids[i], messages[i]);
    }
    char info_message[] = "You have messages from these people. Which one do you want to read? Enter: \n";
    sendAlertMessages(client_socket, info_message);
    char linearBuffer[200 * 200];
    memcpy(linearBuffer, ids, sizeof(ids)); // Copy IDs into a linear buffer and send

    if (send(client_socket, linearBuffer, sizeof(linearBuffer), 0) == -1)
    {
        perror("Error sending messages buffer");
        exit(EXIT_FAILURE);
    }

    char pickOne[50];
    recieveUserId(client_socket, pickOne);

    int pickedIndex = -1;
    int flag = 1;
    i=0;
    while (i < idIndex && flag)  // Search for the selected ID in the array
    {
        if (strcmp(ids[i], pickOne) == 0)
        {
            pickedIndex = i;
            flag = 0;
        }
        i++;
    }

    if (pickedIndex != -1)
    {

        char pickedMessage[200];
        snprintf(pickedMessage, sizeof(pickedMessage), "Picked Message: %s\n", messages[pickedIndex]);

        if (send(client_socket, pickedMessage, sizeof(pickedMessage), 0) == -1)
        {
            perror("Error sending picked message");
            exit(EXIT_FAILURE);
        }

        deleteMessageFromBuffer(userid, pickOne);
    }
    else
    {
        char errorMessage[] = "Error: Invalid ID selected\n";
        if (send(client_socket, errorMessage, sizeof(errorMessage), 0) == -1)
        {
            perror("Error sending error message");
            exit(EXIT_FAILURE);
        }
    }
}

void deleteMessageFromBuffer(char *userId, char *selectedId)
{

    char bufferFilePath[150];
    snprintf(bufferFilePath, sizeof(bufferFilePath), "%s/%s/buffer.csv", USERS_DIRECTORY, userId);

    FILE *bufferFile = fopen(bufferFilePath, "r+");
    if (bufferFile == NULL)
    {
        perror("Error opening buffer.csv file");
        return;
    }

    char buffer[1024];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), bufferFile);

    fseek(bufferFile, 0, SEEK_SET); // set the file position to the beginning

    char modifiedBuffer[1024];
    size_t modifiedSize = 0;

    char ids[200][200];
    char messages[200][200];

    int idIndex = 0;
    char *token = strtok(buffer, "\n");

    int selectedMessageFound = 0;
    char foundMessage[200] = "";
    while (token != NULL)
    {

        char *commaPosition = strchr(token, ',');
        if (commaPosition != NULL)
        {

            *commaPosition = '\0';
            strncpy(ids[idIndex], token, sizeof(ids[idIndex]));
            ids[idIndex][sizeof(ids[idIndex]) - 1] = '\0';

            strncpy(messages[idIndex], commaPosition + 1, sizeof(messages[idIndex]));
            messages[idIndex][sizeof(messages[idIndex]) - 1] = '\0';

            if (strcmp(ids[idIndex], selectedId) != 0) // Checks if the current ID is not the one to be deleted
            {
                printf("test: %s\n", messages[idIndex]);

                strcat(modifiedBuffer, token);             //  ID
                strcat(modifiedBuffer, ",");               // comma separator
                strcat(modifiedBuffer, messages[idIndex]); // message
                strcat(modifiedBuffer, "\n");

                modifiedSize += strlen(token) + strlen(messages[idIndex]) + 2; // comma and newline
            }
            else
            {
                selectedMessageFound = 1;
                strncat(foundMessage, messages[idIndex], sizeof(foundMessage) - strlen(foundMessage) - 1);
                foundMessage[sizeof(foundMessage) - 1] = '\0';
            }

            idIndex++;
        }

        token = strtok(NULL, "\n");
    }

    ftruncate(fileno(bufferFile), 0);
    fwrite(modifiedBuffer, 1, modifiedSize, bufferFile); // Writes the modified buffer back to the buffer file
    fclose(bufferFile);

    if (selectedMessageFound)
    {
        char receiverFolder[150];
        snprintf(receiverFolder, sizeof(receiverFolder), "%s/%s", USERS_DIRECTORY, userId);

        char receiverFilePath[150];
        snprintf(receiverFilePath, sizeof(receiverFilePath), "%s/%s-%s.csv", receiverFolder, userId, selectedId);

        FILE *receiverFile = fopen(receiverFilePath, "a");
        if (receiverFile == NULL)
        {
            perror("Error opening receiver CSV file");
            return;
        }

        fprintf(receiverFile, "1,%s\n", foundMessage);
        fclose(receiverFile);
    }
}

void handleDisplayMessageHistory(int client_socket)
{
    char userid[MAX_LENGTH];
    recieveUserId(client_socket, userid);

    char friendId[MAX_LENGTH];
    recieveUserId(client_socket, friendId);

    char history_file_path[MAX_PATH];
    snprintf(history_file_path, sizeof(history_file_path), "%s/%s/%s-%s.csv", USERS_DIRECTORY, userid, userid, friendId);

    FILE *history_file = fopen(history_file_path, "r");
    if (history_file == NULL)
    {
        perror("Error opening message history file");
        return;
    }

    char historyBuffer[1024];
    size_t bytesRead = fread(historyBuffer, 1, sizeof(historyBuffer), history_file);
    fclose(history_file);

    if (send(client_socket, historyBuffer, bytesRead, 0) == -1)
    {
        perror("Error sending message history");
        return;
    }
}
