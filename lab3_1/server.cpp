// server.cpp
#include "common.h"
using namespace std;

int ClientSeq = 0;
int ServerSeq = 0;

Packet preSendPacket;
Packet preRecvPacket;

const uint32_t ROUTER_PORT = 30000; // ·�����˿ں�

const uint32_t SERVER_PORT = 10000; // ������

//ʵ���������� ����true��ʾ���ֳɹ�
bool Connect_Client(SOCKET serverSocket, sockaddr_in clientAddr){
    int AddrSize = sizeof(clientAddr);
    cout<<"-----------------��ʼ��������-----------------"<<endl;
    Packet packet1;
    Packet packet2;
    Packet packet3;
    cout<<"----------------���յ�һ������-----------------"<<endl;
    while (1)
    {
        //���ܵ�һ������ SYN=1 seq=x
        int recvSize = recvfrom(serverSocket, (char *)&packet1, sizeof(packet1), 0, (SOCKADDR*)&clientAddr, &AddrSize);
        if(recvSize > 0){
            if((packet1.type & SYN) && packet1.Check()){
                cout<<"Server�յ���һ������:SYN"<<endl;
                //���ڵ�һ�����֣��������յ��ı�Ȼ�ǵ�һ�����ֵ����ݰ�������������ݰ�����ClientSeq
                ClientSeq = packet1.seq_no;
                break;
            }
            else{
                if(!packet1.Check()) {
                    cout<<"У��ʹ���"<<endl;
                    return false;
                }
                if(!(packet1.type & SYN)) {
                    cout<<"���ݰ�û��SYN"<<endl;
                    return false;
                }
            }
        }
    }

    cout<<"----------------���͵ڶ�������-----------------"<<endl;
    //���͵ڶ������� SYN=1 ACK=1 seq=y ack=x+1
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
        cout<<"�ڶ���������Ϣ����ʧ��!"<<endl;
        return false;
    }
    cout<<"Server���͵ڶ�������:SYN,ACK"<<endl;

    cout<<"----------------���յ���������-----------------"<<endl;
    //���ܵ��������� ACK=1 seq=x+1 ack=y+1
    while(1){
        int recvSize = recvfrom(serverSocket, (char *)(&packet3), sizeof(packet3), 0, (SOCKADDR*)&clientAddr, &AddrSize);
        if(recvSize>0){
            if((packet3.type & ACK) && packet3.Check() && packet3.ack_no == packet2.seq_no+1 && packet3.seq_no == packet1.seq_no+1){
                cout<<"Server�յ�����������:ACK"<<endl;
                cout<<"-----------------�������ֳɹ�-----------------"<<endl;
                break;
            }
            else{
                if(!packet3.Check()){
                    cout<<"У��ʹ���"<<endl;
                    return false;
                }
                if(!(packet3.type & ACK)) {
                    cout<<"���ݰ�û��ACK"<<endl;
                    continue;
                }
                if(packet3.ack_no != packet2.seq_no+1){
                    cout<<"�յ����ݰ����ǵ���������QAQ"<<endl;
                    //�ظ�һ��ACK��Ȼ�ͻ��˻ᱻ����
                    Packet sendPacket;
                    sendPacket.ScrPort = SERVER_PORT;
                    sendPacket.DestPort = ROUTER_PORT;
                    sendPacket.type = ACK;
                    continue;
                }
                if(packet3.seq_no != packet1.seq_no+1) {
                    cout<<"���кŴ���"<<endl;
                    return false;
                }
            }
        }
    }
    return true;
}

