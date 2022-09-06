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
#include <thread>
#include <sqlite3.h>

using namespace std;

#define MAX_CLIENTS 200
#define BUFFER_SZ 2048
#define NAME_LEN 32
#define SERVERPORT 9909

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];
string nameT, username, password, animal_name, id;
int enterance;
int uid;

int callback(void *data, int argc, char **argv, char **azColName)
{
    if (username == argv[1] && password == argv[2])
    {
        enterance = 1;
        
    }

    return 0;
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

void catch_ctrl_c_and_exit(int number)
{
    flag = 1;
}

void send_message()
{
    char buffer[BUFFER_SZ] = {};
    char message[BUFFER_SZ + NAME_LEN] = {};

    while (1)
    {
        str_overwrite_stdout();
        fgets(message, BUFFER_SZ, stdin);
        str_trim_lf(message, BUFFER_SZ);

        // cout<<message<<name<<buffer;
        send(sockfd, message, strlen(message), 0);
        bzero(buffer, BUFFER_SZ);
        bzero(message, BUFFER_SZ + NAME_LEN);
    }
    catch_ctrl_c_and_exit(2);
}

void recv_message()
{
    char message[BUFFER_SZ] = {};
    while (1)
    {
        int rec = recv(sockfd, message, BUFFER_SZ, 0);
        if (rec > 0)
        {
            cout << message;
            str_overwrite_stdout();
        }
        else
            break;
        bzero(message, BUFFER_SZ);
    }
}

void newUser()
{

    sqlite3 *create_account;

    cout << "Welcome To The Sign Up Screen " << endl;

    cout << "Please give me your name : ";
    cin >> nameT;
    cout << "Please create a username : ";
    cin >> username;
    cout << "Please type a password : ";
    cin >> password;

    string user = "CREATE TABLE PERSON("
                  "name TEXT NOT NULL, "
                  "uid INTEGER NOT NULL );";
    srand(time(NULL));
    uid = random() % (1000 - 100);
    id = to_string(uid);
    cout << id << endl;
    string sign_up = ("INSERT INTO PERSON VALUES('" + nameT + "','" + username + "','" + password + "','" + id + "');");
    int exit = 0;

    exit = sqlite3_open("kayıt.db", &create_account);

    char *messageError;

    exit = sqlite3_exec(create_account, user.c_str(), NULL, 0, &messageError);
    exit = sqlite3_exec(create_account, sign_up.c_str(), NULL, 0, &messageError);

    if (exit != SQLITE_OK)
    {
        cerr << "Error Create Table" << endl;
        sqlite3_free(messageError);
    }

    else
        cout << "Table created Successfully" << endl;

    sqlite3_close(create_account);
}

int main(int argc, char **argv)
{
    int option = 0;
    cout << "Welcome to the asis chat programming " << endl;
    cout << "Enter options " << endl;
    cout << " If you have an account type anything " << endl;
    cout << "If you do not have an account type 1 " << endl;
    cin >> option;

    if (option == 1)
    {
        newUser();
    }

    while (option != 1)
    {

        cout << endl
             << "What is your username : ";
        cin >> username;
        cout << endl
             << "What is your password : ";
        cin >> password;

        sqlite3 *DB;

        int exit = 0;
        exit = sqlite3_open("kayıt.db", &DB);
        string data("CALLBACK FUNCTION");

        string sql("SELECT * FROM PERSON;");
        if (exit)
        {
            cerr << "Error open DB " << sqlite3_errmsg(DB) << endl;
            return (-1);
        }

        int rc = sqlite3_exec(DB, sql.c_str(), callback, (void *)data.c_str(), NULL);

        if (rc != SQLITE_OK)
            cerr << "Error SELECT" << endl;
        sqlite3_close(DB);

        if (enterance)
            break;
        cout << "Error username or password but I can't tell you which one is wrong " << endl;
    }

    cout << "Enter is succesfull..... Waiting for 3 seconds ...." << endl;
    sleep(3);
    system("tput clear");
    signal(SIGINT, catch_ctrl_c_and_exit);

    struct sockaddr_in serv;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(SERVERPORT);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    int err = connect(sockfd, (struct sockaddr *)&serv, sizeof(serv));

    if (err < 0)
        cout << "Not connected";
    else
        cout << "Connected ... " << endl;

    // declaring character array : p
    char p[sizeof(id)];

    int i;
    for (i = 0; i < sizeof(p); i++)
    {
        p[i] = id[i];
        // cout << p[i];
    }

    if (send(sockfd, p, NAME_LEN, 0) > 0)
        cout << "sending is succesfully" << endl;
    else
        cout << "failed";
    cout << p << endl;

    cout << "Welcome to the chatroom ....  " << endl;

    thread sen(&send_message);
    thread rec(&recv_message);

    rec.join();
    sen.join();

    while (1)
    {
        if (flag)
        {
            cout << "Bye" << endl;
            break;
        }
    }
    close(sockfd);

    return 0;
}
