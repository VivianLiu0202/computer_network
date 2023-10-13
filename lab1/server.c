//多线程的聊天服务器
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

const int PORT=8080; // 服务器监听的端口号
#define BUFFER_SIZE 2048 //消息缓冲区大小
//缓冲区用来存放客户端发来的消息，每个客户端对应一个缓冲区
#define MAX_CLIENTS 4 //最大客户端数量

//定义了服务器的socket、新连接的客户端的socket
//两个地址结构体来存储服务器和客户端的地址信息。
int server_socket, new_socket, *new_sock;
struct sockaddr_in server_addr, client_addr;
socklen_t addr_size;

int client_sockets[MAX_CLIENTS];// client_sockets存储所有连接的客户端的套接字
int client_count = 0;//使用client_count来跟踪当前连接的客户端数量
int client_sockets_status[MAX_CLIENTS];//记录当前的socket数组位置是否为空

int client_status(){
    for(int i=0;i<MAX_CLIENTS;i++){
        if(client_sockets_status[i] == 0) return i;
    }
    return -1;
}

void add_info(char* message){
    time_t rawtime;
    struct tm * timeinfo;
    char time_stamp[80];

    //获取当前时间
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    //格式化时间
    strftime(time_stamp, sizeof(time_stamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    //添加时间戳
    strcat(time_stamp, ": ");
    strcat(time_stamp, "\n");
    strcat(time_stamp, message);

    //将带有时间戳的消息复制回原来的消息变量
    strcpy(message, time_stamp);
}

//一个线程函数，用于处理客户端的消息
void *handle_client(void *client_socket) 
{
    int sock = *(int *)client_socket; //获取客户端的socket
    char Recvbuffer[BUFFER_SIZE]; //接受消息的缓冲区
    char Sendbuffer[BUFFER_SIZE]; //发送消息的缓冲区
    int read_size; //存储接收到的消息的大小

    while((read_size=recv(sock, Recvbuffer, BUFFER_SIZE, 0))>0)
    {
        //接收客户端的消息并转发给其他客户端
        Recvbuffer[read_size] = '\0';//消息字符串以空字符结尾
        //如果客户端发来的消息是EXIT，则退出
        if (strcmp(Recvbuffer, "EXIT") == 0) {
            break;
        }
        //将消息添加时间戳
        add_info(Recvbuffer);
        printf("client %d: %s \n", sock ,Recvbuffer);
        sprintf(Sendbuffer, "client %d: %s", sock,Recvbuffer);

        //====================== 向其他socket都发送消息 =====================
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets_status[i] == 1 && client_sockets[i] != sock) {
                int send_result = send(client_sockets[i], Sendbuffer, strlen(Sendbuffer), 0);
                if (send_result <= 0) { // 如果发送失败，更新客户端sockets的状态
                    printf("Client %d disconnected unexpectedly.\n", client_sockets[i]);
                    client_sockets_status[i] = 0;
                    client_sockets[i] = 0;
                    client_count--;
                }
            }
        } 
    }     
    if(strcmp(Recvbuffer, "EXIT") == 0 || read_size == 0){
        for(int i=0;i<MAX_CLIENTS;i++){
            if(client_sockets[i]==sock) {
                printf("Client %d exit, number of current clients: %d.\n", sock, client_count-1);
                client_sockets_status[i]=0;
                client_sockets[i]=0;
                client_count=client_count-1;

                //关闭客户端的socket，释放分配的内存，并结束线程
                close(sock);
                free(client_socket);
                return NULL;
            }
        }
    }
    if(read_size == -1)
    {
        perror("recv failed");
        close(sock);
        free(client_socket);
        return NULL;
    }
    return NULL;
}

int main() {
    printf("服务器启动\n");

    //创建一个新的socket来监听客户端的连接请求。如果创建失败，则打印错误消息并退出程序。
    server_socket = socket(AF_INET, SOCK_STREAM, 0);//创建一个TCP套接字,AF_INET表示IPv4
    if (server_socket == -1) {
        perror("创建socket: failed!\n");
        exit(EXIT_FAILURE);
    }
    printf("创建socket: success!\n");

    //=====================初始化服务器地址============================
    //设置服务器地址结构体的属性。这里，服务器监听所有可用的接口上的指定端口。
    server_addr.sin_family = AF_INET;//AF_INET表示IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;//INADDR_ANY表示服务器监听所有可用的接口上的指定端口
    server_addr.sin_port = htons(PORT);//htons()函数将主机字节序转换为网络字节序,PORT为服务器监听的端口号,htons(PORT)是将PORT转换为网络字节序

    //=====================bind=============================
    //将socket绑定到指定的地址和端口。如果绑定失败，则打印错误消息并退出程序
    int Bindtemp = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (Bindtemp < 0) {
        perror("Bind: failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Bind: bind to port %d success!\n",PORT);

    //=========================listen=========================
    //开始监听客户端的连接请求，并打印一条消息表示服务器已启动。
    int ListenTemp = listen(server_socket, MAX_CLIENTS);
    if(ListenTemp != 0){
        perror("listen: failed!\n");
        exit(EXIT_FAILURE);
    }
    printf("listen: success!\n");
    printf("Server started. Waiting for clients connections...\n\n");

    //=========================接收client=======================
    //设置addr_size为sockaddr_in结构体的大小，这将在接受新的客户端连接时使用
    addr_size = sizeof(struct sockaddr_in);
    //用于持续接受来自client的连接请求。当有新的client尝试连接时，accept函数会返回一个新的socket
    while ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size))) {
        if (client_count >= MAX_CLIENTS) {//如果客户端数量超过最大值，则拒绝连接
            printf("连接失败: client数量达到最大 \n");
            close(new_socket);
            continue;
        }
        
        int temp = client_status();//返回数组中的空位；
        client_sockets_status[temp] = 1;
        client_sockets[temp] = new_socket;
        client_count++;
        //打印一条消息，显示已接受的客户端的IP地址和端口号。
        printf("成功连接 %s:%d clients(%d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),new_socket);
        printf("Now the number of clients is: %d\n",client_count);

        //新建线程，并为新的socket分配内存
        pthread_t client_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = new_socket;

        //创建一个新的线程来处理新的客户端的消息。如果创建线程失败，则打印错误消息并退出程序。
        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("创建线程: failed!\n");
            exit(EXIT_FAILURE);
        }
        printf("创建线程: success!\n");
    }

    //检查accept函数是否出现错误。如果出现错误，则打印错误消息并退出程序
    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    close(server_socket);

    return 0;
}