//ʵ���Ĵλ���
bool Close_Connect_Client(SOCKET serverSocket,SOCKADDR_IN clientAddr){
    int AddrLen = sizeof(clientAddr);
    cout<<"-----------------��ʼ�Ĵλ���-----------------"<<endl;
    Packet packet1;
    Packet packet2;
    Packet packet3;
    Packet packet4;

    cout<<"----------------���ܵ�һ�λ���-----------------"<<endl;
    while(1){
        //���ܵ�һ�λ��� FIN=1 seq=x
        int recvSize = recvfrom(serverSocket, (char *)(&packet1), sizeof(packet1), 0, (SOCKADDR*)&clientAddr, &AddrLen);
        if(recvSize > 0){
            if((packet1.type & FIN) && packet1.Check()){
                cout<<"Server�յ���һ�λ���:FIN"<<endl;

                //�ڶ��λ��� ACK=1 seq=y ack=x+1
                cout<<"----------------���͵ڶ��λ���-----------------"<<endl;
                packet2.ScrPort = SERVER_PORT;
                packet2.DestPort = ROUTER_PORT;
                packet2.type = ACK;
                packet2.seq_no = ServerSeq;
                packet2.ack_no = packet1.seq_no+1;

                packet2.setChecksum();
                int sendSize = sendto(serverSocket, (char *)(&packet2), sizeof(packet2), 0, (SOCKADDR*)&clientAddr, AddrLen);
                if(sendSize <= 0){
                    cout<<"�ڶ��λ��ַ���ʧ��!"<<endl;
                    return false;
                }
                cout<<"Server���͵ڶ��λ���:ACK"<<endl;
                break;
            }
        }
    }

    //���͵����λ��� FIN=1 ACK=1 seq=z
    cout<<"----------------���͵����λ���-----------------"<<endl;
    //������������к�z
    std::srand(time(nullptr)); //�������
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
        cout<<"�����λ��ַ���ʧ��!"<<endl;
        return false;
    }
    cout<<"Server���͵����λ���:FIN,ACK"<<endl;

    //���ܵ��Ĵλ��� ACK=1 seq=x+1 ack=z
    cout<<"----------------���ܵ��Ĵλ���-----------------"<<endl;
    while(1){
        int recvSize = recvfrom(serverSocket, (char *)(&packet4), sizeof(packet4), 0, (SOCKADDR*)&clientAddr, &AddrLen);
        if(recvSize > 0){
            if(packet4.ack_no != packet3.seq_no+1){
                cout<<"�յ����ݰ����ǵ��Ĵλ���QAQ,���ܽ��ܵ��ش��ĵ�һ������"<<endl;
                continue;
            }
            if((packet4.type & ACK) && packet4.Check() && packet4.seq_no == packet3.ack_no){
                cout<<"Server�յ����Ĵλ���:ACK"<<endl;
                cout<<"-----------------�Ĵλ��ֳɹ�-----------------"<<endl;
                break;
            }
            else{
                if(!packet4.Check()){
                    cout<<"У��ʹ���"<<endl;
                    return false;
                }
                if(!(packet4.type & ACK)) {
                    cout<<"���ݰ�û��ACK"<<endl;
                    return false;
                }
                if(packet4.seq_no != packet3.ack_no) {
                    cout<<"���кŴ���"<<endl;
                    return false;
                }
            }
        }
    }
    cout<<"Server�Ͽ�����!"<<endl;
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
                //�ظ�ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.ack_no = recvPacket.seq_no + recvPacket.length;
                sendPacket.length = recvPacket.length;

                sendPacket.setChecksum();
                preSendPacket = sendPacket;
                int sendSize = sendto(serverSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<endl<<"Server�յ� seq= "<<recvPacket.seq_no<<" ack= "<<recvPacket.ack_no<<"�ı��Ĳ��ظ� seq= "<<sendPacket.seq_no<<" ack= "<<sendPacket.ack_no<<"�ı���"<<endl;
                return true;
            }
            // ���seq��=�ڴ�ֵ���������ظ��ı��ģ������ظ����ģ��ش��ñ��ĵĻظ�ACK
            // ��ע�����������������һ���ظ�ACK���Ķζ�ʧ�����·��ͷ���ʱ�ش����£������ظ��ı��ĶΣ�           
            if((recvPacket.seq_no != preRecvPacket.seq_no+preRecvPacket.length) && recvPacket.Check()){
                //�ظ�ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.type += REPEAT;
                sendPacket.setChecksum();
                int sendSize = sendto(serverSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<"���ظ����ĶΡ�Server�յ�seq="<<recvPacket.seq_no<<" ack= "<<recvPacket.ack_no<<"�ı��Ĳ��ظ�����REPEAT��־��ACk�ظ�"<<endl;
            }
        }
    }
}

