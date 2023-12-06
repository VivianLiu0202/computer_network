// common.h
#ifndef COMMON_H
#define COMMON_H

#pragma once
#include <iostream>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>
#include <fstream>
#include <assert.h>

// ��Ҫ����WS2_32.lib
#pragma comment(lib,"ws2_32.lib")

const int MAX_BUFFER_SIZE = 20000000; //����ļ���С
const int MAX_PACKET_SIZE = 10000; //�������ݰ���С
const int MAX_REPEAT_TIMES = 60000; //����ʹ���
const int TIMEOUT_MILLISECONDS = 100; // ��ʱʱ��
const int WINODWS_SIZE = 5; // ���ڴ�С

const char* IP = "127.0.0.1"; //��������·�������ͻ���IP��ַ

const unsigned short SYN = 0x1;
const unsigned short ACK = 0x2;
const unsigned short FIN = 0x4;
const unsigned short INFO = 0x8;
const unsigned short DATA = 0x10;
const unsigned short REPEAT = 0x20;

#pragma pack(1) // �ñ��������ṹ������ǿ���������У���ֹ�Ż��洢�ṹ���ж���
struct Packet {
    u_int SrcIP,DestIP; // ԴIP��Ŀ��IP
    u_short ScrPort,DestPort; // Դ�˿ں�Ŀ�Ķ˿�
    u_short type; // ��־λ
    u_int seq_no; // ���к�
    u_int ack_no; // ȷ�Ϻ�
    u_int length; // ��Ϣ��С
    u_short checksum; // У���
    char data[MAX_PACKET_SIZE]; // ��������
    bool visited[2000];//SACK��ָ��ֻ��������Щ�ض��ķֶ�
    Packet();
    void setChecksum();
    bool Check();
    void PrintAll();
};

#pragma pack()

Packet::Packet(){
    memset(this, 0, sizeof(Packet));
    SrcIP = 0;
    DestIP = 0;
    seq_no = 0;
    ack_no = 0;
    length = 0;
    type = 0;
    for(int i=0;i<2000;i++)
        visited[i] = false;
}

/**
 * У��ͣ�������������16λ������ӣ�Ȼ�󽫽�λ�ؾ�ӵ���16λ�У�ȡ��
 * ���ؾ�У��͡����㷨��һ�ֳ��õ��������ݰ������ⷽ����
*/
void Packet::setChecksum(){
    this->checksum = 0;
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)this;
    for (size_t i = 0; i < sizeof(Packet) / sizeof(unsigned short); ++i) {
        sum += *ptr;
        if (sum >> 16) { // ��������ؾ�
            sum = (sum & 0xFFFF);
            sum++;
        }
        ++ptr;
    }
    this->checksum = ~(sum & 0xFFFF);
}

/**
 * ��֤У��ͣ�������������16λ������ӣ�Ȼ�󽫽�λ�ؾ�ӵ���16λ�У�������Ϊ0xFFFF��У��ɹ�
*/
bool Packet::Check(){
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)this;
    for (size_t i = 0; i < sizeof(Packet) / sizeof(unsigned short); ++i) {
        sum += *ptr;
        if (sum >> 16) { // ��������ؾ�
            sum = (sum & 0xFFFF);
            sum++;
        }
        ++ptr;
    }
    if(sum == 0xFFFF)
        return true;
    return false;
}

void Packet::PrintAll(){
    std::cout << "SrcIP: " << SrcIP << std::endl;
    std::cout << "DestIP: " << DestIP << std::endl;
    std::cout << "SrcPort: " << ScrPort << std::endl;
    std::cout << "DestPort: " << DestPort << std::endl;
    std::cout << "type: " << type << std::endl;
    std::cout << "seq_no: " << seq_no << std::endl;
    std::cout << "ack_no: " << ack_no << std::endl;
    std::cout << "length: " << length << std::endl;
    std::cout << "checksum: " << checksum << std::endl;
    std::cout << "data: " << data << std::endl;
}

#endif // COMMON_H