#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream>
#include <atomic>
#include <sqlite3.h>
#include <map>
#include <vector>

using namespace std;

#define MAX_CLIENTS 200
#define BUFFER_SZ 2048
#define SERVERPORT 9909
#define NAME_LEN 128

static atomic<unsigned int> cli_count = 0;
static int uid = 10;

map<string, string> match_map;

class client_t
{
public:
    struct sockaddr_in adress;
    int sockfd;
    int uid; // KULLANICIYA ÖZEL OLACAK.
    char name[NAME_LEN];
    char matched_id[BUFFER_SZ];
};

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int option = 1;
int listenfd, connfd;

struct sockaddr_in serv;
struct sockaddr_in cli;

pthread_t trd;

void sit_set_online(string sit_id)
{
    sqlite3 *DB;
    char *messageError;
    string sql("UPDATE PERSON SET SIT = 'ONLINE' WHERE ID = '" + sit_id + "'");

    int exit = sqlite3_open("kayıt.db", &DB);
    /* An open database, SQL to be evaluated, Callback function, 1st argument to callback, Error msg written here */
    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messageError);
    if (exit != SQLITE_OK)
    {
        cerr << "Error in updateData function." << endl;
        sqlite3_free(messageError);
    }
    else
        cout << "Online ise succes";
}

void sit_set_offline(string sit_id)
{
    sqlite3 *DB;
    char *messageError;
    string sql("UPDATE PERSON SET SIT = 'OFFLINE' WHERE ID = '" + sit_id + "'");

    int exit = sqlite3_open("kayıt.db", &DB);

    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messageError);
    if (exit != SQLITE_OK)
    {
        cerr << "Error in updateData function." << endl;
        sqlite3_free(messageError);
    }
}

void str_overwrite_stdout()
{
    cout << "\r%s"
         << "> ";
    fflush(stdout);
}

void str_trim_lf(char *arr, int lenght)
{
    for (int i = 0; i < lenght; i++)
    {
        if (arr[i] == '\n')
        {
            arr[i] == '\0';
            break;
        }
    }
}

void queue_add(client_t *cl)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!clients[i])
        {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void queue_remove(int uid)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                clients[i] = NULL;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void setupServer()
{
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(SERVERPORT);

    signal(SIGPIPE, SIG_IGN);

    // setsockopt nedir?
    if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
    {
        cout << "Error setsockopt";
    }
    if ((bind(listenfd, (struct sockaddr *)&serv, sizeof(serv))) < 0)
    {
        cout << "Failed to bind" << endl;
    }

    if ((listen(listenfd, MAX_CLIENTS) < 0))
    {
        cout << "Listen failed" << endl;
    }
}

void sendMessage(char *message, int uid)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i])
        {
            if (clients[i]->uid != uid)
            {
                if (write(clients[i]->sockfd, message, strlen(message)) < 0)
                {
                    cout << "Failed to send message" << endl;
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void sendMessage2(char *message, string send_id)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i])
        {
            if (clients[i]->name == send_id)
            {
                if (write(clients[i]->sockfd, message, strlen(message)) < 0)
                {
                    cout << "Failed to send message" << endl;
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

int callback(void *data, int argc, char **argv, char **azColName)
{
    match_map.insert(pair<string, string>(argv[0], argv[1]));
    return 0;
}

void match()
{
    sqlite3 *DB;
    int exit = 0;
    exit = sqlite3_open("kayıt.db", &DB);
    string data("CALLBACK FUNCTION");

    string sql("SELECT * FROM MATCH;");
    if (exit)
    {
        cerr << "Error open DB " << sqlite3_errmsg(DB) << endl;
        return;
    }
    int rc = sqlite3_exec(DB, sql.c_str(), callback, (void *)data.c_str(), NULL);

    if (rc != SQLITE_OK)
        cerr << "Error SELECT2" << endl;
    sqlite3_close(DB);
}

void *handleconnection(void *arg)
{

    char buffer[BUFFER_SZ];
    char name[NAME_LEN];
    int leave_flag = 0;
    cli_count++;

    client_t *clire = (client_t *)arg;

    if (recv(clire->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) >= NAME_LEN - 1)
    {
        cout << "Enter name is correctly" << endl;
        leave_flag = 1;
    }
    else
    {
        strcpy(clire->name, name);
        cout << buffer << " joined" << endl
             << clire->name;
        cout << buffer;
        sit_set_online(clire->name);
        cout << buffer;
        sendMessage(buffer, clire->uid);
    }

    bzero(buffer, BUFFER_SZ);
    auto idC = match_map.find(name);
    string sending = idC->second;
    while (1)
    {
        if (leave_flag)
            break;
        int receive = recv(clire->sockfd, buffer, BUFFER_SZ, 0);

        if (receive > 0)
        {
            if (strlen(buffer) > 0)
            {
                // sendMessage(buffer, clire->uid);
                sendMessage2(buffer, sending);
                str_trim_lf(buffer, strlen(buffer));
                printf("%s -> %s", buffer, clire->name);
            }
        }
        else if (receive == 0 || strcmp(buffer, "exit") == 0)
        {
            cout << buffer << " has left" << endl
                 << clire->name;
            sit_set_offline(clire->name);
            cout << buffer;
            // sendMessage(buffer, clire->uid);
            sendMessage2(buffer, sending);
            leave_flag = 1;
        }
        bzero(buffer, BUFFER_SZ);
    }
    close(clire->sockfd);
    queue_remove(clire->uid);
    free(clire);
    pthread_detach(pthread_self());
    return NULL;
}

int main(int argc, char **argv)
{
    setupServer();

    while (1)
    {

        socklen_t cliLen = sizeof(cli);
        connfd = accept(listenfd, (struct sockaddr *)&cli, &cliLen); // CliLen referans olması şüpheli.
        match();

        // Gereksiz gibi duruyor.
        if (cli_count + 1 == MAX_CLIENTS)
        {
            cout << "Client is full  ...  " << endl;
            // print_ip_addr(cli);  // fonksiyonu tanımlamadım.
            close(connfd);
            continue;
        }

        client_t *clire = (client_t *)malloc(sizeof(client_t));
        clire->adress = cli;
        clire->sockfd = connfd;
        clire->uid = uid++;
        queue_add(clire);
        pthread_create(&trd, NULL, &handleconnection, (void *)clire);

        sleep(1);
    }
    return 0;
}
