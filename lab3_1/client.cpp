// client.cpp
#include "common.h"
using namespace std;

const uint32_t ROUTER_PORT = 30000; // 路由器端口号

const uint32_t CLIENT_PORT = 20000; // client端口号

int ClientSeq = 0;
int ServerSeq = 0;
Packet prePacket;
int totalTimeOut = 0;



//实现三次握手 ~
bool Connect_Server(SOCKET &clientSocket, sockaddr_in &serverAddr){
    int AddrLen = sizeof(serverAddr);
    cout<<"-----------------开始三次握手-----------------"<<endl;
    Packet Packet1;
    Packet Packet2;
    Packet Packet3;

    //第一次握手 SYN=1 seq=x
    cout<<"----------------开始第一次握手-----------------"<<endl;
    Packet1.ScrPort = CLIENT_PORT;
    Packet1.DestPort = ROUTER_PORT;
    Packet1.type = SYN;
    Packet1.seq_no = ClientSeq;
    Packet1.ack_no = 0;

    Packet1.setChecksum();
    cout<<sizeof(Packet1)<<endl;
    int sendSize = sendto(clientSocket, (char *)(&Packet1), sizeof(Packet1), 0, (SOCKADDR *)&serverAddr, AddrLen);
    if (sendSize <= 0) {
        int error = WSAGetLastError();
        cout << "发送失败，错误代码：" << error << endl;
        return false;
    }
    cout<<"Client发送第一次握手:SYN!"<<endl;

    //接收第二次握手 SYN=1 ACK=1 ack=x
    clock_t start1 = clock();
    cout<<"----------------接受第二次握手-----------------"<<endl;
    int ReSendCount = 0;
    while(1){
        int recvSize = recvfrom(clientSocket, (char *)(&Packet2), sizeof(Packet2), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if (recvSize > 0) {
            if((Packet2.type & ACK) && (Packet2.type & SYN) && Packet2.ack_no == Packet1.seq_no+1 && Packet2.Check()){
                cout<<"Client收到第二次握手:ACK,SYN"<<endl;
                ServerSeq = Packet2.seq_no;
                cout<<"Server的seq_no="<<ServerSeq<<endl;
                break;
            }
            else{
                if(!(Packet2.type & ACK) || !(Packet2.type & SYN)){
                    cout<<"第二次握手标记错没有ACK/SYN"<<endl;
                    return false;
                }
                else if(Packet2.ack_no != Packet1.seq_no+1){
                    cout<<"第二次握手序列号错误"<<endl;
                    return false;
                }
                else if(!Packet2.Check()){
                    cout<<"第二次握手校验和错误"<<endl;
                    return false;
                }
            }
        }

        //超时处理，重新发送数据包
        else if(clock() - start1 > TIMEOUT_MILLISECONDS){
            if(ReSendCount == MAX_REPEAT_TIMES){
                cout<<"第二次握手超时,重传次数超过"<<MAX_REPEAT_TIMES<<"次，连接失败！"<<endl;
                return false;
            }
            ReSendCount++;
            cout<<"第二次握手超时，重新发送第一次握手数据包......"<<endl;
            int SendSize = sendto(clientSocket, (char *)(&Packet1), sizeof(Packet1), 0, (SOCKADDR*)&serverAddr, AddrLen);
            start1 = clock();
            if (SendSize <=0) {
                cout<<"第一次握手再次发送失败！"<<endl;
                return false;
            }
            else{
                cout<<"第一次握手再次发送成功！"<<endl;
                continue;
            }
        }

    }

    //第三次握手 ACK=1 seq=x+1
    cout<<"----------------开始第三次握手-----------------"<<endl;
    Packet3.ScrPort = CLIENT_PORT;
    Packet3.DestPort = ROUTER_PORT;
    Packet3.type = ACK;
    Packet3.seq_no = Packet1.seq_no+1;
    Packet3.ack_no = Packet2.seq_no+1;

    Packet3.setChecksum();
    int SendSize = sendto(clientSocket, (char *)(&Packet3), sizeof(Packet3), 0, (SOCKADDR*)&serverAddr, AddrLen);
    if (SendSize<=0) {
        cout<<"第三次握手消息发送失败"<<endl;
        return false;
    }
    cout<<"Client发送第三次握手:ACK!"<<endl;
    cout<<"-----------------三次握手成功-----------------"<<endl;
    return true;
}

//实现四次挥手~
bool Close_Connect_Server(SOCKET &clientSocket, sockaddr_in &serverAddr){
    int AddrLen = sizeof(serverAddr);
    cout<<"-----------------开始四次挥手-----------------"<<endl;
    Packet Packet1;
    Packet Packet2;
    Packet Packet3;
    Packet Packet4;

    //第一次挥手 FIN=1 seq=y
    cout<<"----------------开始第一次挥手-----------------"<<endl;
    Packet1.ScrPort = CLIENT_PORT;
    Packet1.DestPort = ROUTER_PORT;
    Packet1.type = FIN;
    Packet1.seq_no = ClientSeq;
    Packet1.ack_no = 0;

    Packet1.setChecksum();
    int sendSize1 = sendto(clientSocket, (char *)(&Packet1), sizeof(Packet1), 0, (SOCKADDR*)&serverAddr, AddrLen);
    clock_t start1 = clock();
    if (sendSize1 <= 0) {
        cout<<"第一次挥手消息发送失败！"<<endl;
        return false;
    }
    cout<<"Client发送第一次挥手消息:FIN!"<<endl;

    //接受第二次挥手 等待服务器回复 ack=y ACK=1
    int ReSendCount = 0;
    cout<<"----------------接受第二次挥手-----------------"<<endl;
    while(1){
        int recvSize2 = recvfrom(clientSocket, (char *)(&Packet2), sizeof(Packet2), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if (recvSize2 > 0) {
            if((Packet2.type & ACK) && Packet2.ack_no == Packet1.seq_no+1 && Packet2.Check()){
                cout<<"Client接受第二次挥手消息:ACK!"<<endl;
                cout<<"第二次挥手成功！"<<endl;
                break;
            }
            else{
                if(!(Packet2.type & ACK)){
                    cout<<"第二次挥手标记没有ACK"<<endl;
                    return false;
                }
                else if(Packet2.ack_no != Packet1.seq_no+1){
                    cout<<"第二次挥手序列号错误"<<endl;
                    return false;
                }
                else if(!Packet2.Check()){
                    cout<<"第二次挥手校验和错误"<<endl;
                    return false;
                }
            }
        }
        //超时处理，重新发送数据包
        if(clock() - start1 > TIMEOUT_MILLISECONDS){
            if(ReSendCount == MAX_REPEAT_TIMES){
                cout<<"第二次挥手超时,重传次数超过"<<MAX_REPEAT_TIMES<<"次，连接失败！"<<endl;
                return false;
            }
            ReSendCount++;
            cout<<"第二次挥手超时,重新发送第一次挥手数据包......"<<endl;
            int SendSize = sendto(clientSocket, (char *)(&Packet1), sizeof(Packet1), 0, (SOCKADDR*)&serverAddr, AddrLen);
            start1 = clock();
            if (SendSize <=0) {
                cout<<"第一次挥手再次发送失败！"<<endl;
                return false;
            }
            else{
                cout<<"第一次挥手再次发送成功！"<<endl;
                continue;
            }
        }
    }

    //第三次挥手 接受服务器数据包 FIN=1 ACK=1 seq=z
    cout<<"----------------接受第三次挥手-----------------"<<endl;
    while(1){
        int recvSize3 = recvfrom(clientSocket, (char *)(&Packet3), sizeof(Packet3), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if(recvSize3 <= 0){
            cout<<"接受第三次挥手失败！"<<endl;
            return false;
        }
        else if (recvSize3 > 0) {
            if((Packet3.type & FIN) && (Packet3.type & ACK) && Packet3.Check() && Packet3.ack_no == Packet2.ack_no){
                cout<<"Client接受第三次挥手消息:FIN,ACK!"<<endl;
                break;
            }
            else{
                if(!(Packet3.type & FIN) || !(Packet3.type & ACK)){
                    cout<<"第三次挥手标记没有FIN/ACK"<<endl;
                    return false;
                }
                else if(Packet3.ack_no != Packet2.ack_no){
                    cout<<"第三次挥手序列号错误"<<endl;
                    return false;
                }
                else if(!Packet3.Check()){
                    cout<<"第三次挥手校验和错误"<<endl;
                    return false;
                }
            }
        }
    }

    cout<<"----------------发送第四次挥手-----------------"<<endl;
    //第四次挥手 ACK=1 seq=z+1
    Packet4.ScrPort = CLIENT_PORT;
    Packet4.DestPort = ROUTER_PORT;
    Packet4.type = ACK;
    Packet4.seq_no = Packet3.ack_no;
    Packet4.ack_no = Packet3.seq_no+1; //这里的ack_no是上一个数据包的seq_no+1
    Packet4.setChecksum();

    int sendSize4 = sendto(clientSocket, (char *)(&Packet4), sizeof(Packet4), 0, (SOCKADDR*)&serverAddr, AddrLen);
    if (sendSize4 <= 0) {
        cout<<"第四次挥手失败！"<<endl;
        return false;
    }
    cout<<"Client发送第四次挥手消息:ACK!"<<endl;

    //等待服务器进入2MSL状态
    int tclock = clock();
    cout<<"client wait 2MSL~"<<endl;
    Packet PacketTemp;
    while(clock() - tclock < 2*TIMEOUT_MILLISECONDS){
        int recvSize = recvfrom(clientSocket, (char *)(&PacketTemp), sizeof(PacketTemp), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if(recvSize == 0){
            cout<<"关闭连接失败！"<<endl;
            return false;
        }
        else if (recvSize > 0) {
            int sendSize= sendto(clientSocket, (char *)(&Packet4), sizeof(Packet4), 0, (SOCKADDR*)&serverAddr, AddrLen);
            cout<<"重新回复ack"<<endl;
        }
    }
    cout<<"-----------------四次挥手成功-----------------"<<endl;
    return true;
}

//发送数据包
int ClientSendAndRecvPacket(Packet& sendPacket,SOCKET clientSocket, SOCKADDR_IN serverAddr){
    int AddrLen = sizeof(serverAddr); //地址长度
    int sendSize = sendto(clientSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN));
    if (sendSize <= 0) {
        int error = WSAGetLastError();
        cout << "发送失败，错误代码：" << error << endl;
        return false;
    }
    cout<<endl<<"Client已发送 seq= "<<sendPacket.seq_no<<" ack= "<<sendPacket.ack_no<<" length= "<<sendPacket.length<<"的数据包"<<endl;
    int TimesOfTimeOut = 0; //超时次数
    clock_t start = clock();

    Packet RecvPacket;
    while(1){
        int recvSize = recvfrom(clientSocket, (char *)(&RecvPacket), sizeof(RecvPacket), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if (recvSize > 0) {
            // cout<<"recvPacket.flag="<<RecvPacket.type<<endl;
            // cout<<"recvPacket.ack_no="<<RecvPacket.ack_no<<endl;
            // cout<<"recvPacket.seq_no + recvPacket.length="<<RecvPacket.seq_no+RecvPacket.length<<endl;

            if((RecvPacket.type & REPEAT) && (RecvPacket.type & ACK)){
                cout<<"收到序列号错误数据包，可能为重复数据包"<<endl;
                continue;
            }

            else if((RecvPacket.type & ACK) && RecvPacket.ack_no == sendPacket.seq_no+sendPacket.length){
                cout<<"Client已收到 seq= "<<std::dec <<RecvPacket.seq_no<<" ack= "<<RecvPacket.ack_no<<" length= "<<RecvPacket.length<<"的ACK报文"<<endl;
                return RecvPacket.seq_no;
            }

        }
        //超时处理
        else if(clock() - start > TIMEOUT_MILLISECONDS){
            if(TimesOfTimeOut >= MAX_REPEAT_TIMES){
                cout<<"超时重传【"<<MAX_REPEAT_TIMES<<"】次。传输失败！"<<endl;
                return -1;
            }
            TimesOfTimeOut++;
            totalTimeOut++;
            cout<<"【超时重传】序列号为 seq= "<<sendPacket.seq_no<<"的报文超时,重新发送"<<endl;;
            sendto(clientSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN));
            start = clock();
        }
        else continue;
    }
}

//传输文件
void ClientSendFile(string filename,SOCKET clientSocket, SOCKADDR_IN serverAddr){
    //打开文件
    ifstream file(filename,ios::in|ios::binary);
    if(!file.is_open()){
        cout<<"文件打开失败！"<<endl;
        return ;
    }
    cout<<"文件打开成功！"<<endl;

    //获取文件大小
    file.seekg(0,ios::end);
    int FileSize = file.tellg();
    file.seekg(0,ios::beg);
    cout<<"文件大小为："<<FileSize<<"B"<<endl;

    //读取文件内容
    char *filebuf = new char[FileSize];
    if (filebuf == NULL)
    {
        std::cerr << "内存分配失败" << std::endl;
        file.close();
        return;
    }

    file.read(filebuf,FileSize);
    file.close();
    cout<<"文件读取成功！"<<endl;

    string FileSize_str=std::to_string(FileSize);

    //获取文件名
    string FILENAME;
    int i;
    for(i=filename.length()-1;i>=0;i--){
        if(filename[i] == '\\') break;
    }
    for(int j=i+1;j<filename.length();j++){
        FILENAME += filename[j];
    }
    cout<<"文件名为："<<FILENAME<<endl;

    int start = clock();

    //发送文件名和文件大小
    cout<<"----------------开始发送文件"<<FILENAME<<"的信息--------------"<<endl;
    totalTimeOut = 0;
    Packet FileInfoPacket;
    FileInfoPacket.ScrPort = CLIENT_PORT;
    FileInfoPacket.DestPort = ROUTER_PORT;
    //FileInfoPacket.length = FILENAME.length()+1; //补充结尾符号
    FileInfoPacket.length = FileSize; //补充结尾符号
    FileInfoPacket.type = INFO;
    FileInfoPacket.seq_no = ++ClientSeq;


    for(int i=0;i<FILENAME.length();i++){
        FileInfoPacket.data[i] = FILENAME[i];
    }
    FileInfoPacket.data[FILENAME.length()] = '\0';

    FileInfoPacket.setChecksum();

    int sendSize1 = ClientSendAndRecvPacket(FileInfoPacket,clientSocket,serverAddr);
    if(sendSize1 == -1){
        cout<<"文件信息发送失败"<<endl;
        return ;
    }
    cout<<">>>>>>>>>>>>>>成功发送文件信息<<<<<<<<<<<<<<<"<<endl;

    //发送文件内容
    cout<<"-----------------开始发送文件内容-------------------"<<endl;
    int FillCount = FileSize / MAX_PACKET_SIZE; //全部装满的报文个数
    int LeaveSize = FileSize % MAX_PACKET_SIZE; //剩余报文大小

    prePacket = FileInfoPacket; //记录上一个发送的数据包
    int preSeq = sendSize1;

    for(int i=0;i<FillCount;i++){
        Packet ContentPacket;
        ContentPacket.ScrPort = CLIENT_PORT;
        ContentPacket.DestPort = ROUTER_PORT;
        ContentPacket.seq_no = prePacket.seq_no+prePacket.length;
        ContentPacket.length = MAX_PACKET_SIZE;
        ContentPacket.type = DATA;

        for(int k=0;k<MAX_PACKET_SIZE;k++){
            ContentPacket.data[k] = filebuf[i*MAX_PACKET_SIZE+k];
        }

        ContentPacket.setChecksum();
        int sendSize2 = ClientSendAndRecvPacket(ContentPacket,clientSocket,serverAddr);
        if(sendSize2<0){
            cout<<"文件数据发送失败"<<endl;
            return ;
        }
        cout<<"成功发送并确认第"<<i+1<<"个最大装载报文段,序列号 seq= "<<ContentPacket.seq_no<<",数据长度 length= "<<ContentPacket.length<<endl;
        prePacket = ContentPacket;
        preSeq = sendSize2;

    }
    cout<<endl<<"成功发送"<<FillCount<<"个数据包,即将传输剩余部分数据包"<<endl;
    if(LeaveSize != 0){
        Packet ContentPacket;
        ContentPacket.ScrPort = CLIENT_PORT;
        ContentPacket.DestPort = ROUTER_PORT;
        ContentPacket.seq_no = prePacket.seq_no+prePacket.length;
        ContentPacket.ack_no = preSeq+prePacket.length;
        ContentPacket.length = LeaveSize;
        ContentPacket.type = DATA;

        for(int k=0;k<LeaveSize;k++){
            ContentPacket.data[k] = filebuf[FillCount*MAX_PACKET_SIZE+k];
        }

        ContentPacket.setChecksum();
        int sendSize3 = ClientSendAndRecvPacket(ContentPacket,clientSocket,serverAddr); 
        if(sendSize3<0){
            cout<<"文件数据发送失败"<<endl;
            return ;
        }
        cout<<"成功发送最后一个数据包"<<endl;
        prePacket = ContentPacket;
        preSeq = sendSize3;
    }

    delete[] filebuf;

    //计算传输时间与吞吐率
    int end = clock();
    cout<<endl<<"-----------------传输情况总结---------------"<<endl;
    double time = (double)(end-start)/CLOCKS_PER_SEC;
    cout<<"传输时间："<<time<<"s"<<endl;
    float throughput = (float)FileSize/time;
    cout<<"吞吐率："<<throughput<<"B/s = "<<throughput/1024<<"KB/s"<<endl;
    cout<<"传输文件大小："<<FileSize<<"B = "<<FileSize/1024<<"KB"<<endl;
    cout<<"超时发送数据包："<<totalTimeOut<<endl;
    cout<<"-----------------传输情况总结---------------"<<endl;
    return ;
}

int main() {
WSADATA wsaData;
    SOCKET clientSocket;

    // 初始化Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "WSAStartup success!" << std::endl;
    }

    // ==============定义服务器的IP地址和端口号，以便客户端知道应将数据发送到哪里================

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    unsigned long on = 1;
    ioctlsocket(clientSocket, FIONBIO, &on); // 设置非阻塞

    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "socket failed!" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "socket success!" << std::endl;
    }
    // 初始化服务器地址
    // AF_INET: 表示该套接字使用IPv4地址。
    // SOCK_DGRAM: 表示该套接字是一个数据报套接字（UDP）。
    // 0: 指定协议类型。对于UDP，这里应为0
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;                          // IPv4
    serverAddr.sin_port = htons(ROUTER_PORT);                  // 端口号
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 服务器IP地址

    // 初始化客户端地址
    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;                          // 地址类型
    clientAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 地址
    clientAddr.sin_port = htons(CLIENT_PORT);                  // 端口号
    int bindtemp = bind(clientSocket, (LPSOCKADDR)&clientAddr, sizeof(clientAddr));

    std::srand(time(0)); // 设置随机数种子用于随机握手时候序列号
    ClientSeq = rand()%10000;

    cout<<"握手前的ClientSeq= "<<ClientSeq<<endl;


    //连接服务器
    int temp = Connect_Server(clientSocket,serverAddr);

    cout<<"握手后的ClientSeq= "<<ClientSeq<<endl;

    while(temp){
        cout<<"请输入文件路径："<<endl;
        string filename;
        cin>>filename;
        ClientSendFile(filename,clientSocket,serverAddr);
        cout<<"是否继续传输？(n/n)"<<endl;
        char c;
        cin>>c;
        if(c == 'n') {
           
            break;
        }
    }
    cout<<"关闭连接！"<<endl;

    //--------------四次挥手-------------------
    std::srand(time(nullptr));
    ClientSeq = rand()%10000;

    Close_Connect_Server(clientSocket,serverAddr);

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
