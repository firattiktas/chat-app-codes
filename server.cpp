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
#include <string>

using namespace std;

#define MAX_CLIENTS 200
#define BUFFER_SZ 2048
#define SERVERPORT 9909
#define NAME_LEN 128

static atomic<unsigned int> cli_count = 0;
static int uid = 10;

char control[BUFFER_SZ];
static string controlS = "basarisiz";

string enterance;
string username_dedect_string;
string password_dedect_string;
static string get_id_from_db;

sqlite3 *DB;

map<string, string> match_map;
map<string, string> online_register;

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
void match_db(char id_register_to_db[1024])
{

    string id_to_db_match;
    id_to_db_match = id_register_to_db;
    char select_rec[BUFFER_SZ];
    char *messageError;

    sqlite3 *match_db;
    sqlite3_open("kayıt.db", &match_db);

    read(connfd, select_rec, NAME_LEN);
    string select_s = select_rec;
    string match_string("UPDATE MATCH SET SECOND = '" + select_s + "' WHERE FIRST = '" + id_to_db_match + "'");

    int a = sqlite3_exec(match_db, match_string.c_str(), NULL, 0, &messageError);
    if (a != SQLITE_OK)
        cout << "fail to succes";

}

void new_user(char keep_id[1024])
{
    cout << "new user is working" << endl;
    char new_username[BUFFER_SZ];
    char new_password[BUFFER_SZ];
    int new_uid_random;
    string new_sit = "OFFLINE";
    string new_id;
    string new_username_db;
    string new_password_db;

    srand(time(NULL));
    new_uid_random = random() % (999 - 100);
    new_id = to_string(new_uid_random);
    cout << new_uid_random << endl;

    strcpy(keep_id, new_id.c_str());

    recv(connfd, new_username, BUFFER_SZ, 0);
    recv(connfd, new_password, BUFFER_SZ, 0);

    new_username_db = new_username;
    new_password_db = new_password;

    // keep_id = new_username;

    cout << "new name is" << new_password_db << endl
         << "char password is" << new_password << endl;

    string sign_up = ("INSERT INTO PERSON VALUES('" + new_username_db + "','" + new_password_db + "','" + new_id + "','" + new_sit + "');");
    string sign_up_match = ("INSERT INTO MATCH VALUES('" + new_id + "','" + new_id + "');");

    int exit = 0;

    exit = sqlite3_open("kayıt.db", &DB);

    char *messageError;

    exit = sqlite3_exec(DB, sign_up.c_str(), NULL, 0, &messageError);

    if (exit != SQLITE_OK)
    {
        cerr << "Error Create Table" << endl;
        sqlite3_free(messageError);
    }
    exit = sqlite3_exec(DB, sign_up_match.c_str(), NULL, 0, &messageError);

    if (exit != SQLITE_OK)
    {
        cerr << "Error Create Table" << endl;
        sqlite3_free(messageError);
    }
}

int callback_dedect(void *data, int argc, char **argv, char **azColName)
{

    if (username_dedect_string == argv[0] && password_dedect_string == argv[1])
    {
        controlS = "basarili";
        cout << argv[0] << " enterance is succes" << endl;
        get_id_from_db = argv[2];
        cout << "getting_id : " << get_id_from_db << endl;
    }

    return 0;
}

void user_dedect(char get_id[1024])
{

    char username_dedect[BUFFER_SZ];
    char password_dedect[BUFFER_SZ];

    memset(username_dedect, 0, NAME_LEN);
    memset(password_dedect, 0, NAME_LEN);
    get_id_from_db.clear();

    int exit = 0;
    exit = sqlite3_open("kayıt.db", &DB);

    string data("CALLBACK FUNCTION");
    string sql("SELECT * FROM PERSON;");

    recv(connfd, username_dedect, BUFFER_SZ, 0);
    recv(connfd, password_dedect, BUFFER_SZ, 0);

    username_dedect_string = username_dedect;
    password_dedect_string = password_dedect;

    cout << "password is : " << password_dedect_string << endl;
    cout << "username is : " << username_dedect_string << endl;

    exit = sqlite3_exec(DB, sql.c_str(), callback_dedect, (void *)data.c_str(), NULL);

    cout << "getting_id : " << get_id_from_db << endl;
    strcpy(get_id, get_id_from_db.c_str());

    cout << "controls = " << controlS << endl;

    strcpy(control, controlS.c_str());
    
    write(connfd, control, sizeof(control));

    cout << "----" << control << endl;

    if (exit != SQLITE_OK)
        cerr << "Error SELECT" << endl;
}

