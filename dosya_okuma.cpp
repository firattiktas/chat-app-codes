#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>
#include <malloc.h>
#include <limits.h>

#define SERVERPORT 9099
#define BUFSIZE 4096
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 100

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;


using namespace std;
void handle_connection(int client_socket)
{

    char buffer[BUFSIZE];
    size_t bytes_read;
    int msgsize = 0;
    char actualpath[PATH_MAX+1];
    char message[PATH_MAX];
    
    while ((bytes_read = read(client_socket, buffer + msgsize, sizeof(buffer) - msgsize - 1)) > 0)
    {
        msgsize += bytes_read;
        if (msgsize > BUFSIZE - 1 || buffer[msgsize - 1] == '\n')
            break;
    }
    // bytes_read;
    buffer[msgsize - 1] = 0;
    cout << "bytes_read : " << bytes_read << endl;
    cout << "Request " << endl
         << buffer;
    // flush(stdout);

    /*if (realpath(buffer, actualpath) == NULL)
    {
        cout << "error path" << buffer;
        close(client_socket);
        return;
    }*/
    FILE *fp = fopen(buffer, "r");
    if (fp == NULL)
    {
        cout << "ERROR(Open) " << buffer << endl;
        close(client_socket);
        return;
    }
    while ((bytes_read = fread(message, 1, BUFSIZE, fp)) > 0)
    {
        cout << "sending " << bytes_read << " bytes" << endl;
        write(client_socket, message, bytes_read);
    }
    close(client_socket);
    fclose(fp);
    cout << "closing connection";
}
int main()
{
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        cout << "Failed to create a socket" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Socket created succesfully" << endl;

    // initialize the address struct

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVERPORT);

    if ((bind(server_socket, (SA *)&server_addr, sizeof(server_addr))) < 0)
    {
        cout << "Failed to bind" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "bind has been created succesfully" << endl;
    if ((listen(server_socket, SERVER_BACKLOG) < 0))
    {
        cout << "Listen failed" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Listen succesful" << endl;

    while (true)
    {

        cout << "Waiting for connections..." << endl;
        addr_size = sizeof(SA_IN);
        client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *)&addr_size);
        if (client_socket < 0)
        {
            cout << "Accept failed" << endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            cout << "Connected" << endl;
        }

        // do whatever we do with connections.
        handle_connection(client_socket);
    }
    return 0;
}
