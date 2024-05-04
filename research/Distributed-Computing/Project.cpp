#include<iostream>
#include<string.h>
#include<stdlib.h>
#include<assert.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/time.h>
#include<unistd.h>
#include<pthread.h>
#include<mutex>
#include<chrono>
#include<thread>
#include<time.h>

#define  MAXCL 100

using namespace std;

std::mutex mtx;
std::mutex mtx1;
std::mutex mtx2;
std::mutex mtx3;
std::mutex mtx4;
std::mutex mtx5;
std::mutex mtxchild;

char log[65536];
char tim[10];
char slt[20];
int PORT;
int clientsocks[MAXCL];
int clientList[MAXCL];
int leaders[100];
int no_of_leaders;
int am_i_leader = 0;
int my_leader = 0;
int nt, initial, k, ncon, ne, a, b;

int my_req_no = 0;
int executing_cs = 0;
int message_complexity = 0;
unsigned long long int response_time = 0;
char time_of_request[10], time_of_entering[10];


int terminated[MAXCL];
struct socmsg{
    int id;
    int soc;
};

struct nclient{
    int pid;
    int csock;
};

//msgtype 0 means request, and 1 means token
//11 means normal_request, 12 means grant, 13 means release.
int smsgtype, rmsgtype;

struct req_message{
    int sid;
    int did;
}sreq, rreq;

struct token_message{
    int sid;
    int did;
}token;

struct grant_message{
    int sid;
    int did;
}grant;

struct release_message{
    int sid;
    int did;
}release;

int mysoc, mypid;
int req_queue[MAXCL], queue_front;
int parent_id;


//Get the current localtime and write it
//to the String named tim
void gettime(){
    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime (tim, 10,"%I:%M:%S%p",timeinfo);
}


//Insert a process id to the queue
void enqueue(int key){
    for(int i=queue_front;i>=0;i--)req_queue[i+1] = req_queue[i];
    req_queue[0] = key;
    queue_front++;
}


//Remove a process id from the queue and return
int dequeue(){
    queue_front--;
    return req_queue[queue_front];
}


