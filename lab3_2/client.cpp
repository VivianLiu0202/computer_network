// client.cpp
#include "common.h"
#include <mutex>
using namespace std;

const uint32_t ROUTER_PORT = 33333; // ·�����˿ں�
const uint32_t CLIENT_PORT = 22222; // client�˿ں�

int ClientSeq = 0;
int ServerSeq = 0;
int totalTimeOut = 0;

int base = 0;
int nextseqnum = 0;
int pktStart;
bool sendAgain = 0;
bool finished = 0;

std::mutex mtx;
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
        cout << "����ʧ�ܣ�������룺" << error << endl;
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
                    cout<<"�ڶ�������У��ʹ���"<<endl;
                    return false;
                }
            }
        }

        //��ʱ�������·������ݰ�
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
                    cout<<"�ڶ��λ���У��ʹ���"<<endl;
                    return false;
                }
            }
        }
        //��ʱ�������·������ݰ�
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
                    cout<<"�����λ���У��ʹ���"<<endl;
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

struct parameters {
	SOCKADDR_IN serverAddr;
	SOCKET clientSocket;
	int pktSum;
};


//һ���߳����ڷ����ļ����ݣ���һ���߳����ڽ���ȷ����Ϣ��ACK��
//����ack���߳�
DWORD WINAPI recvThread(PVOID pParam)
{
	parameters* pa = (parameters*)pParam;
	SOCKADDR_IN serverAddr = pa->serverAddr;
	SOCKET clientSocket = pa->clientSocket;
	int pktSum = pa->pktSum;
	int AddrLen = sizeof(serverAddr);

	int mistake_ACK = -1;
	int mistake_count = 0;
	while (1){
		//rdt_rcv
		Packet recvPacket;
		int recvSize = recvfrom(clientSocket, (char*)&recvPacket, sizeof(recvPacket), 0, (sockaddr*)&serverAddr, &AddrLen);
		if (recvSize > 0){
            if (recvPacket.type & REPEAT){
                cout << "���յ��ظ�ȷ�ϰ���ȷ�Ϻ�Ϊ" << recvPacket.ack_no << endl;
                continue;
            }
			//�ɹ��յ���Ϣ����notcorrupt
			if (recvPacket.Check()){
				if (recvPacket.ack_no >= base) base = recvPacket.ack_no + 1;
				if (base != nextseqnum) pktStart = clock();

                std::lock_guard<std::mutex> lock(mtx);
				cout << "\n#----����RECV----#Client���յ���ack = " << recvPacket.ack_no << "����ȷ�ϱ���" << endl;
				//��ӡ�������
                cout << "��������Ϣ�� �����ܴ�С��" << WINODWS_SIZE << "��base = "<<base<<" , nextseqnum = "<<nextseqnum<<",�ѷ��͵�δ�յ�ACK��" << nextseqnum - base << "����δ���ͣ�" << WINODWS_SIZE - (nextseqnum - base) <<'\n';

				//�жϽ��������
				if (recvPacket.ack_no == pktSum - 1){
					cout << "\n����......" << endl;
					finished = 1;
					return 0;
				}

				//�����ش�
				if (mistake_ACK != recvPacket.ack_no){
					mistake_count = 0;
					mistake_ACK = recvPacket.ack_no;
				}
				else{
					mistake_count++;
				}

				if (mistake_count == 3) {
                    sendAgain = 1;//���·���
                }
			}
			//��У��ʧ�ܻ�ack���ԣ�����ԣ������ȴ�
		}
	}
	return 0;
}