void ServerReceiveFile(SOCKET serversocket,SOCKADDR_IN clientAddr){
    int AddrLen = sizeof(clientAddr);

    //�����ļ���
    Packet recvInfoPacket;
    u_int FileNameSize = 0;
    u_int FileSize=0;
    char FileName[10];
    ClientSeq++; //ͬ����ʼ�����к�
    while(1){
        int recvSize = recvfrom(serversocket, (char *)(&recvInfoPacket), sizeof(recvInfoPacket), 0, (SOCKADDR *)&clientAddr, &AddrLen);
        if(recvSize > 0){
            //�����ļ���
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
                cout<<"�����ļ���Ϊ��"<<FileName<<",��СΪ"<<FileSize<<"�ֽ�"<<endl;


                //�ظ�ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.ack_no = recvInfoPacket.seq_no + recvInfoPacket.length;
                sendPacket.length = recvInfoPacket.length;

                sendPacket.setChecksum();
                int sendSize = sendto(serversocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<"Server�յ� seq= "<<recvInfoPacket.seq_no<<" ack= "<<recvInfoPacket.ack_no<<"�ı��Ĳ��ظ� seq= "<<sendPacket.seq_no<<" ack= "<<sendPacket.ack_no<<"�ı���"<<endl;
                break;
            }

            // ���seq<�ڴ�ֵ���������ظ��ı��ģ������ظ����ģ��ش��ñ��ĵĻظ�ACK
            // ��ע�����������������һ���ظ�ACK���Ķζ�ʧ�����·��ͷ���ʱ�ش����£������ظ��ı��ĶΣ�
            else if(recvInfoPacket.seq_no!=ClientSeq && recvInfoPacket.Check()){
                //�ظ�ACK
                Packet sendPacket;
                sendPacket.ScrPort = SERVER_PORT;
                sendPacket.DestPort = ROUTER_PORT;
                sendPacket.type = ACK;
                sendPacket.type += REPEAT;
                //sendPacket.ack_no = recvInfoPacket.seq_no + recvInfoPacket.length;
                sendPacket.setChecksum();
                int sendSize = sendto(serversocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&clientAddr, AddrLen);
                cout<<"���ظ����ĶΡ�Server�յ� seq= "<<recvInfoPacket.seq_no<<"�ı��Ĳ����� ack= "<<sendPacket.ack_no<<"�ı���"<<endl;
            }
        }
    }

    cout<<">>>>>>>>>>>>>>�ɹ������ļ���Ϣ<<<<<<<<<<<<<<<"<<endl;

    //�����ļ�����
    int FillCount = FileSize / MAX_PACKET_SIZE;
    int LeaveSize = FileSize % MAX_PACKET_SIZE;
    char* filebuf = new char[MAX_BUFFER_SIZE];

    cout<<"��ʼ�����ļ����ݣ������װ�ر��ĶΡ�Ϊ"<<FillCount<<"��"<<endl;

    for(int i=0;i<FillCount;i++){
        Packet dataPacket;
        if(ServerReceivePacket(dataPacket,serversocket,clientAddr)){
            cout<<"��"<<i<<"/"<<FillCount<<"��"<<endl;
            cout<<"�������ݰ�("<<dataPacket.seq_no<<")�ɹ�!"<<endl;
            for(int j=0;j<MAX_PACKET_SIZE;j++){
                filebuf[i*MAX_PACKET_SIZE+j] = dataPacket.data[j];
            }
        }
        else{
            cout<<"�������ݰ�ʧ��!"<<endl<<endl;
            return;
        }
    }

    if(LeaveSize>0){
        cout<<"��ʼ����ʣ������ݰ�"<<endl;
        Packet dataPacket;
        if(ServerReceivePacket(dataPacket,serversocket,clientAddr)){
            cout<<"�������ݰ�("<<dataPacket.seq_no<<")�ɹ�!"<<endl<<endl;
            for(int j=0;j<LeaveSize;j++){
                filebuf[FillCount*MAX_PACKET_SIZE+j] = dataPacket.data[j];
            }
        }
        else{
            cout<<"�������ݰ�ʧ��!"<<endl<<endl;
            return;
        }
    }

    cout<<endl<<"�����ļ����ݳɹ�!д���ļ�......"<<endl<<endl;
    //д���ļ�
    string FileName_string=string(FileName);
    FileName_string = "server_"+FileName_string;
    FILE* fp = fopen(FileName_string.c_str(),"wb");
    if(fp == NULL){
        cout<<"д���ļ�ʧ��!"<<endl;
        return ;
    }
    if(filebuf){
        fwrite(filebuf,1,FileSize,fp);
        fclose(fp);
        cout<<"д���ļ��ɹ�!"<<endl;
    }
    else{
        cout<<"д���ļ�ʧ��!"<<endl;
        return ;
    }
}




int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    // ��ʼ��Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    // �����׽���
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    unsigned long on = 1;
    ioctlsocket(serverSocket, FIONBIO, &on); // ���÷�����

    if (serverSocket == INVALID_SOCKET)
    {
        cout << "����socket��fail��\n"<< endl;
        return -1;
    }
    cout << "����socket��success��\n"<< endl;

    // ���׽���
    serverAddr.sin_family = AF_INET;                          // ��ַ��
    serverAddr.sin_port = htons(SERVER_PORT);                  // �˿ں�
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // IP��ַ
    int temp = bind(serverSocket, (LPSOCKADDR)&serverAddr, sizeof(serverAddr));

    // ��ʼ���ͻ��˺�·����
    clientAddr.sin_family = AF_INET;                          // ��ַ��
    clientAddr.sin_port = htons(ROUTER_PORT);                  // �˿ں�
    clientAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // IP��ַ

    std::srand(time(0)); // ������������������������ʱ�����к�
    ServerSeq = rand() % 10000;
    //ServerSeq = 5000;

    cout<<"����ǰ��ServerSeq= "<<ServerSeq<<endl;

    //����  
    Connect_Client(serverSocket,clientAddr);

    cout<<"���ֺ��ServerSeq= "<<ServerSeq<<endl;

    //�����ļ�
    ServerReceiveFile(serverSocket,clientAddr);

    //�Ͽ�����
    std::srand(time(nullptr)); //������û������к�
    ServerSeq = std::rand() % 10000;
    Close_Connect_Client(serverSocket,clientAddr);

    //�ر�socket
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