int callback4(void *data, int argc, char **argv, char **azColName)
{
    string online_sit = "ONLINE";
    if (argv[3] == online_sit)
    {
        online_register.insert(pair<string, string>(argv[0], argv[2]));
    }

    return 0;
}

void online_dedect()
{
    string online_sit_send_s;
    char online_sit_send[BUFFER_SZ];

    int exit = 0;
    exit = sqlite3_open("kayıt.db", &DB);

    string data("CALLBACK FUNCTION");
    string sql("SELECT * FROM PERSON;");

    if (exit)
    {
        cerr << "Error open DB " << sqlite3_errmsg(DB) << endl;
        return;
    }

    int rc = sqlite3_exec(DB, sql.c_str(), callback4, (void *)data.c_str(), NULL);

    if (rc != SQLITE_OK)
        cerr << "Error SELECT2" << endl;

    for (auto pair : online_register)
        online_sit_send_s += pair.first + " - " + pair.second + "\n";

    strcpy(online_sit_send, online_sit_send_s.c_str());

    if (write(connfd, online_sit_send, strlen(online_sit_send)) < 0)
    {
        cout << "Failed to send message" << endl;
    }
    cout << "online is : " << online_sit_send;
    cout << online_sit_send << endl;
}

void sit_set_online(string sit_id)
{
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
}

void *handleconnection(void *arg)
{

    char buffer[BUFFER_SZ];

    int leave_flag = 0;
    cli_count++;

    

    client_t *clire = (client_t *)arg;

    online_dedect();

    cout << "name is " << clire->name << "---------" << endl;
    match_db(clire->name);
    match();

    //strcpy(clire->name, name);
    cout << buffer << " joined" << endl
         << clire->name;
    cout << buffer;
    sit_set_online(clire->name);
    cout << buffer;
    sendMessage(buffer, clire->uid);

    bzero(buffer, BUFFER_SZ);
    auto idC = match_map.find(clire->name);
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

        if (cli_count + 1 == MAX_CLIENTS)
        {
            cout << "Client is full  ...  " << endl;
            // print_ip_addr(cli);  // fonksiyonu tanımlamadım.
            close(connfd);
            continue;
        }
        char name[NAME_LEN];
        char enter_option[BUFFER_SZ];
        string enter_option_to_if;
        
        if (recv(connfd, enter_option, NAME_LEN, 0) <= 0)
            cout << "Option situtaion is failed" << endl;

        enter_option_to_if = enter_option;

        if (enter_option_to_if == "H" || enter_option_to_if == "h")
        {
            cout << "in new user" << endl;
            new_user(name);
        }

        else if (enter_option_to_if == "E" || enter_option_to_if == "e")
        {
            cout << "in user dedect" << endl;
            while (1)
            {
                user_dedect(name);
                cout << "break " << controlS << endl;
                if (controlS == "basarili")
                    break;
            }
        }
        //Şimdilik core dumped yapıyor
        //    online_dedect();

        client_t *clire = (client_t *)malloc(sizeof(client_t));
        clire->adress = cli;
        clire->sockfd = connfd;
        clire->uid = uid++;
        strcpy(clire->name,name);
        queue_add(clire);
        pthread_create(&trd, NULL, &handleconnection, (void *)clire);

        sleep(1);
    }
    return 0;
}
