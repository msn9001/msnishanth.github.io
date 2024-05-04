#include<iostream>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/time.h>
#include<unistd.h>
#include<thread>
#include<mutex>
#include<libgen.h>

#define PORT 9122
#define  MAXCL 100

using namespace std;

std::mutex mtx;

int clientList[MAXCL];

int clientsocks[MAXCL];
int sendsocks[MAXCL];
int leaders[MAXCL];
int no_of_leaders;
int n, k, initial, a, b, nc;
int flag = 1;

struct socmsg{
    int id;
    int soc;
};

struct nclient{
    int pid;
    int csock;
};

void mserver(){
    int opt = 1;
    int serverSocket, addrl, tempSocket, maxClients = MAXCL, activity, i, valread, socdescp, temp;
    int maxsd;
    struct sockaddr_in ipaddr;
    fd_set readfds;
    for (i = 0; i < maxClients; i++)clientList[i] = 0, clientsocks[i] = 0;
    serverSocket = socket(AF_INET , SOCK_STREAM, 0);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    ipaddr.sin_family = AF_INET;
    ipaddr.sin_addr.s_addr = INADDR_ANY;
    ipaddr.sin_port = htons( PORT );
    if(bind(serverSocket, (struct sockaddr *)&ipaddr, sizeof(ipaddr))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
//    cout << "Listener on port :" << PORT << endl;
    listen(serverSocket, 20);
    addrl = sizeof(ipaddr);
//    cout << "Waiting for connections ..." << endl;
    while(flag){
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        maxsd = serverSocket;
        for(i=0;i<maxClients;i++){
            socdescp = clientList[i];
            if(socdescp > 0)FD_SET(socdescp, &readfds);
            if(socdescp > maxsd)maxsd = socdescp;
        }
        activity = select( maxsd + 1 , &readfds , NULL , NULL , NULL);
        if(FD_ISSET(serverSocket, &readfds))
        {
            tempSocket = accept(serverSocket, (struct sockaddr *)&ipaddr, (socklen_t*)&addrl);
//            cout << "New connection, socket fd is :"<< tempSocket <<", ip is :" << inet_ntoa(ipaddr.sin_addr) <<", port is :" << ntohs(ipaddr.sin_port) << endl;
            for(i=0;i<maxClients;i++){
                if(clientList[i] == 0){
                    struct nclient nc;
                    struct socmsg sm;
                    nc.pid = i;
                    clientList[i] = tempSocket;
//                    cout << "New process added with pid:" << i << endl;
                    send(tempSocket, &i, sizeof(int), 0);
                    read(tempSocket, &clientsocks[i], sizeof(int));
                    nc.csock = clientsocks[i];
                    send(tempSocket, &clientsocks, sizeof(int)*MAXCL, 0);
                    send(tempSocket, &n, sizeof(int), 0);
                    send(tempSocket, &n, sizeof(int), 0);
                    send(tempSocket, &initial, sizeof(int), 0);
                    send(tempSocket, &k, sizeof(int), 0);
                    send(tempSocket, &a, sizeof(int), 0);
                    send(tempSocket, &b, sizeof(int), 0);
//                    send(tempSocket, &no_of_leaders, sizeof(no_of_leaders), 0);
                    send(tempSocket, &leaders, sizeof(leaders), 0);
                    for(int j=0;j<maxClients;j++){
                        if(clientList[j]>0 && j!= i){
                            send(clientList[j], &nc, sizeof(struct nclient), 0);
                        }
                    }
//                    cout << "All proceess intimidated of new Process " << i << "." << endl;
                    break;
                }
            }
        }
        for(i=0;i<maxClients;i++){
            socdescp = clientList[i];
            if(FD_ISSET(socdescp, &readfds)){
                if((valread = read(socdescp, &temp, sizeof(int))) == 0){
                    getpeername(socdescp, (struct sockaddr*)&ipaddr, (socklen_t*)&addrl);
                    //cout << "Process " << i <<" disconnected." << endl;
                    close(socdescp);
                    clientList[i] = 0;
                    clientsocks[i] = 0;
                    struct nclient dc;
                    dc.pid = i;
                    dc.csock = 0;
                    for(int j=0;j<maxClients;j++){
                        if(clientsocks[j]>0 && j!=i)send(clientList[j], &dc, sizeof(struct nclient), 0);
                    }
                    //cout << "All proceess intimidated of process " << i << " terminating!" << endl;
                }
                else if(temp == 999){
                    nc--;
                    if(nc<=0){
                        struct nclient dc;
                        dc.pid = 999;
                        dc.csock = 0;
                        for(int j=0;j<maxClients;j++){
                            if(clientsocks[j]>0)send(clientList[j], &dc, sizeof(struct nclient), 0);
                        }
                    }
                }
            }
        }
    }
}
void logPrint(){
    int opt = 1;
    int serverSocket, addrl, tempSocket, clientList[MAXCL], maxClients = MAXCL, activity, i, valread, socdescp;
    int maxsd;
    int msgd[MAXCL];
    double resp_time[MAXCL];
    struct sockaddr_in ipaddr;
    char buffer[65536];
    fd_set readfds;
    char msg[1024];
    for (i = 0; i < maxClients; i++){
        clientList[i] = 0;
    }
    serverSocket = socket(AF_INET , SOCK_STREAM, 0);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    ipaddr.sin_family = AF_INET;
    ipaddr.sin_addr.s_addr = INADDR_ANY;
    ipaddr.sin_port = htons( 9123 );
    if(bind(serverSocket, (struct sockaddr *)&ipaddr, sizeof(ipaddr))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    listen(serverSocket, 20);
    addrl = sizeof(ipaddr);
    while(flag){
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        maxsd = serverSocket;
        for(i=0;i<maxClients;i++){
            socdescp = clientList[i];
            if(socdescp > 0)FD_SET(socdescp, &readfds);
            if(socdescp > maxsd)maxsd = socdescp;
        }
        activity = select( maxsd + 1 , &readfds , NULL , NULL , NULL);
        if(FD_ISSET(serverSocket, &readfds)){
            tempSocket = accept(serverSocket, (struct sockaddr *)&ipaddr, (socklen_t*)&addrl);
            sprintf(msg, "Connection Established!\n");
            send(tempSocket, msg, strlen(msg), 0);
            read(tempSocket, &msgd[--nc], sizeof(msgd[0]));
            read(tempSocket, &resp_time[nc], sizeof(resp_time[0]));
            valread = read(tempSocket, buffer, 65536);
            buffer[valread] = '\0';
            cout << buffer << endl;
            if(nc<=0){
                double sum = 0;
                for(int x=0;x<n;x++){
                    sum += msgd[x];

                }
                cout << "Average Message Complexity in System is " << sum/n << endl;
                sum = 0;
                for(int x=0;x<n;x++){
                    sum += resp_time[x];

                }
                cout << "Average Response Time in System is " << sum/n << "secs." << endl;
                flag = 0;
            }
        }
    }
}

int main(){
    scanf("%d%d%d%d%d", &n, &k, &initial,  &a, &b);
    scanf("%d", &no_of_leaders);

    for(int i = 0; i < MAXCL; i++)
        leaders[i] = 0;

    for(int i = 0; i < n; i++){
        int temp;
        scanf("%d", &temp);
        leaders[temp] = 1;
    }

    nc = n;
    std::thread t1(mserver);
    std::thread t2(logPrint);
    t1.detach();
    t2.join();
    return 0;
}
