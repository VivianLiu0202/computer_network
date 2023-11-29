// client.cpp
#include "common.h"
using namespace std;

const uint32_t ROUTER_PORT = 30000; // ·�����˿ں�

const uint32_t CLIENT_PORT = 20000; // client�˿ں�

int ClientSeq = 0;
int ServerSeq = 0;
Packet prePacket;
int totalTimeOut = 0;



//ʵ���������� ~
bool Connect_Server(SOCKET &clientSocket, sockaddr_in &serverAddr){
    int AddrLen = sizeof(serverAddr);
    cout<<"-----------------��ʼ��������-----------------"<<endl;
    Packet Packet1;
    Packet Packet2;
    Packet Packet3;

    //��һ������ SYN=1 seq=x
    cout<<"----------------��ʼ��һ������-----------------"<<endl;
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
        cout << "����ʧ�ܣ��������?" << error << endl;
        return false;
    }
    cout<<"Client���͵�һ������:SYN!"<<endl;

    //���յڶ������� SYN=1 ACK=1 ack=x
    clock_t start1 = clock();
    cout<<"----------------���ܵڶ�������-----------------"<<endl;
    int ReSendCount = 0;
    while(1){
        int recvSize = recvfrom(clientSocket, (char *)(&Packet2), sizeof(Packet2), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if (recvSize > 0) {
            if((Packet2.type & ACK) && (Packet2.type & SYN) && Packet2.ack_no == Packet1.seq_no+1 && Packet2.Check()){
                cout<<"Client�յ��ڶ�������:ACK,SYN"<<endl;
                ServerSeq = Packet2.seq_no;
                cout<<"Server��seq_no="<<ServerSeq<<endl;
                break;
            }
            else{
                if(!(Packet2.type & ACK) || !(Packet2.type & SYN)){
                    cout<<"�ڶ������ֱ�Ǵ�û��ACK/SYN"<<endl;
                    return false;
                }
                else if(Packet2.ack_no != Packet1.seq_no+1){
                    cout<<"�ڶ����������кŴ���"<<endl;
                    return false;
                }
                else if(!Packet2.Check()){
                    cout<<"�ڶ�������У��ʹ���?"<<endl;
                    return false;
                }
            }
        }

        //��ʱ���������·������ݰ�
        else if(clock() - start1 > TIMEOUT_MILLISECONDS){
            if(ReSendCount == MAX_REPEAT_TIMES){
                cout<<"�ڶ������ֳ�ʱ,�ش���������"<<MAX_REPEAT_TIMES<<"�Σ�����ʧ�ܣ�"<<endl;
                return false;
            }
            ReSendCount++;
            cout<<"�ڶ������ֳ�ʱ�����·��͵�һ���������ݰ�......"<<endl;
            int SendSize = sendto(clientSocket, (char *)(&Packet1), sizeof(Packet1), 0, (SOCKADDR*)&serverAddr, AddrLen);
            start1 = clock();
            if (SendSize <=0) {
                cout<<"��һ�������ٴη���ʧ�ܣ�"<<endl;
                return false;
            }
            else{
                cout<<"��һ�������ٴη��ͳɹ���"<<endl;
                continue;
            }
        }

    }

    //���������� ACK=1 seq=x+1
    cout<<"----------------��ʼ����������-----------------"<<endl;
    Packet3.ScrPort = CLIENT_PORT;
    Packet3.DestPort = ROUTER_PORT;
    Packet3.type = ACK;
    Packet3.seq_no = Packet1.seq_no+1;
    Packet3.ack_no = Packet2.seq_no+1;

    Packet3.setChecksum();
    int SendSize = sendto(clientSocket, (char *)(&Packet3), sizeof(Packet3), 0, (SOCKADDR*)&serverAddr, AddrLen);
    if (SendSize<=0) {
        cout<<"������������Ϣ����ʧ��"<<endl;
        return false;
    }
    cout<<"Client���͵���������:ACK!"<<endl;
    cout<<"-----------------�������ֳɹ�-----------------"<<endl;
    return true;
}

