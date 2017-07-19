/// \file		ConstDef.h
/// \brief		使用常数的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建
#ifndef CONSTDEF_H_
#define CONSTDEF_H_

#ifdef _WIN32
#pragma once
#endif

// 默认IO服务线程数
const unsigned short DEFAULT_IOTHREAD_COUNT = 6;

// 心跳间隔时间(毫秒)
const unsigned int HEARTBEAT_INTERVAL = 15000;

// 心跳检测次数
const unsigned int NO_HEARTBEAT_COUNT = 3;

// 一般睡眠时间(毫秒)
const unsigned int NORMAL_SLEEP_TIME = 50;

// 起始字节
const unsigned char STX = 0x02;

// 终止字节
const unsigned char ETX = 0x03;

// 数据包开始标志字节
const unsigned char PACKET_START_FLAG = 0x78;

// 心跳标志
const unsigned char HEARTBEAT_BITFLAG = 0x01;

// 接收缓冲区大小
const unsigned int RECV_BUFFER_SIZE = 1024 * 1024;

// 原始数据发送队列允许最大长度
const unsigned int MAX_RAWDATA_COUNT = 10000;

#endif
