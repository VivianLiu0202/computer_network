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

// 需要链接WS2_32.lib
#pragma comment(lib,"ws2_32.lib")

const int MAX_BUFFER_SIZE = 20000000; //最大文件大小
const int MAX_PACKET_SIZE = 10000; //单个数据包大小
const int MAX_REPEAT_TIMES = 3000; //最大发送次数
const int TIMEOUT_MILLISECONDS = 100; // 超时时间
const int WINODWS_SIZE = 5; // 窗口大小

const char* IP = "127.0.0.1"; //服务器、路由器、客户端IP地址

const unsigned short SYN = 0x1;
const unsigned short ACK = 0x2;
const unsigned short FIN = 0x4;
const unsigned short INFO = 0x8;
const unsigned short DATA = 0x10;

#pragma pack(1) // 让编译器将结构体数据强制连续排列，禁止优化存储结构进行对齐
struct Packet {
    u_int SrcIP,DestIP; // 源IP和目的IP
    u_short ScrPort,DestPort; // 源端口和目的端口
    u_short type; // 标志位
    u_int seq_no; // 序列号
    u_int ack_no; // 确认号
    u_int length; // 消息大小
    u_short checksum; // 校验和
    char data[MAX_PACKET_SIZE]; // 报文数据
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
}

/**
 * 校验和：将报文中所有16位的字相加，然后将进位回卷加到低16位中，取反
 * “回卷校验和”的算法，一种常用的网络数据包错误检测方法。
*/
void Packet::setChecksum(){
    this->checksum = 0;
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)this;
    for (size_t i = 0; i < sizeof(Packet) / sizeof(unsigned short); ++i) {
        sum += *ptr;
        if (sum >> 16) { // 如果溢出则回卷
            sum = (sum & 0xFFFF);
            sum++;
        }
        ++ptr;
    }
    this->checksum = ~(sum & 0xFFFF);
}

/**
 * 验证校验和：将报文中所有16位的字相加，然后将进位回卷加到低16位中，如果结果为0xFFFF则校验成功
*/
bool Packet::Check(){
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)this;
    for (size_t i = 0; i < sizeof(Packet) / sizeof(unsigned short); ++i) {
        sum += *ptr;
        if (sum >> 16) { // 如果溢出则回卷
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