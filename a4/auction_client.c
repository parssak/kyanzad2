// add localhost 50101
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 128

#define MAX_AUCTIONS 5
#ifndef VERBOSE
#define VERBOSE 1
#endif

#define ADD 0
#define SHOW 1
#define BID 2
#define QUIT 3

/* Auction struct - this is different than the struct in the server program
 */
struct auction_data
{
    int sock_fd;
    char item[BUF_SIZE];
    int current_bid;
};

/* Displays the command options available for the user.
 * The user will type these commands on stdin.
 */

void print_menu()
{
    printf("The following operations are available:\n");
    printf("    show\n");
    printf("    add <server address> <port number>\n");
    printf("    bid <item index> <bid value>\n");
    printf("    quit\n");
}

/* Prompt the user for the next command 
 */
void print_prompt()
{
    printf("Enter new command: ");
    fflush(stdout);
}

/* Unpack buf which contains the input entered by the user.
 * Return the command that is found as the first word in the line, or -1
 * for an invalid command.
 * If the command has arguments (add and bid), then copy these values to
 * arg1 and arg2.
 */
int parse_command(char *buf, int size, char *arg1, char *arg2)
{
    int result = -1;
    char *ptr = NULL;
    if (strncmp(buf, "show", strlen("show")) == 0)
    {
        return SHOW;
    }
    else if (strncmp(buf, "quit", strlen("quit")) == 0)
    {
        return QUIT;
    }
    else if (strncmp(buf, "add", strlen("add")) == 0)
    {
        result = ADD;
    }
    else if (strncmp(buf, "bid", strlen("bid")) == 0)
    {
        result = BID;
    }
    ptr = strtok(buf, " "); // first word in buf

    ptr = strtok(NULL, " "); // second word in buf
    if (ptr != NULL)
    {
        strncpy(arg1, ptr, BUF_SIZE);
    }
    else
    {
        return -1;
    }
    ptr = strtok(NULL, " "); // third word in buf
    if (ptr != NULL)
    {
        strncpy(arg2, ptr, BUF_SIZE);
        return result;
    }
    else
    {
        return -1;
    }
    return -1;
}

/* Connect to a server given a hostname and port number.
 * Return the socket for this server
 */
int add_server(char *hostname, int port)
{
    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    struct addrinfo *ai;

    /* this call declares memory and populates ailist */
    if (getaddrinfo(hostname, NULL, NULL, &ai) != 0)
    {
        close(sock_fd);
        return -1;
    }
    /* we only make use of the first element in the list */
    server.sin_addr = ((struct sockaddr_in *)ai->ai_addr)->sin_addr;

    // free the memory that was allocated by getaddrinfo for this list
    freeaddrinfo(ai);

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        printf("bad 1\n");
        perror("client: connect");
        close(sock_fd);
        return -1;
    }
    if (VERBOSE)
    {
        fprintf(stderr, "\nDebug: New server connected on socket %d.  Awaiting item\n", sock_fd);
    }
    return sock_fd;
}
/* ========================= Add helper functions below ========================
 * Please add helper functions below to make it easier for the TAs to find the 
 * work that you have done.  Helper functions that you need to complete are also
 * given below.
 */

/* Print to standard output information about the auction
 */
void print_auctions(struct auction_data *a, int size)
{
    printf("Current Auctions:\n");
    for (int i = 0; i < size; i++)
    {
        if (a[i].sock_fd != -1)
            printf("(%d) %s bid = %d\n", i, a[i].item, a[i].current_bid);
    }
}

/* Process the input that was sent from the auction server at a[index].
 * If it is the first message from the server, then copy the item name
 * to the item field.  (Note that an item cannot have a space character in it.)
 */
