#include <iostream>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<cstring>
#include<map>
#include<thread>
#include <vector>
#include<time.h>

#define PORT "10086"

using namespace std;

map<string,string> container;
bool editlock=false;
bool readlock=false;
vector<thread> threadPool;

void deal_thread(int sockfd){
    clock_t start,end;
    start=end=0;
    cout<<"accepted a new transaction"<<endl;
    cout<<sockfd<<endl;
    string hint="tinyDatabase v1.0\r\n";
    if(send(sockfd,hint.data(),hint.size(),0)<0){
        perror("send hint message error");
        close(sockfd);
        return ;
    }
    char buff[256];

    while(true) {
        memset(buff,0,sizeof buff);
        if (recv(sockfd, buff, sizeof buff, 0) <=0) {
            fprintf(stderr,"disconnect with client on %d\r\n",sockfd);
        }
        string command=buff;
        string message;
        command=command.substr(0,command.size()-2);
        cout<<command<<endl;
        if(command=="list") {
            while(readlock){
                chrono::milliseconds dura(50);
                this_thread::sleep_for(dura);
            };
            editlock = true;
            start=clock();
            message="";
            for (map<string, string>::iterator it = container.begin(); it != container.end(); it++) {
                message += it->first + "=>" + it->second + "\r\n";
            }
            end=clock();
                if (send(sockfd, message.data(), message.size(), 0) < 0) {
                    perror("data send error");
                    editlock = false;
                    close(sockfd);
                    return;
                }

            editlock = false;
        }
            else if (command=="status"){
            message = "total count: " + to_string(container.size()) + ", current lock status: editlock:" +
                      (editlock ? "On" : "Off") + ", readlock:" + (readlock ? "On" : "Off") + ",running threads:"+to_string(threadPool.size())+"\r\n";
            if (send(sockfd, message.data(), message.size(), 0) < 0) {
                perror("data send error");
                close(sockfd);
                return;
            }
        }
            else if(command=="time"){
            message="last query used "+to_string(end-start)+"ms\r\n";
            if (send(sockfd, message.data(), message.size(), 0) < 0) {
                perror("data send error");
                close(sockfd);
                return;
            }
        }
        else{
            string comm=command.substr(0,command.find_first_of(' '));
            if(comm=="set"){
                string content=command.substr(command.find_first_of(' ')+1);
                string key=content.substr(0,content.find_first_of(' '));
                string value=content.substr(content.find_first_of(' ')+1);
                while(editlock){
                    chrono::milliseconds dura(50);
                    this_thread::sleep_for(dura);
                };
                readlock=true;
                editlock=true;
                start=clock();
                container[key]=value;
                end=clock();
                message="set success\r\n";
                if(send(sockfd,message.data(),message.size(),0)<0){
                    perror("send error");
                    editlock=false;
                    readlock=false;
                    close(sockfd);
                    return ;
                }
                editlock=false;
                readlock=false;
            }
            else if(comm=="get"){
                string key=command.substr(command.find_first_of(' ')+1);
                while(readlock){
                    chrono::milliseconds dura(50);
                    this_thread::sleep_for(dura);
                };
                editlock=true;
                start=clock();
                map<string,string>::iterator it;
                if((it=container.find(key))==container.end()){
                    end=clock();
                    message="Error key not found\r\n";
                    if(send(sockfd,message.data(),message.size(),0)<0){
                        perror("fail to send message");
                        editlock=false;
                        close(sockfd);
                        return;
                    }
                }
                else{
                    end=clock();
                    message=it->second+"\r\n";

                    if(send(sockfd,message.data(),message.size(),0)<0){
                        perror("fail to send message\r\n");
                        editlock=false;
                        close(sockfd);
                        return;
                    }
                }
                editlock=false;
            }
            else if(comm=="del"){
                string key=command.substr(command.find_first_of(' ')+1);
                readlock=true;
                editlock=true;
                start=clock();
                map<string,string>::iterator it;
                if((it=container.find(key))==container.end()){
                    end=clock();
                    message="del error:no such key found\r\n";
                    if(send(sockfd,message.data(),message.size(),0)<0){
                        perror("fail to send message\r\n");
                        readlock=false;
                        editlock=false;
                        close(sockfd);
                        return;
                    }
                }
                else{
                    container.erase(it);
                    end=clock();
                    message="del success\r\n";
                    if(send(sockfd,message.data(),message.size(),0)<0){
                        perror("fail to send message\r\n");
                        readlock=false;
                        editlock=false;
                        close(sockfd);
                        return;
                    }
                }
                readlock=false;
                editlock=false;
            }
            else{
                cout<<"client invalid input"<<endl;
                close(sockfd);
                return;
            }
        }
    }

}

int main(int argc,char** argv) {
    pid_t pid;
    int listener;
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char buf[256];
    char mess[256];
    char port[10];
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];

    int yes = 1;
    int i, j, rv;
    struct addrinfo hints, *ai, *p;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    memset(port,0,sizeof port);
    if(argc>1)strncpy(port,argv[1],(sizeof port) -1);
    else strncpy(port,PORT,(sizeof port) -1);
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver:%s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai);
    if (listen(listener, 10) == -1)
    {
        perror("listen");
        exit(3);
    }

    for(;;){
        newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);


        threadPool.push_back(thread(deal_thread,newfd));
        if (newfd == -1)
        {
            perror("accept");
            continue;

        }

    }

    return 0;
}