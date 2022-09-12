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
char control[BUFFER_SZ];
static int uid;
string sit = "OFFLINE";
string controlS;

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

void userdedect()
{

        
        cout << "Username : ";
        cin >> username;
        send(sockfd, username, strlen(username), 0);

        cout << "Password : ";
        cin >> password;
        send(sockfd, password, strlen(password), 0);
        cout << "control first is " << control << endl;
        if (read(sockfd, control, NAME_LEN) > 0)
            cout << "rec suc"<<endl;

        cout << "control is : " << control << endl;
        memset(username,0,NAME_LEN);
        memset(password,0,NAME_LEN);
        memset(control,0,NAME_LEN);

}

void newUser()
{
    cout << "Welcome To The Sign Up Screen " << endl;

    cout << "Please create a username : ";
    cin >> username;
    send(sockfd, username, strlen(username), 0);

    cout << "Please create a password :  ";
    cin >> password;
    send(sockfd, password, strlen(password), 0);
}

void online_dedector()
{
char online_sit[BUFFER_SZ] = {};
    
    
        int rec = recv(sockfd, online_sit, BUFFER_SZ, 0);
        if (rec > 0)
        {
            cout << online_sit;
            str_overwrite_stdout();
        }
        
        bzero(online_sit, BUFFER_SZ);
    

}

void match_db()
{
    char select[BUFFER_SZ];
    cout << "Select the ID you want to send message" << endl;
    cin >> select;
    int wrt = write(sockfd,select,sizeof(select));
    if(wrt<=0)
        cout<<"failed to select"<<endl;

}

int main(int argc, char **argv)
{
    struct sockaddr_in serv;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(SERVERPORT);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    int err = connect(sockfd, (struct sockaddr *)&serv, sizeof(serv));



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
        while(1)
        {
            memset(username,0,NAME_LEN);
            memset(password,0,NAME_LEN);
            memset(control,0,NAME_LEN);
            cout << "Username : ";
            cin >> username;
            send(sockfd, username, strlen(username), 0);

            cout << "Password : ";
            cin >> password;
            send(sockfd, password, strlen(password), 0);
            cout << "control first is " << control << endl;
            
         //   if (recv_message() )
           //     cout << "rec suc"<<endl;
                   int rec = recv(sockfd, control, BUFFER_SZ, 0);

            cout << "control is : " << control << endl;

            controlS=control;
            if(controlS == "basarili")
                break;
            cout<<"hatali giris tekrar deneyin"<<endl;
            bzero(control,BUFFER_SZ);
        }
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
    //sleep(3);
    // system("tput clear");

    signal(SIGINT, catch_ctrl_c_and_exit);

    cout << "Online kullanicilar : " << endl;

    online_dedector();
    char online_sit[BUFFER_SZ];
    recv(sockfd, online_sit, BUFFER_SZ, 0);
    cout<<"Online users "<<endl<<online_sit;

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
