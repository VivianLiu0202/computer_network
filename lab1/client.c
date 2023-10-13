#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

//定义了服务器的端口号和缓冲区大小
const int PORT= 8080;
#define BUFFER_SIZE 2048

//为接收服务器消息的线程设计的。它接受一个socket作为参数。
void *receive_messages(void *socket) {
    int sock = *(int *)socket;
    while(1)
    {
        char buffer[BUFFER_SIZE];
        int read_size=recv(sock, buffer, BUFFER_SIZE, 0);
        if(read_size<=0)//如果接收到的字符数小于0，就退出
        {
            perror("与服务器断开连接\n");
            exit(EXIT_FAILURE);
            break;
        }
        else{//如果接收到的字符数大于0，就打印出来
            buffer[read_size] = '\0';
            printf("%s\n", buffer);
        }
    }
    return NULL;
}

int main() {
    //定义客户端client，服务器地址结构和消息缓冲区
    int client_socket;
    struct sockaddr_in server_addr;

    printf("客户端启动\n");

    ////==================创建客户端的socket=======================
    //创建一个新的socket。如果创建失败，打印错误消息并退出程序
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("创建socket: failed\n");
        return -1;
    }
    printf("创建socket: success\n");

    //=========================服务器地址=========================
    //设置服务器地址结构。这里，客户端尝试连接到本地地址127.0.0.1和之前定义的端口号
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    //=========================连接到服务器=========================
    //连接到服务器。如果连接失败，打印错误消息并退出程序
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect: failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Connect to the server.Type messages...\n");

    //=========================创建线程=========================
    //创建一个新的线程来接收服务器的消息。如果创建线程失败，打印错误消息并退出程序。
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&client_socket) < 0) {
        perror("创建线程: failed\n");
        exit(EXIT_FAILURE);
    }
    printf("创建线程: success\n");

    //=========================发送消息=========================
    //持续从标准输入读取消息并发送给服务器，如果输入EXIT就退出循环
    while (1) {
        char buffer[BUFFER_SIZE] = {0};
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline
        send(client_socket, buffer, strlen(buffer), 0);
        if (strcmp(buffer, "EXIT") == 0) {
            printf("退出聊天室\n");
            break;
        }
    }

    close(client_socket);
    return 0;
}