//ʵ���Ĵλ���~
bool Close_Connect_Server(SOCKET &clientSocket, sockaddr_in &serverAddr){
    int AddrLen = sizeof(serverAddr);
    cout<<"-----------------��ʼ�Ĵλ���-----------------"<<endl;
    Packet Packet1;
    Packet Packet2;
    Packet Packet3;
    Packet Packet4;

    //��һ�λ��� FIN=1 seq=y
    cout<<"----------------��ʼ��һ�λ���-----------------"<<endl;
    Packet1.ScrPort = CLIENT_PORT;
    Packet1.DestPort = ROUTER_PORT;
    Packet1.type = FIN;
    Packet1.seq_no = ClientSeq;
    Packet1.ack_no = 0;

    Packet1.setChecksum();
    int sendSize1 = sendto(clientSocket, (char *)(&Packet1), sizeof(Packet1), 0, (SOCKADDR*)&serverAddr, AddrLen);
    clock_t start1 = clock();
    if (sendSize1 <= 0) {
        cout<<"��һ�λ�����Ϣ����ʧ�ܣ�"<<endl;
        return false;
    }
    cout<<"Client���͵�һ�λ�����Ϣ:FIN!"<<endl;

    //���ܵڶ��λ��� �ȴ��������ظ� ack=y ACK=1
    int ReSendCount = 0;
    cout<<"----------------���ܵڶ��λ���-----------------"<<endl;
    while(1){
        int recvSize2 = recvfrom(clientSocket, (char *)(&Packet2), sizeof(Packet2), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if (recvSize2 > 0) {
            if((Packet2.type & ACK) && Packet2.ack_no == Packet1.seq_no+1 && Packet2.Check()){
                cout<<"Client���ܵڶ��λ�����Ϣ:ACK!"<<endl;
                cout<<"�ڶ��λ��ֳɹ���"<<endl;
                break;
            }
            else{
                if(!(Packet2.type & ACK)){
                    cout<<"�ڶ��λ��ֱ��û��ACK"<<endl;
                    return false;
                }
                else if(Packet2.ack_no != Packet1.seq_no+1){
                    cout<<"�ڶ��λ������кŴ���"<<endl;
                    return false;
                }
                else if(!Packet2.Check()){
                    cout<<"�ڶ��λ���У��ʹ���?"<<endl;
                    return false;
                }
            }
        }
        //��ʱ���������·������ݰ�
        if(clock() - start1 > TIMEOUT_MILLISECONDS){
            if(ReSendCount == MAX_REPEAT_TIMES){
                cout<<"�ڶ��λ��ֳ�ʱ,�ش���������"<<MAX_REPEAT_TIMES<<"�Σ�����ʧ�ܣ�"<<endl;
                return false;
            }
            ReSendCount++;
            cout<<"�ڶ��λ��ֳ�ʱ,���·��͵�һ�λ������ݰ�......"<<endl;
            int SendSize = sendto(clientSocket, (char *)(&Packet1), sizeof(Packet1), 0, (SOCKADDR*)&serverAddr, AddrLen);
            start1 = clock();
            if (SendSize <=0) {
                cout<<"��һ�λ����ٴη���ʧ�ܣ�"<<endl;
                return false;
            }
            else{
                cout<<"��һ�λ����ٴη��ͳɹ���"<<endl;
                continue;
            }
        }
    }

    //�����λ��� ���ܷ��������ݰ� FIN=1 ACK=1 seq=z
    cout<<"----------------���ܵ����λ���-----------------"<<endl;
    while(1){
        int recvSize3 = recvfrom(clientSocket, (char *)(&Packet3), sizeof(Packet3), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if(recvSize3 <= 0){
            cout<<"���ܵ����λ���ʧ�ܣ�"<<endl;
            return false;
        }
        else if (recvSize3 > 0) {
            if((Packet3.type & FIN) && (Packet3.type & ACK) && Packet3.Check() && Packet3.ack_no == Packet2.ack_no){
                cout<<"Client���ܵ����λ�����Ϣ:FIN,ACK!"<<endl;
                break;
            }
            else{
                if(!(Packet3.type & FIN) || !(Packet3.type & ACK)){
                    cout<<"�����λ��ֱ��û��FIN/ACK"<<endl;
                    return false;
                }
                else if(Packet3.ack_no != Packet2.ack_no){
                    cout<<"�����λ������кŴ���"<<endl;
                    return false;
                }
                else if(!Packet3.Check()){
                    cout<<"�����λ���У��ʹ���?"<<endl;
                    return false;
                }
            }
        }
    }

    cout<<"----------------���͵��Ĵλ���-----------------"<<endl;
    //���Ĵλ��� ACK=1 seq=z+1
    Packet4.ScrPort = CLIENT_PORT;
    Packet4.DestPort = ROUTER_PORT;
    Packet4.type = ACK;
    Packet4.seq_no = Packet3.ack_no;
    Packet4.ack_no = Packet3.seq_no+1; //�����ack_no����һ�����ݰ���seq_no+1
    Packet4.setChecksum();

    int sendSize4 = sendto(clientSocket, (char *)(&Packet4), sizeof(Packet4), 0, (SOCKADDR*)&serverAddr, AddrLen);
    if (sendSize4 <= 0) {
        cout<<"���Ĵλ���ʧ�ܣ�"<<endl;
        return false;
    }
    cout<<"Client���͵��Ĵλ�����Ϣ:ACK!"<<endl;

    //�ȴ�����������2MSL״̬
    int tclock = clock();
    cout<<"client wait 2MSL~"<<endl;
    Packet PacketTemp;
    while(clock() - tclock < 2*TIMEOUT_MILLISECONDS){
        int recvSize = recvfrom(clientSocket, (char *)(&PacketTemp), sizeof(PacketTemp), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if(recvSize == 0){
            cout<<"�ر�����ʧ�ܣ�"<<endl;
            return false;
        }
        else if (recvSize > 0) {
            int sendSize= sendto(clientSocket, (char *)(&Packet4), sizeof(Packet4), 0, (SOCKADDR*)&serverAddr, AddrLen);
            cout<<"���»ظ�ack"<<endl;
        }
    }
    cout<<"-----------------�Ĵλ��ֳɹ�-----------------"<<endl;
    return true;
}

//�������ݰ�
int ClientSendAndRecvPacket(Packet& sendPacket,SOCKET clientSocket, SOCKADDR_IN serverAddr){
    int AddrLen = sizeof(serverAddr); //��ַ����
    int sendSize = sendto(clientSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN));
    if (sendSize <= 0) {
        int error = WSAGetLastError();
        cout << "����ʧ�ܣ��������?" << error << endl;
        return false;
    }
    cout<<endl<<"Client�ѷ��� seq= "<<sendPacket.seq_no<<" ack= "<<sendPacket.ack_no<<" length= "<<sendPacket.length<<"�����ݰ�"<<endl;
    int TimesOfTimeOut = 0; //��ʱ����
    clock_t start = clock();

    Packet RecvPacket;
    while(1){
        int recvSize = recvfrom(clientSocket, (char *)(&RecvPacket), sizeof(RecvPacket), 0, (SOCKADDR*)&serverAddr, &AddrLen);
        if (recvSize > 0) {
            // cout<<"recvPacket.flag="<<RecvPacket.type<<endl;
            // cout<<"recvPacket.ack_no="<<RecvPacket.ack_no<<endl;
            // cout<<"recvPacket.seq_no + recvPacket.length="<<RecvPacket.seq_no+RecvPacket.length<<endl;

            if((RecvPacket.type & REPEAT) && (RecvPacket.type & ACK)){
                cout<<"�յ����кŴ������ݰ�������Ϊ�ظ����ݰ�"<<endl;
                continue;
            }

            else if((RecvPacket.type & ACK) && RecvPacket.ack_no == sendPacket.seq_no+sendPacket.length){
                cout<<"Client���յ� seq= "<<std::dec <<RecvPacket.seq_no<<" ack= "<<RecvPacket.ack_no<<" length= "<<RecvPacket.length<<"��ACK����"<<endl;
                return RecvPacket.seq_no;
            }

        }
        //��ʱ����
        else if(clock() - start > TIMEOUT_MILLISECONDS){
            if(TimesOfTimeOut >= MAX_REPEAT_TIMES){
                cout<<"��ʱ�ش���"<<MAX_REPEAT_TIMES<<"���Ρ�����ʧ�ܣ�"<<endl;
                return -1;
            }
            TimesOfTimeOut++;
            totalTimeOut++;
            cout<<"����ʱ�ش������к�Ϊ seq= "<<sendPacket.seq_no<<"�ı��ĳ�ʱ,���·���"<<endl;;
            sendto(clientSocket, (char *)(&sendPacket), sizeof(sendPacket), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN));
            start = clock();
        }
        else continue;
    }
}

//�����ļ�
void ClientSendFile(string filename,SOCKET clientSocket, SOCKADDR_IN serverAddr){
    //���ļ�
    ifstream file(filename,ios::in|ios::binary);
    if(!file.is_open()){
        cout<<"�ļ���ʧ�ܣ�"<<endl;
        return ;
    }
    cout<<"�ļ��򿪳ɹ���"<<endl;

    //��ȡ�ļ���С
    file.seekg(0,ios::end);
    int FileSize = file.tellg();
    file.seekg(0,ios::beg);
    cout<<"�ļ���СΪ��"<<FileSize<<"B"<<endl;

    //��ȡ�ļ�����
    char *filebuf = new char[FileSize];
    if (filebuf == NULL)
    {
        std::cerr << "�ڴ����ʧ��?" << std::endl;
        file.close();
        return;
    }

    file.read(filebuf,FileSize);
    file.close();
    cout<<"�ļ���ȡ�ɹ���"<<endl;

    string FileSize_str=std::to_string(FileSize);

    //��ȡ�ļ���
    string FILENAME;
    int i;
    for(i=filename.length()-1;i>=0;i--){
        if(filename[i] == '\\') break;
    }
    for(int j=i+1;j<filename.length();j++){
        FILENAME += filename[j];
    }
    cout<<"�ļ���Ϊ��"<<FILENAME<<endl;

    int start = clock();

    //�����ļ������ļ���С
    cout<<"----------------��ʼ�����ļ�"<<FILENAME<<"����Ϣ--------------"<<endl;
    totalTimeOut = 0;
    Packet ContentPacket;
    ContentPacket.ScrPort = CLIENT_PORT;
    ContentPacket.DestPort = ROUTER_PORT;
    //ContentPacket.length = FILENAME.length()+1; //�����β����?
    ContentPacket.length = FileSize; //�����β����?
    ContentPacket.type = INFO;
    ContentPacket.seq_no = ++ClientSeq;


    for(int i=0;i<FILENAME.length();i++){
        ContentPacket.data[i] = FILENAME[i];
    }
    ContentPacket.data[FILENAME.length()] = '\0';

    ContentPacket.setChecksum();

    int sendSize1 = ClientSendAndRecvPacket(ContentPacket,clientSocket,serverAddr);
    if(sendSize1 == -1){
        cout<<"�ļ���Ϣ����ʧ��"<<endl;
        return ;
    }
    cout<<">>>>>>>>>>>>>>�ɹ������ļ���Ϣ<<<<<<<<<<<<<<<"<<endl;

    //�����ļ�����
    cout<<"-----------------��ʼ�����ļ�����-------------------"<<endl;
    int FillCount = FileSize / MAX_PACKET_SIZE; //ȫ��װ���ı��ĸ���
    int LeaveSize = FileSize % MAX_PACKET_SIZE; //ʣ�౨�Ĵ�С

    prePacket = ContentPacket; //��¼��һ�����͵����ݰ�
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
            cout<<"�ļ����ݷ���ʧ��"<<endl;
            return ;
        }
        cout<<"�ɹ����Ͳ�ȷ�ϵ�"<<i+1<<"�����װ�ر��Ķ�?,���к� seq= "<<ContentPacket.seq_no<<",���ݳ��� length= "<<ContentPacket.length<<endl;
        prePacket = ContentPacket;
        preSeq = sendSize2;

    }
    cout<<endl<<"�ɹ�����"<<FillCount<<"�����ݰ�,��������ʣ�ಿ�����ݰ�"<<endl;
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
            cout<<"�ļ����ݷ���ʧ��"<<endl;
            return ;
        }
        cout<<"�ɹ��������һ�����ݰ�?"<<endl;
        prePacket = ContentPacket;
        preSeq = sendSize3;
    }

    delete[] filebuf;

    //���㴫��ʱ����������
    int end = clock();
    cout<<endl<<"-----------------��������ܽ�?---------------"<<endl;
    double time = (double)(end-start)/CLOCKS_PER_SEC;
    cout<<"����ʱ�䣺"<<time<<"s"<<endl;
    float throughput = (float)FileSize/time;
    cout<<"�����ʣ�"<<throughput<<"B/s = "<<throughput/1024<<"KB/s"<<endl;
    cout<<"�����ļ���С��"<<FileSize<<"B = "<<FileSize/1024<<"KB"<<endl;
    cout<<"��ʱ�������ݰ���"<<totalTimeOut<<endl;
    cout<<"-----------------��������ܽ�?---------------"<<endl;
    return ;
}

