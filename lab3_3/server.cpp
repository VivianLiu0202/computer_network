// server.cpp
#include "common.h"
using namespace std;

int ClientSeq = 0;
int ServerSeq = 0;
const uint32_t ROUTER_PORT = 33333; // 路由器端口号
const uint32_t SERVER_PORT = 11111; // 服务器

int DisorderPacketNum = 0;//lab3_3新增，标定一个窗口内乱序到达的包的数量
char *fileBufferCopy = new char[50 * MAX_PACKET_SIZE]; // lab3_3新增，用于暂存乱序到达的包的数据

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

bool ServerReceivePacket(Packet& recvPacket, SOCKET serverSocket, SOCKADDR_IN clientAddr, int& expectSeq)
{
	int AddrLen = sizeof(clientAddr);
	while (1)
	{
		int recvSize = recvfrom(serverSocket, (char*)&recvPacket, sizeof(recvPacket), 0, (sockaddr*)&clientAddr, &AddrLen);
		if (recvSize > 0)
		{
			//成功收到消息
			if (recvPacket.Check() && (recvPacket.seq_no == expectSeq))
			{
				//回复ACK
				Packet sendPacket;
				sendPacket.ScrPort = SERVER_PORT;
				sendPacket.DestPort = ROUTER_PORT;
				sendPacket.type += ACK;
				sendPacket.ack_no = recvPacket.seq_no;

				sendPacket.setChecksum();
				sendto(serverSocket, (char*)&sendPacket, sizeof(sendPacket), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "\nServer收到 Seq = " << recvPacket.seq_no << "的报文段，并回复 ack = " << sendPacket.ack_no << " 的回复报文段" << endl;
				expectSeq++;
                
                if(DisorderPacketNum > 0){
                    expectSeq+=DisorderPacketNum;
                }
                cout<<"expectSeq="<<expectSeq<<endl;
				return true;
			}

			//如果seq！= 期待值，则返回
			else if (recvPacket.Check() && (recvPacket.seq_no > expectSeq))
			{
                DisorderPacketNum++;
				//回复ACK
				Packet sendPacket;
				sendPacket.ScrPort = SERVER_PORT;
				sendPacket.DestPort = ROUTER_PORT;
				sendPacket.type += ACK;
				sendPacket.ack_no = recvPacket.seq_no;

				sendPacket.setChecksum();
				sendto(serverSocket, (char*)&sendPacket, sizeof(sendPacket), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "\n【失序！】server收到 Seq = " << recvPacket.seq_no << "的报文段，并发送 Ack = " << sendPacket.ack_no << " 的回复报文段" << endl;

                //Todo:这里写一个小缓冲区存储失序的报文段
                for(int i=0;i<MAX_PACKET_SIZE;i++){
                    fileBufferCopy[(DisorderPacketNum)*MAX_PACKET_SIZE+i] = recvPacket.data[i];
                }
			}
		}
		else if (recvSize == 0)
			return false;
	}
}

void ServerReceiveFile(SOCKET serverSocket, SOCKADDR_IN clientAddr)
{
	int expectSeq = 0;
	int AddrLen = sizeof(clientAddr);
	//======================接收文件名和文件大小=====================
	Packet recvInfoPacket;
	unsigned int FileSize;//文件大小
	char FileName[50];//文件名
	while (1)
	{
		int recvSize = recvfrom(serverSocket, (char*)&recvInfoPacket, sizeof(recvInfoPacket), 0, (sockaddr*)&clientAddr, &AddrLen);
		if (recvSize > 0){
			//成功收到消息且为expectSeq
			if (recvInfoPacket.Check() && (recvInfoPacket.seq_no == expectSeq)){
				FileSize = recvInfoPacket.length;//获取文件大小
                for (int i = 0; i < 1000; i++){
                    if (recvInfoPacket.data[i] != '\0'){
                        FileName[i] = recvInfoPacket.data[i];
                    }
                    else{
                        FileName[i] = recvInfoPacket.data[i];
                        break;
                    }
                }
				cout<<"\n接受文件名为："<<FileName<<",大小为"<<FileSize<<"字节"<<endl;
                expectSeq = recvInfoPacket.seq_no;

				//回复ACK
				Packet sendPacket;
				sendPacket.ScrPort = SERVER_PORT;
				sendPacket.DestPort = ROUTER_PORT;
				sendPacket.type += ACK;
				sendPacket.ack_no = recvInfoPacket.seq_no;//确认expectSeq

				sendPacket.setChecksum();
				sendto(serverSocket, (char*)&sendPacket, sizeof(sendPacket), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "Server收到 seq = " << recvInfoPacket.seq_no << "的报文段，并发送 ack = " << sendPacket.ack_no << " 的回复报文段" << endl;
				expectSeq++;
				break;
			}

			// 如果接收到的序号不是期望的序号
			else if (recvInfoPacket.Check() && (recvInfoPacket.seq_no != expectSeq)){
				//回复ACK
				Packet sendPacket;
				sendPacket.ScrPort = SERVER_PORT;
				sendPacket.DestPort = ROUTER_PORT;
				sendPacket.type += ACK;
                sendPacket.type += REPEAT;
				sendPacket.ack_no = recvInfoPacket.seq_no; // 确认号真实序列号

				sendPacket.setChecksum();
				sendto(serverSocket, (char*)&sendPacket, sizeof(sendPacket), 0, (sockaddr*)&clientAddr, sizeof(SOCKADDR_IN));
				cout << "【重复接受报文段】Server收到 seq = " << recvInfoPacket.seq_no << "的报文段，并发送 ack = " << sendPacket.ack_no << " 的回复报文段" << endl;
			}
		}
	}

    cout<<">>>>>>>>>>>>>>成功接受文件信息<<<<<<<<<<<<<<<"<<endl;
	//==========================接收数据段==================================
	int FillCount = FileSize / MAX_PACKET_SIZE;//全装满的报文个数
	int LeaveSize = FileSize % MAX_PACKET_SIZE;//不能装满的剩余报文大小
	char* fileBuffer = new char[FileSize];

	cout<<"开始接受文件内容，【最大装载报文段】为"<<FillCount<<"个"<<endl;

	for (int i = 0; i < FillCount; i++)
	{
		Packet dataPacket;
		if (ServerReceivePacket(dataPacket, serverSocket, clientAddr, expectSeq)){
            cout << "<==========第 " << i + 1 << " 个最大装载报文段==========>" << endl;
            cout<<"接受数据包("<<dataPacket.seq_no<<")成功!"<<endl<<endl;
		}
		else{
			cout<<"接受数据包失败!"<<endl<<endl;
			return;
		}
		//读取数据部分
		for (int j = 0; j < MAX_PACKET_SIZE; j++)
		{
			fileBuffer[i * MAX_PACKET_SIZE + j] = dataPacket.data[j];
		}
        if(DisorderPacketNum > 0){
            //把备用缓冲区中的数据拷贝到主缓冲区
            cout << "发现" << DisorderPacketNum << "个失序报文段，正在重组......" << endl;
            for (int j = 1; j <= DisorderPacketNum; j++)
            {
                for (int k = 0; k < MAX_PACKET_SIZE; k++)
                {
                    fileBuffer[i * MAX_PACKET_SIZE + j * MAX_PACKET_SIZE + k] = fileBufferCopy[j * MAX_PACKET_SIZE + k];
                }
            }
            i += DisorderPacketNum; // 调整循环控制变量
            DisorderPacketNum = 0;  // 清空备用缓冲区
        }
	}

	//剩余部分
	if (LeaveSize > 0){
        cout<<"开始接受剩余的数据包"<<endl;
		Packet dataPacket;
		if (ServerReceivePacket(dataPacket, serverSocket, clientAddr, expectSeq)){
			cout<<"接受数据包("<<dataPacket.seq_no<<")成功!"<<endl<<endl;
		}
		else{
            cout<<"接受数据包失败!"<<endl<<endl;
            return;
		}
		for (int j = 0; j < LeaveSize; j++){
			fileBuffer[FillCount * MAX_PACKET_SIZE + j] = dataPacket.data[j];
		}

	}

	cout << "\n\n文件传输成功，正在写入文件......" << endl;
    //写入文件
    string FileName_string=string(FileName);
    FileName_string = "server_"+FileName_string;
    FILE* fp = fopen(FileName_string.c_str(),"wb");
    if(fp == NULL){
        cout<<"写入文件失败!"<<endl;
        return ;
    }
    if(fileBuffer){
        fwrite(fileBuffer,1,FileSize,fp);
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
    ServerReceiveFile(serverSocket, clientAddr);

    //断开连接
    std::srand(time(nullptr)); //随机设置挥手序列号
    ServerSeq = std::rand() % 10000;
    Close_Connect_Client(serverSocket,clientAddr);

    //关闭socket
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