//Thread terminates once it has detected all other processes have terminated
void checkTerminate(){
    int flag = 1;
    while(flag){
        flag  = 0;
        mtx.lock();
        for(int i=0;i<nt;i++){
            if(!terminated[i]){
                flag = 1;
                break;
            }
        }
        mtx.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


//Connect to the smsg.did process
//and the send the message type information
//followed by the data message.
void sendMessage(){
    message_complexity++;
    if(smsgtype == 0 || smsgtype == 2 || smsgtype == 11){
        struct sockaddr_in address;
        int sock = 0;
        if(smsgtype == 0 || smsgtype == 11)
            cout << "Sending request to " << sreq.did << endl;
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[sreq.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        send(sock, &smsgtype, sizeof(smsgtype), 0);
        send(sock, &sreq, sizeof(sreq), 0);
        close(sock);
    }
    else if(smsgtype == 12){
        struct sockaddr_in address;
        int sock = 0;
        gettime();
        cout << "sends grant to " << grant.did << endl;
        sprintf(log, "%s\nLeader l%d sends grant to p%d at %s", log, mypid, grant.did, tim);
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[grant.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        send(sock, &smsgtype, sizeof(smsgtype), 0);
        send(sock, &grant, sizeof(token), 0);
        close(sock);
    }
    else if(smsgtype == 13){
        struct sockaddr_in address;
        int sock = 0;
        gettime();
        cout << "sending release to " << release.did << endl;
        sprintf(log, "%s\np%d sends release to p%d at %s", log, mypid, release.did, tim);
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[release.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        send(sock, &smsgtype, sizeof(smsgtype), 0);
        send(sock, &release, sizeof(token), 0);
        close(sock);
    }
    else{
        struct sockaddr_in address;
        int sock = 0;
        cout << "Sending token to " << token.did << endl;
        gettime();
        sprintf(log, "%s\nLeader l%d sends token to p%d at %s", log, mypid, token.did, tim);
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[token.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        send(sock, &smsgtype, sizeof(smsgtype), 0);
        send(sock, &token, sizeof(token), 0);
        close(sock);
    }
    return;
}


//Thread to remove an element from the queue,
//retrieve the token and pass it to the
//requesting process and keep retpeating
//unless queue is empty
void processRequests(){
    while(queue_front){
        while(executing_cs);
        mtx.lock();
        int id = dequeue();
        if(parent_id!=mypid){
            sreq.sid = mypid;
            sreq.did = parent_id;
            smsgtype = 0;
            gettime();
            if(id!=mypid)
                sprintf(log, "%s\nLeader l%d forwarded l%d's request to enter CS at %s", log, mypid, rreq.sid, tim);
            sendMessage();
        }
        mtx.unlock();
        while(parent_id != mypid);
        mtx.lock();
        if(id == mypid){
            executing_cs = 1;
            mtx4.unlock();
        }
        else if(leaders[id]){
            token.sid = mypid;
            token.did = id;
            smsgtype = 1;
            parent_id = id;
            sendMessage();
        }
        else{
            grant.sid = mypid;
            grant.did = id;
            smsgtype = 12;
            sendMessage();
        }
        mtx.unlock();
        if(!leaders[id])
            mtxchild.lock();
    }
    mtx5.unlock();
}


//Every process creates its own server
//so that other processes may connect to it
//All incoming messages other than those
//from the master Server arrive here
//and are then either processed as they come
//or handed over as a job to a new thread.
void* server(void * tid){
    int opt = 1;
    int serverSocket, addrl, tempSocket, maxClients = MAXCL, activity, i, valread, socdescp, temp;
    int maxsd;
    struct sockaddr_in ipaddr;
    srand(time(0));
    PORT = rand()%10000 + 5000;
    fd_set readfds;
    serverSocket = socket(AF_INET , SOCK_STREAM, 0);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    ipaddr.sin_family = AF_INET;
    ipaddr.sin_addr.s_addr = INADDR_ANY;
    ipaddr.sin_port = htons(PORT);
    if(bind(serverSocket, (struct sockaddr *)&ipaddr, sizeof(ipaddr))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    mysoc = serverSocket;
    listen(serverSocket, 3);
    addrl = sizeof(ipaddr);
    mtx1.unlock();
    while(1){
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        maxsd = serverSocket;
        for(i=0;i<maxClients;i++){
            socdescp = clientList[i];
            if(socdescp > 0)FD_SET(socdescp, &readfds);
            if(socdescp > maxsd)maxsd = socdescp;
        }
        activity = select(maxsd + 1, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(serverSocket, &readfds))
        {
            tempSocket = accept(serverSocket, (struct sockaddr *)&ipaddr, (socklen_t*)&addrl);
            for(i=0;i<maxClients;i++){
                if(clientList[i] == 0){
                    clientList[i] = tempSocket;
                    read(tempSocket, &rmsgtype, sizeof(rmsgtype));
                    mtx.lock();
                    if(rmsgtype == 11){
                        assert(am_i_leader && "Child processs recieved request from another child!!");
                        read(tempSocket, &rreq, sizeof(rreq));
                        cout << "Received request from Child " << rreq.sid << endl;
                        gettime();
                        sprintf(log, "%s\nLeader l%d recieves p%d's request to enter CS at %s", log, mypid, rreq.sid, tim);
                        if(mtx5.try_lock()){
                            enqueue(rreq.sid);
                            std::thread request_processor(processRequests);
                            request_processor.detach();
                        }
                        else{
                            enqueue(rreq.sid);
                        }
                    }
                    else if(rmsgtype == 12){
                        read(tempSocket, &grant, sizeof(release));
                        cout << "Recieved grant from " << grant.sid << endl;
                        gettime();
                        sprintf(log, "%s\np%d recieves grant from l%d at %s", log, mypid, grant.sid, tim);
                        mtx4.unlock();
                    }
                    else if(rmsgtype == 13){
                        read(tempSocket, &release, sizeof(release));
                        cout << "Recieved release from " << release.sid << endl;
                        gettime();
                        sprintf(log, "%s\nLeader l%d recieves release from p%d at %s", log, mypid, release.sid, tim);
                        mtxchild.unlock();
                    }
                    else if(rmsgtype == 0){
                        read(tempSocket, &rreq, sizeof(rreq));
                        cout << "Recieved request from " << rreq.sid << endl;
                        gettime();
                        sprintf(log, "%s\nLeader l%d recieves l%d's request to enter CS at %s", log, mypid, rreq.sid, tim);
                        if(mtx5.try_lock()){
                            enqueue(rreq.sid);
                            std::thread request_processor(processRequests);
                            request_processor.detach();
                        }
                        else{
                            enqueue(rreq.sid);
                        }
                    }
                    else if(rmsgtype == 1){
                        read(tempSocket, &token, sizeof(token));
                        cout << "Recieved token from " << token.sid << endl;
                        gettime();
                        sprintf(log, "%s\nLeader l%d recieves token from l%d at %s", log, mypid, token.sid, tim);
                        parent_id = mypid;
                    }
                    else if(rmsgtype == 2){
                        read(tempSocket, &rreq, sizeof(token));
                        terminated[rreq.sid] = 1;
                    }
                    mtx.unlock();
                    break;
                }
            }
        }
        for(i=0;i<maxClients;i++){
            socdescp = clientList[i];
            if(FD_ISSET(socdescp, &readfds)){
                if((valread = read(socdescp, &temp, sizeof(int))) == 0){
                    getpeername(socdescp, (struct sockaddr*)&ipaddr, (socklen_t*)&addrl);
                    close(socdescp);
                    clientList[i] = 0;
                }
            }
        }
    }
}


//This function establishes connection
//with the master Server so that it may
//send the master server his servers port no,
//and in turn the master server assigns it
//a id and also shares the ports of other
//connected process.
//Also it used for intimidation when a process leaves the system.
void *getPid(void *tid){
    mtx1.lock();
    struct sockaddr_in address;
    int sock = 0;
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9122);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    read(sock, &mypid, sizeof(int));
    cout << "My Process id: " << mypid << "." << endl;
    send(sock, &PORT, sizeof(int), 0);
    read(sock, &clientsocks, sizeof(int)*MAXCL);
    read(sock, &nt, sizeof(nt));
    read(sock, &ncon, sizeof(ncon));
    read(sock, &initial, sizeof(initial));
    read(sock, &k, sizeof(k));
    read(sock, &a, sizeof(a));
    read(sock, &b, sizeof(b));
//    read(sock, &no_of_leaders, sizeof(no_of_leaders));
    read(sock, &leaders, sizeof(leaders));
    am_i_leader = leaders[mypid];
    if(!am_i_leader){
        int i = mypid+1;
        while(!leaders[i])
            i++;
        my_leader = i;
        cout << "I am a child.\nMy leader is " << my_leader << endl;
    }
    else{
        if(mypid == initial){
            cout << "I am a leader and I hold the token" << endl;
            parent_id = mypid;

        }
        else if(mypid  < initial){
            int i = mypid+1;
            while(!leaders[i])
                i++;
            parent_id = i;
            cout << "I am a leader, I point to " << parent_id << endl;
        }
        else{
            int i = mypid-1;
                while(!leaders[i])
                    i--;
                parent_id = i;
            cout << "I am a leader, I point to " << parent_id << endl;
        }
    }
    cout << "Processes currently online:" << endl;
    mtx.lock();
    for(int i=0;i<nt;i++){
        if(clientsocks[i]>0 && i!=mypid){
            cout << i << " ";
            ne++;
        }
    }
    cout << endl;
    if(ne>=ncon)mtx2.unlock();
    if(ne<ncon)cout << "Waiting for " << ncon-ne << " processes to join..." << endl;
    mtx.unlock();
    cout << endl;
    struct nclient cl;
    mtx1.unlock();
    while(1){
        read(sock, &cl, sizeof(struct nclient));
        if(ne<ncon)cout << "Message From Server." << endl;
        mtx.lock();
        if(cl.csock>0){
            if(ne<ncon)cout << "New Process joined with id: " << cl.pid << " & socket: " << cl.csock << "." << endl;
            ne++;
            if(ne<ncon)cout << "Waiting for " << ncon-ne << " more processes to join..." << endl;
            if(ne>=ncon)mtx2.unlock();
        }
//        else{
//            cout << "Process left with id: "<< cl.pid <<" &  socket: " << clientsocks[cl.pid] <<"." << endl;
//        }
        clientsocks[cl.pid] = cl.csock;
        mtx.unlock();
    }
}


//This Function sends the log files
//to the master server so that all the log
//maybe printed in a single file.
void sendLog(){
    double avg_response_time = (double)response_time/nt;
    struct sockaddr_in serv_addr;
    sprintf(log, "%s\nMessages exchanged by p%d is %d.", log, mypid, message_complexity);
    sprintf(log, "%s\nAverage Reponse time of p%d is %0.2lf secs.\n", log, mypid, avg_response_time);
    char tempmsg[1024];
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9123);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    read(sock, tempmsg, 1024);
    send(sock, &message_complexity, sizeof(message_complexity), 0);
    send(sock, &avg_response_time, sizeof(avg_response_time), 0);
    send(sock, log, strlen(log), 0);
    close(sock);
    return;
}


//Function to calculate the response
//time by comparing time of request
//and time of request being granted
void calcResponseTime(){
    unsigned h1, h2, m1, m2, s1, s2;
    unsigned long long t1, t2;
    sscanf(time_of_request, "%d:%d:%d", &h1, &m1, &s1);
    sscanf(time_of_entering, "%d:%d:%d", &h2, &m2, &s2);
    t1 = h1 * 60 * 60 + m1 * 60 + s1;
    t2 = h2 * 60 * 60 + m2 * 60 + s2;
    response_time += t2-t1;
}


//Thread to sleep for sleeping
//rand(b) seconds to simulate
//work inside CS
void executeCS(){
    int w = rand() % b;
    w++;
    cout << "Executing CS for " << w << "secs..." << endl;
    gettime();
    if(!am_i_leader)
        sprintf(log, "%s\np%d enters CS at %s", log, mypid,  tim);
    else
        sprintf(log, "%s\nLeader l%d enters CS at %s", log, mypid,  tim);
    strcpy(time_of_entering, tim);
    std::this_thread::sleep_for(std::chrono::seconds(w));
    gettime();
    if(!am_i_leader)
        sprintf(log, "%s\np%d leaves CS at %s", log, mypid,  tim);
    else
        sprintf(log, "%s\nLeader l%d leaves CS at %s", log, mypid,  tim);
    cout << "left CS" << endl;
}

//This is for simulating work done
//by the process by sleeping for
//rand(a) period of time
void work(){
    int w = rand() % a;
    w++;
    cout << "Executing local computation for " << w << "secs..." << endl;
    gettime();
    if(!am_i_leader)
        sprintf(log, "%s\np%d is doing local computation at %s", log, mypid,  tim);
    else
        sprintf(log, "%s\nLeader l%d is doing local computation at %s", log, mypid,  tim);
    std::this_thread::sleep_for(std::chrono::seconds(w));
}


//This schedules the different tasks done by the process,
//like sending messages or doing work
void autoWork(){
    cout << "Waiting for processes to join..." << endl;
    mtx2.lock();
    ncon = 0;
    for(int i=0;i<nt;i++)if(i!=mypid)ncon++;
    cout << "Started Auto Executing Tasks..." << endl;
    for(int i=0;i<k;i++){
        std::thread worker(work);
        worker.join();
        strcpy(time_of_request, tim);
        my_req_no++;
        gettime();
        if(!am_i_leader)
            sprintf(log, "%s\np%d requests to enter CS at %s for %d time", log, mypid,  tim, my_req_no);
        else
            sprintf(log, "%s\nLeader l%d requests to enter CS at %s for %d time", log, mypid,  tim, my_req_no);
        mtx.lock();
        if(am_i_leader){
            if(mtx5.try_lock()){
                enqueue(mypid);
                std::thread request_processor(processRequests);
                request_processor.detach();
            }
            else{
                enqueue(mypid);
            }
        }
        else{
            smsgtype = 11;
            sreq.did = my_leader;
            sreq.sid = mypid;
            sendMessage();
        }
        mtx.unlock();
        mtx4.lock();
        executeCS();
        executing_cs = 0;
        calcResponseTime();
        if(!am_i_leader){
            mtx.lock();
            release.sid = mypid;
            release.did = my_leader;
            smsgtype = 13;
            sendMessage();
            mtx.unlock();
        }
    }
    terminated[mypid] = 1;
    mtx.lock();
    for(int i=0;i<nt;i++){
        if(i!=mypid){
            smsgtype = 2;
            sreq.sid = mypid;
            sreq.did = i;
            sendMessage();
        }
    }
    mtx.unlock();
    std::thread check_terminate(checkTerminate);
    check_terminate.join();
    std::this_thread::sleep_for(std::chrono::seconds(mypid));
    sendLog();
    cout << "All tasks completed..." << endl;
}


//The main function simply spawns various threads
//and waits for them to finish.
int main(){
    pthread_t threads[3];
    int sthread, lthread, tid[2] = {0, 1}, w = 0, ch;
    char temp[1024];
    for (int i = 0; i < MAXCL; i++)clientList[i] = 0, clientsocks[i] = 0;
    ne++;
    srand(time(0));
    mtx1.lock();
    mtx2.lock();
    mtx3.lock();
    mtx4.lock();
    mtxchild.lock();
    lthread = pthread_create(&threads[0], NULL, server, (void *)&tid[0]);
    sthread = pthread_create(&threads[1], NULL, getPid, (void *)&tid[1]);
    std::thread autow(autoWork);
    autow.join();
}