int main() {
WSADATA wsaData;
    SOCKET clientSocket;

    // ��ʼ��Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "WSAStartup success!" << std::endl;
    }

    // ==============�����������IP��ַ�Ͷ˿ںţ��Ա�ͻ���֪��Ӧ�����ݷ��͵�����?================

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    unsigned long on = 1;
    ioctlsocket(clientSocket, FIONBIO, &on); // ���÷�����

    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "socket failed!" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "socket success!" << std::endl;
    }
    // ��ʼ����������ַ
    // AF_INET: ��ʾ���׽���ʹ��IPv4��ַ��
    // SOCK_DGRAM: ��ʾ���׽�����һ�����ݱ��׽��֣�UDP����
    // 0: ָ��Э�����͡�����UDP������ӦΪ0
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;                          // IPv4
    serverAddr.sin_port = htons(ROUTER_PORT);                  // �˿ں�
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // ������IP��ַ

    // ��ʼ���ͻ��˵�ַ
    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;                          // ��ַ����
    clientAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // ��ַ
    clientAddr.sin_port = htons(CLIENT_PORT);                  // �˿ں�
    int bindtemp = bind(clientSocket, (LPSOCKADDR)&clientAddr, sizeof(clientAddr));

    std::srand(time(0)); // ������������������������ʱ�����к�
    ClientSeq = rand()%10000;

    cout<<"����ǰ��ClientSeq= "<<ClientSeq<<endl;


    //���ӷ�����
    int temp = Connect_Server(clientSocket,serverAddr);

    cout<<"���ֺ��ClientSeq= "<<ClientSeq<<endl;

    while(temp){
        cout<<"�������ļ�·����"<<endl;
        string filename;
        cin>>filename;
        ClientSendFile(filename,clientSocket,serverAddr);
        cout<<"�Ƿ��������?(n/n)"<<endl;
        char c;
        cin>>c;
        if(c == 'n') {
           
            break;
        }
    }
    cout<<"�ر����ӣ�"<<endl;

    //--------------�Ĵλ���-------------------
    std::srand(time(nullptr));
    ClientSeq = rand()%10000;

    Close_Connect_Server(clientSocket,serverAddr);

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