void update_auction(char *buf, int size, struct auction_data *a, int index)
{
    // If it is the first message, add it at index
    int isFirstMessage = 0;

    if (a[index].sock_fd == -1)
        isFirstMessage = 1;

    const char *format = "%s %d %d";
    char itemName[BUF_SIZE];
    int currentBid = 0;
    int timeLeft = 0;
    if (isFirstMessage == 1)
    {
        strncpy(a[index].item, itemName, BUF_SIZE);
    }

    sscanf(buf, format, itemName, &currentBid, &timeLeft);
    // Auction is closed now
    if (strcmp(itemName,"Auction") == 0) {
        a[index].sock_fd = -1;
    } else
    {
        strncpy(a[index].item, itemName, BUF_SIZE);
        a[index].current_bid = currentBid;
        printf("\nNew bid for %s [%d] is %d (%d seconds left)\n", itemName, index, currentBid, timeLeft);
    }
}

int main(void)
{
    char name[BUF_SIZE];

    // Declare and initialize necessary variables

    struct auction_data data[MAX_AUCTIONS];
    
    // Init all sock_fd to -1, this is the empty case
    for (int i = 0; i < MAX_AUCTIONS; i++)
        data[i].sock_fd = -1;

    char input[BUF_SIZE];
    char param1[BUF_SIZE];
    char param2[BUF_SIZE];

    int result = 0;
    int bidIndex = 0;
    int paramToPort = 0;
    char *ptr = NULL;

    char read_buffer[BUF_SIZE];

    // Get the user to provide a name.
    printf("Please enter a username: ");
    fflush(stdout);
    int num_read = read(STDIN_FILENO, name, BUF_SIZE);
    if (num_read <= 0)
    {
        fprintf(stderr, "ERROR: read from stdin failed\n");
        exit(1);
    }
    print_menu();

    // Sockets
    fd_set sockets;
    int max_sock = 0;

    while (1)
    {
        print_prompt();

        FD_ZERO(&sockets);
        FD_SET(STDIN_FILENO, &sockets);
        max_sock = 0;
        // add to the set
        for (int i = 0; i < MAX_AUCTIONS; i++)
        {
            if (data[i].sock_fd > max_sock)
                max_sock = data[i].sock_fd;

            FD_SET(data[i].sock_fd, &sockets);
        }
        if (select(max_sock + 1, &sockets, NULL, NULL, NULL) < 0)
        {
            perror("select error");
            exit(1);
        }

        for (int i = 0; i <= max_sock; i++)
        {
            if (FD_ISSET(i, &sockets))
            {
                if (i != STDIN_FILENO)
                {
                    read(i, read_buffer, BUF_SIZE);
                    for (int j = 0; j < MAX_AUCTIONS; j++)
                    {
                        if (data[j].sock_fd == i)
                            update_auction(read_buffer, BUF_SIZE, data, j);
                    }
                }
                else
                {
                    fgets(input, BUF_SIZE, stdin);
                    result = parse_command(input, 2, param1, param2);

                    switch (result)
                    {
                    case SHOW:
                        print_auctions(data, MAX_AUCTIONS);
                        break;
                    case ADD:
                        paramToPort = strtol(param2, &ptr, 10);
                        int new_sock = add_server(param1, paramToPort);
                        write(new_sock, name, BUF_SIZE);
                        read(new_sock, read_buffer, BUF_SIZE);

                        // Get an available location
                        int nextAvailable = -1;
                        for (int i = 0; i < MAX_AUCTIONS; i++)
                        {
                            if (data[i].sock_fd == -1)
                            {
                                nextAvailable = i;
                                break;
                            }
                        }

                        update_auction(read_buffer, BUF_SIZE, data, nextAvailable);
                        data[nextAvailable].sock_fd = new_sock;
                        break;
                    case BID:
                        bidIndex = strtol(param1, &ptr, 10);
                        if (data[bidIndex].sock_fd == -1)
                            printf("There is no auction open at %d\n", bidIndex);
                        else
                            write(data[bidIndex].sock_fd, param2, BUF_SIZE);
                        break;
                    case QUIT:
                        return 0 ;
                    default:
                        print_menu();
                        break;
                    }
                }
            }
        }
    }

    return 0; // Should never get here
}