void ClientSendRecvFile(string filename, SOCKET clientSocket, SOCKADDR_IN serverAddr)
{
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
        std::cerr << "�ڴ����ʧ��" << std::endl;
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


	int FillCount = FileSize / MAX_PACKET_SIZE;//ȫװ���ı��ĸ���
	int LeaveSize = FileSize % MAX_PACKET_SIZE;//����װ����ʣ�౨�Ĵ�
	//=================== ����������Ϣ�߳� =====================
	int pktSum = LeaveSize > 0 ? FillCount + 2 : FillCount + 1;

	parameters param;
	param.serverAddr = serverAddr;
	param.clientSocket = clientSocket;
	param.pktSum = pktSum;
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)recvThread, &param, 0, 0);

	while (1){
		if (nextseqnum < base + WINODWS_SIZE && nextseqnum < pktSum)
		{
			Packet ContentPacket;
			if (nextseqnum == 0){
				ContentPacket.ScrPort = CLIENT_PORT;
				ContentPacket.DestPort = ROUTER_PORT;
				ContentPacket.length = FileSize;
				ContentPacket.type += INFO;
				ContentPacket.seq_no = nextseqnum;
                for(int i=0;i<FILENAME.length();i++)
                    ContentPacket.data[i] = FILENAME[i];
                ContentPacket.data[FILENAME.length()] = '\0';
                ContentPacket.setChecksum();
			}
			else if (nextseqnum == FillCount + 1 && LeaveSize > 0){
				ContentPacket.ScrPort = CLIENT_PORT;
				ContentPacket.DestPort = ROUTER_PORT;
				ContentPacket.seq_no = nextseqnum;
                ContentPacket.type += DATA;
				for (int j = 0; j < LeaveSize; j++)
					ContentPacket.data[j] = filebuf[FillCount * MAX_PACKET_SIZE + j];
				ContentPacket.setChecksum();
			}
			else{
				ContentPacket.ScrPort = CLIENT_PORT;
				ContentPacket.DestPort = ROUTER_PORT;
				ContentPacket.seq_no = nextseqnum;
                ContentPacket.type += DATA;
				for (int j = 0; j < MAX_PACKET_SIZE; j++)
					ContentPacket.data[j] = filebuf[(nextseqnum - 1) * MAX_PACKET_SIZE + j];
				ContentPacket.setChecksum();
			}

			//send_pkt
			sendto(clientSocket, (char*)&ContentPacket, sizeof(ContentPacket), 0, (sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));

            std::lock_guard<std::mutex> lock(mtx);
			cout << "\n��!!!!����SEND!!!!��Client�ѷ��͡�seq = " << ContentPacket.seq_no << "���ı��ĶΣ�" << endl;
			if (base == nextseqnum){
				pktStart = clock();
			}
			nextseqnum++;
			//��ӡ�������
            cout << "��������Ϣ�� �����ܴ�С��" << WINODWS_SIZE << "��base = "<<base<<" , nextseqnum = "<<nextseqnum<<",�ѷ��͵�δ�յ�ACK��" << nextseqnum - base << "����δ���ͣ�" << WINODWS_SIZE - (nextseqnum - base) <<'\n';
		}

		//timeout
		if (clock() - pktStart> TIMEOUT_MILLISECONDS || sendAgain){
            totalTimeOut++;
			if (sendAgain) 
            {
                std::lock_guard<std::mutex> lock(mtx);
                cout << "�������ش��������յ����δ���ACK: ��ʼ�������ش���......" << endl;
            }
			//�ط���ǰ��������message
			Packet ContentPacket;
			for (int i = 0; i < nextseqnum - base; i++){
				int sendnum = base + i;
				if (sendnum == 0){
                    ContentPacket.ScrPort = CLIENT_PORT;
                    ContentPacket.DestPort = ROUTER_PORT;
                    ContentPacket.length = FileSize;
                    ContentPacket.type += INFO;
                    ContentPacket.seq_no = sendnum;
                    for(int i=0;i<FILENAME.length();i++)
                        ContentPacket.data[i] = FILENAME[i];
                    ContentPacket.data[FILENAME.length()] = '\0';
                    ContentPacket.setChecksum();
				}
				else if (sendnum == FillCount + 1 && LeaveSize > 0){
                    ContentPacket.ScrPort = CLIENT_PORT;
                    ContentPacket.DestPort = ROUTER_PORT;
                    ContentPacket.seq_no = sendnum;
                    for (int j = 0; j < LeaveSize; j++)
                        ContentPacket.data[j] = filebuf[FillCount * MAX_PACKET_SIZE + j];
                    ContentPacket.setChecksum();
				}
				else{
                    ContentPacket.ScrPort = CLIENT_PORT;
                    ContentPacket.DestPort = ROUTER_PORT;
                    ContentPacket.seq_no = sendnum;
                    for (int j = 0; j < MAX_PACKET_SIZE; j++)
                        ContentPacket.data[j] = filebuf[(sendnum - 1) * MAX_PACKET_SIZE + j];
                    ContentPacket.setChecksum();
                   // int result = sendto(clientSocket, (char*)&ContentPacket, sizeof(ContentPacket), 0, (sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
				}
				int result = sendto(clientSocket, (char*)&ContentPacket, sizeof(ContentPacket), 0, (sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
                {
                    std::lock_guard<std::mutex> lock(mtx);
				    cout << "seq = " << ContentPacket.seq_no << "�ı��Ķ��ѳ�ʱ�������ش�......" << endl; 
                }

			}
			pktStart = clock();
			sendAgain = 0;
		}
		if (finished == 1) break;
	}

	CloseHandle(hThread);
	cout << "\n\n�ѷ��Ͳ�ȷ�����б��ģ��ļ�����ɹ���\n\n";
    delete[] filebuf;
	//���㴫��ʱ���������
    int end = clock();
    cout<<endl<<"-----------------��������ܽ�---------------"<<endl;
    double time = (double)(end-start)/CLOCKS_PER_SEC;
    cout<<"����ʱ�䣺"<<time<<"s"<<endl;
    float throughput = (float)FileSize/time;
    cout<<"�����ʣ�"<<throughput<<"B/s = "<<throughput/1024<<"KB/s"<<endl;
    cout<<"�����ļ���С��"<<FileSize<<"B = "<<FileSize/1024<<"KB"<<endl;
    cout<<"��ʱ�������ݰ���"<<totalTimeOut<<endl;
    cout<<"-----------------��������ܽ�---------------"<<endl;
    return ;
}




int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    // ��ʼ��Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }
    else{
        std::cout << "WSAStartup success!" << std::endl;
    }

    // ==============�����������IP��ַ�Ͷ˿ںţ��Ա�ͻ���֪��Ӧ�����ݷ��͵�����================

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    unsigned long on = 1;
    ioctlsocket(clientSocket, FIONBIO, &on); // ���÷�����

    if (clientSocket == INVALID_SOCKET){
        std::cerr << "socket failed!" << std::endl;
        return 1;
    }
    else{
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

    // cout<<"����ǰ��ClientSeq= "<<ClientSeq<<endl;


    //���ӷ�����
    int temp = Connect_Server(clientSocket,serverAddr);

    cout<<"���ֺ��ClientSeq= "<<ClientSeq<<endl;

    while(temp){
        cout<<"�������ļ�·����"<<endl;
        string filename;
        cin>>filename;
        //ClientSendFile(filename,clientSocket,serverAddr);
        ClientSendRecvFile(filename, clientSocket,serverAddr);
        cout<<"�Ƿ�������䣿(n/n)"<<endl;
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
    system("pause");
    return 0;
}
