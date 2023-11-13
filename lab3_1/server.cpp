// server.cpp
#include "common.h"
using namespace std;

int ClientSeq = 0;
int ServerSeq = 0;

Packet preSendPacket;
Packet preRecvPacket;

const uint32_t ROUTER_PORT = 30000; // 路由器端口号

const uint32_t SERVER_PORT = 10000; // 服务器

//实现三次握手 返回true表示握手成功
bool Connect_Client(SOCKET serverSocket, sockaddr_in clientAddr){
    int AddrSize = sizeof(clientAddr);
    cout<<"-----------------开始三次握手-----------------"<<endl;
    Packet packet1;
    Packet packet2;
    Packet packet3;
    cout<<"----------------接收第一次握手-----------------"<<endl;
    while (1)
    {
        //接受第一次握手 SYN=1 seq=x
        int recvSize = recvfrom(serverSocket, (char *)&packet1, sizeof(packet1), 0, (SOCKADDR*)&clientAddr, &AddrSize);
        if(recvSize > 0){
            if((packet1.type & SYN) && packet1.Check()){
                cout<<"Server收到第一次握手:SYN"<<endl;
                //由于第一次握手，服务器收到的必然是第一次握手的数据包，根据这个数据包设置ClientSeq
                ClientSeq = packet1.seq_no;
                break;
            }
            else{
                if(!packet1.Check()) {
                    cout<<"校验和错误"<<endl;
                    return false;
                }
                if(!(packet1.type & SYN)) {
                    cout<<"数据包没有SYN"<<endl;
                    return false;
                }
            }
        }
    }

    cout<<"----------------发送第二次握手-----------------"<<endl;
    //发送第二次握手 SYN=1 ACK=1 seq=y ack=x+1
    packet2.ScrPort = SERVER_PORT;
    packet2.DestPort = ROUTER_PORT;

    packet2.type = SYN;
    packet2.type += ACK;
    packet2.seq_no = ServerSeq;
    packet2.ack_no = packet1.seq_no+1; 
    packet2.setChecksum();
    int sendSize = sendto(serverSocket, (char *)(&packet2), sizeof(packet2), 0, (SOCKADDR*)&clientAddr, AddrSize);
    clock_t start2 = clock();
    if(sendSize <= 0){
        cout<<"第二次握手消息发送失败!"<<endl;
        return false;
    }
    cout<<"Server发送第二次握手:SYN,ACK"<<endl;

    cout<<"----------------接收第三次握手-----------------"<<endl;
    //接受第三次握手 ACK=1 seq=x+1 ack=y+1
    while(1){
        int recvSize = recvfrom(serverSocket, (char *)(&packet3), sizeof(packet3), 0, (SOCKADDR*)&clientAddr, &AddrSize);
        if(recvSize>0){
            if((packet3.type & ACK) && packet3.Check() && packet3.ack_no == packet2.seq_no+1 && packet3.seq_no == packet1.seq_no+1){
                cout<<"Server收到第三次握手:ACK"<<endl;
                cout<<"-----------------三次握手成功-----------------"<<endl;
                break;
            }
            else{
                if(!packet3.Check()){
                    cout<<"校验和错误"<<endl;
                    return false;
                }
                if(!(packet3.type & ACK)) {
                    cout<<"数据包没有ACK"<<endl;
                    continue;
                }
                if(packet3.ack_no != packet2.seq_no+1){
                    cout<<"收到数据包不是第三次握手QAQ"<<endl;
                    //回复一个ACK不然客户端会被阻塞
                    Packet sendPacket;
                    sendPacket.ScrPort = SERVER_PORT;
                    sendPacket.DestPort = ROUTER_PORT;
                    sendPacket.type = ACK;
                    continue;
                }
                if(packet3.seq_no != packet1.seq_no+1) {
                    cout<<"序列号错误！"<<endl;
                    return false;
                }
            }
        }
    }
    return true;
}

