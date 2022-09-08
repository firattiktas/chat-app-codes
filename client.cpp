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
string nameT, id;
char username[BUFFER_SZ];
char password[BUFFER_SZ];
int control;
static int uid;
string sit = "OFFLINE";

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

int callback(void *data, int argc, char **argv, char **azColName)
{
    if (username == argv[0] && password == argv[1])
    {
        control = 1;
        id = argv[2];
    }

    return 0;
}

void userdedect()
{
    while (1)
    {
        cout << "Kullanici adinizi giriniz : " << endl;
        cin >> username;
        cout << "Şifrenizi girniz : " << endl;
        cin >> password;
        sqlite3 *DB;

        int exit = 0;
        exit = sqlite3_open("kayıt.db", &DB);
        string data("CALLBACK FUNCTION");

        string sql("SELECT * FROM PERSON;");
        if (exit)
        {
            cerr << "Error open DB " << sqlite3_errmsg(DB) << endl;
            return;
        }

        int rc = sqlite3_exec(DB, sql.c_str(), callback, (void *)data.c_str(), NULL);

        if (rc != SQLITE_OK)
            cerr << "Error SELECT" << endl;
        sqlite3_close(DB);
        if (control)
            break;
        cout << "Kullanici adi veya şifre yanlis !!!!!" << endl;
    }
}

void newUser()
{
    cout << "Welcome To The Sign Up Screen " << endl;
    
    cout << "Please create a username : ";
    cin >> username;

    send(sockfd, username, strlen(username), 0);
    cout << "Please type a password : ";
    cin >> password;
    send(sockfd, password, strlen(password), 0);

}

int callback2(void *data, int argc, char **argv, char **azColName)
{
    string online_sit = "ONLINE";
    if (argv[3] == online_sit)
    {
        cout << "AD : " << argv[0] << " -----  ID : " << argv[2] << endl;
    }

    return 0;
}

void online_dedect()
{
    sqlite3 *DB;
    int exit = 0;
    exit = sqlite3_open("kayıt.db", &DB);
    string data("CALLBACK FUNCTION");

    string sql("SELECT * FROM PERSON;");
    if (exit)
    {
        cerr << "Error open DB " << sqlite3_errmsg(DB) << endl;
        return;
    }
    int rc = sqlite3_exec(DB, sql.c_str(), callback2, (void *)data.c_str(), NULL);

    if (rc != SQLITE_OK)
        cerr << "Error SELECT2" << endl;
    sqlite3_close(DB);
}

void match_db()
{
    string select;
    char *messageError;

    cout << "Select the ID you want to send message" << endl;
    cin >> select;

    sqlite3 *match_db;
    sqlite3_open("kayıt.db", &match_db);
    
    string match_string("UPDATE MATCH SET SECOND = '"+select+"' WHERE FIRST = '" +id+ "'");
    
    int a =sqlite3_exec(match_db, match_string.c_str(), NULL, 0, &messageError);
    if(a != SQLITE_OK)
        cout<<"fail to succes";
    
    sqlite3_close(match_db);
}

int main(int argc, char **argv)
{
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


    cout << "Welcome to the chat app ....  " << endl;
    char option[BUFFER_SZ];

    cout << "KAYITLI MISIN" << endl;
    cin >> option;

    if (send(sockfd, option, NAME_LEN, 0) > 0)
        cout << "Request is succesfully" << endl;
    else
        cout << "failed";
    string enter_option = option;
    if (enter_option == "E" || enter_option == "e")
    {
        
        userdedect();
    }
    else if (enter_option == "H" || enter_option == "h")
    {
        newUser();
    }
    else
    {
        cout << "Hatali tuslama yaptiniz" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Enter is succesfull..... Waiting for 3 seconds ...." << endl;
    sleep(3);
    system("tput clear");
    
    signal(SIGINT, catch_ctrl_c_and_exit);

    cout << "Online kullanicilar : " << endl;
    
    online_dedect();
    
    match_db();

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


    // declaring character array : p