//实现四次挥手
bool Close_Connect_Client(SOCKET serverSocket,SOCKADDR_IN clientAddr){
    int AddrLen = sizeof(clientAddr);
    cout<<"-----------------开始四次挥手-----------------"<<endl;
    Packet packet1;
    Packet packet2;
    Packet packet3;
    Packet packet4;

    cout<<"----------------接受第一次挥手-----------------"<<endl;
    while(1){
        //接受第一次挥手 FIN=1 seq=x
        int recvSize = recvfrom(serverSocket, (char *)(&packet1), sizeof(packet1), 0, (SOCKADDR*)&clientAddr, &AddrLen);
        if(recvSize > 0){
            if((packet1.type & FIN) && packet1.Check()){
                cout<<"Server收到第一次挥手:FIN"<<endl;

                //第二次挥手 ACK=1 seq=y ack=x+1
                cout<<"----------------发送第二次挥手-----------------"<<endl;
                packet2.ScrPort = SERVER_PORT;
                packet2.DestPort = ROUTER_PORT;
                packet2.type = ACK;
                packet2.seq_no = ServerSeq;
                packet2.ack_no = packet1.seq_no+1;

                packet2.setChecksum();
                int sendSize = sendto(serverSocket, (char *)(&packet2), sizeof(packet2), 0, (SOCKADDR*)&clientAddr, AddrLen);
                if(sendSize <= 0){
                    cout<<"第二次挥手发送失败!"<<endl;
                    return false;
                }
                cout<<"Server发送第二次挥手:ACK"<<endl;
                break;
            }
        }
    }

    //发送第三次挥手 FIN=1 ACK=1 seq=z
    cout<<"----------------发送第三次挥手-----------------"<<endl;
    //重置随机的序列号z
    std::srand(time(nullptr)); //随机握手
    ServerSeq = std::rand() % 10000;

    packet3.ScrPort = SERVER_PORT;
    packet3.DestPort = ROUTER_PORT;
    packet3.type = FIN;
    packet3.type += ACK;
    packet3.seq_no = ServerSeq;
    packet3.ack_no = packet1.seq_no+1;

    packet3.setChecksum();
    int sendSize = sendto(serverSocket, (char *)(&packet3), sizeof(packet3), 0, (SOCKADDR*)&clientAddr, AddrLen);
    if(sendSize <= 0){
        cout<<"第三次挥手发送失败!"<<endl;
        return false;
    }
    cout<<"Server发送第三次挥手:FIN,ACK"<<endl;

    //接受第四次挥手 ACK=1 seq=x+1 ack=z
    cout<<"----------------接受第四次挥手-----------------"<<endl;
    while(1){
        int recvSize = recvfrom(serverSocket, (char *)(&packet4), sizeof(packet4), 0, (SOCKADDR*)&clientAddr, &AddrLen);
        if(recvSize > 0){
            if(packet4.ack_no != packet3.seq_no+1){
                cout<<"收到数据包不是第四次挥手QAQ,可能接受到重传的第一次握手"<<endl;
                continue;
            }
            if((packet4.type & ACK) && packet4.Check() && packet4.seq_no == packet3.ack_no){
                cout<<"Server收到第四次挥手:ACK"<<endl;
                cout<<"-----------------四次挥手成功-----------------"<<endl;
                break;
            }
            else{
                if(!packet4.Check()){
                    cout<<"校验和错误"<<endl;
                    return false;
                }
                if(!(packet4.type & ACK)) {
                    cout<<"数据包没有ACK"<<endl;
                    return false;
                }
                if(packet4.seq_no != packet3.ack_no) {
                    cout<<"序列号错误！"<<endl;
                    return false;
                }
            }
        }
    }
    cout<<"Server断开连接!"<<endl;
    return true;
}

bool ServerReceivePacket(Packet& recvPacket,SOCKET serverSocket,SOCKADDR_IN clientAddr){
    int AddrLen = sizeof(clientAddr);
    while(1){
        int recvSize = recvfrom(serverSocket, (char *)(&recvPacket), sizeof(recvPacket), 0, (SOCKADDR*)&clientAddr, &AddrLen);
        if(recvSize > 0){
            // cout<<"recvPacket.seq= "<<recvPacket.seq_no<<endl;
            // cout<<"preRecvPacket.seq + preRecvPacket.length"<<preRecvPacket.seq_no + preRecvPacket.length<<endl;
            if((recvPacket.seq_no == preRecvPacket.seq_no+preRecvPacket.length) && recvPacket.Check()){
                preRecvPacket = recvPacket;
                //回复ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.ack_no = recvPacket.seq_no + recvPacket.length;
                sendPacket.length = recvPacket.length;

                sendPacket.setChecksum();
                preSendPacket = sendPacket;
                int sendSize = sendto(serverSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<endl<<"Server收到 seq= "<<recvPacket.seq_no<<" ack= "<<recvPacket.ack_no<<"的报文并回复 seq= "<<sendPacket.seq_no<<" ack= "<<sendPacket.ack_no<<"的报文"<<endl;
                return true;
            }
            // 如果seq！=期待值，则传来了重复的报文：丢弃重复报文，重传该报文的回复ACK
            // （注：这种情况是由于上一个回复ACK报文段丢失，导致发送方超时重传所致，传了重复的报文段）           
            if((recvPacket.seq_no != preRecvPacket.seq_no+preRecvPacket.length) && recvPacket.Check()){
                //回复ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.type += REPEAT;
                sendPacket.setChecksum();
                int sendSize = sendto(serverSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<"【重复报文段】Server收到seq="<<recvPacket.seq_no<<" ack= "<<recvPacket.ack_no<<"的报文并回复带有REPEAT标志的ACk回复"<<endl;
            }
        }
    }
}

void ServerReceiveFile(SOCKET serversocket,SOCKADDR_IN clientAddr){
    int AddrLen = sizeof(clientAddr);

    //接受文件名
    Packet recvInfoPacket;
    u_int FileNameSize = 0;
    u_int FileSize=0;
    char FileName[10];
    ClientSeq++; //同步初始化序列号
    while(1){
        int recvSize = recvfrom(serversocket, (char *)(&recvInfoPacket), sizeof(recvInfoPacket), 0, (SOCKADDR *)&clientAddr, &AddrLen);
        if(recvSize > 0){
            //接受文件名
            if(recvInfoPacket.seq_no==ClientSeq && recvInfoPacket.Check()){
                preRecvPacket = recvInfoPacket;

                //FileNameSize = recvInfoPacket.length;
                for (int i = 0; i < 1000; i++)
                {
                    if (recvInfoPacket.data[i] != '\0')
                    {
                        FileName[i] = recvInfoPacket.data[i];
                    }
                    else{
                        FileName[i] = recvInfoPacket.data[i];
                        break;
                    }
                }
                // FileName=string(recvInfoPacket.data,recvInfoPacket.data+FileNameSize);
                FileSize=recvInfoPacket.length;
                cout<<"接受文件名为："<<FileName<<",大小为"<<FileSize<<"字节"<<endl;


                //回复ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.ack_no = recvInfoPacket.seq_no + recvInfoPacket.length;
                sendPacket.length = recvInfoPacket.length;

                sendPacket.setChecksum();
                int sendSize = sendto(serversocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<"Server收到 seq= "<<recvInfoPacket.seq_no<<" ack= "<<recvInfoPacket.ack_no<<"的报文并回复 seq= "<<sendPacket.seq_no<<" ack= "<<sendPacket.ack_no<<"的报文"<<endl;
                break;
            }

            // 如果seq<期待值，则传来了重复的报文：丢弃重复报文，重传该报文的回复ACK
            // （注：这种情况是由于上一个回复ACK报文段丢失，导致发送方超时重传所致，传了重复的报文段）
            else if(recvInfoPacket.seq_no!=ClientSeq && recvInfoPacket.Check()){
                //回复ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.type += REPEAT;
                //sendPacket.ack_no = recvInfoPacket.seq_no + recvInfoPacket.length;
                sendPacket.setChecksum();
                int sendSize = sendto(serversocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<"【重复报文段】Server收到 seq= "<<recvInfoPacket.seq_no<<"的报文并发送 ack= "<<sendPacket.ack_no<<"的报文"<<endl;
            }
        }
    }

    cout<<">>>>>>>>>>>>>>成功接受文件信息<<<<<<<<<<<<<<<"<<endl;

    //接受文件内容
    int FillCount = FileSize / MAX_PACKET_SIZE;
    int LeaveSize = FileSize % MAX_PACKET_SIZE;
    char* filebuf = new char[MAX_BUFFER_SIZE];

    cout<<"开始接受文件内容，【最大装载报文段】为"<<FillCount<<"个"<<endl;

    for(int i=0;i<FillCount;i++){
        Packet dataPacket;
        if(ServerReceivePacket(dataPacket,serversocket,clientAddr)){
            cout<<"【"<<i<<"/"<<FillCount<<"】"<<endl;
            cout<<"接受数据包("<<dataPacket.seq_no<<")成功!"<<endl;
            for(int j=0;j<MAX_PACKET_SIZE;j++){
                filebuf[i*MAX_PACKET_SIZE+j] = dataPacket.data[j];
            }
        }
        else{
            cout<<"接受数据包失败!"<<endl<<endl;
            return;
        }
    }

    if(LeaveSize>0){
        cout<<"开始接受剩余的数据包"<<endl;
        Packet dataPacket;
        if(ServerReceivePacket(dataPacket,serversocket,clientAddr)){
            cout<<"接受数据包("<<dataPacket.seq_no<<")成功!"<<endl<<endl;
            for(int j=0;j<LeaveSize;j++){
                filebuf[FillCount*MAX_PACKET_SIZE+j] = dataPacket.data[j];
            }
        }
        else{
            cout<<"接受数据包失败!"<<endl<<endl;
            return;
        }
    }

    cout<<endl<<"接受文件内容成功!写入文件......"<<endl<<endl;
    //写入文件
    string FileName_string=string(FileName);
    FileName_string = "server_"+FileName_string;
    FILE* fp = fopen(FileName_string.c_str(),"wb");
    if(fp == NULL){
        cout<<"写入文件失败!"<<endl;
        return ;
    }
    if(filebuf){
        fwrite(filebuf,1,FileSize,fp);
        fclose(fp);
        cout<<"写入文件成功!"<<endl;
    }
    else{
        cout<<"写入文件失败!"<<endl;
        return ;
    }
}




int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    // 初始化Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    unsigned long on = 1;
    ioctlsocket(serverSocket, FIONBIO, &on); // 设置非阻塞

    if (serverSocket == INVALID_SOCKET)
    {
        cout << "创建socket：fail！\n"<< endl;
        return -1;
    }
    cout << "创建socket：success！\n"<< endl;

    // 绑定套接字
    serverAddr.sin_family = AF_INET;                          // 地址族
    serverAddr.sin_port = htons(SERVER_PORT);                  // 端口号
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // IP地址
    int temp = bind(serverSocket, (LPSOCKADDR)&serverAddr, sizeof(serverAddr));

    // 初始化客户端和路由器
    clientAddr.sin_family = AF_INET;                          // 地址族
    clientAddr.sin_port = htons(ROUTER_PORT);                  // 端口号
    clientAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // IP地址

    std::srand(time(0)); // 设置随机数种子用于随机握手时候序列号
    ServerSeq = rand() % 10000;
    //ServerSeq = 5000;

    cout<<"握手前的ServerSeq= "<<ServerSeq<<endl;

    //连接  
    Connect_Client(serverSocket,clientAddr);

    cout<<"握手后的ServerSeq= "<<ServerSeq<<endl;

    //接受文件
    ServerReceiveFile(serverSocket,clientAddr);

    //断开连接
    std::srand(time(nullptr)); //随机设置挥手序列号
    ServerSeq = std::rand() % 10000;
    Close_Connect_Client(serverSocket,clientAddr);

    //关闭socket
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
