/**
 * 最简单的视音频数据处理示例
 * Simplest MediaData Test
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本项目包含如下几种视音频测试示例：
 *  (1)像素数据处理程序。包含RGB和YUV像素格式处理的函数。
 *  (2)音频采样数据处理程序。包含PCM音频采样格式处理的函数。
 *  (3)H.264码流分析程序。可以分离并解析NALU。
 *  (4)AAC码流分析程序。可以分离并解析ADTS帧。
 *  (5)FLV封装格式分析程序。可以将FLV中的MP3音频码流分离出来。
 *  (6)UDP-RTP协议分析程序。可以将分析UDP/RTP/MPEG-TS数据包。
 *
 * This project contains following samples to handling multimedia data:
 *  (1) Video pixel data handling program. It contains several examples to handle RGB and YUV data.
 *  (2) Audio sample data handling program. It contains several examples to handle PCM data.
 *  (3) H.264 stream analysis program. It can parse H.264 bitstream and analysis NALU of stream.
 *  (4) AAC stream analysis program. It can parse AAC bitstream and analysis ADTS frame of stream.
 *  (5) FLV format analysis program. It can analysis FLV file and extract MP3 audio stream.
 *  (6) UDP-RTP protocol analysis program. It can analysis UDP/RTP/MPEG-TS Packet.
 *
 */
#include <stdio.h>
#include <winsock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

#pragma pack(1)

/*
 * [memo] FFmpeg stream Command:
 * ffmpeg -re -i sintel.ts -f mpegts udp://127.0.0.1:8880
 * ffmpeg -re -i sintel.ts -f rtp_mpegts udp://127.0.0.1:8880
 */


// 共 12 字节 -> 前面的12个字节是固定存在于每个rtp中
typedef struct RTP_FIXED_HEADER{
    /* byte 0 */
    unsigned char csrc_len:4;            // SCRC 计数
    unsigned char extension:1;           // 扩展位
    unsigned char padding:1;             // 填充位
    unsigned char version:2;             // 版本号
    /* byte 1 */
    unsigned char payload:7;             // 有效载荷类型
    unsigned char marker:1;        /* expect 1 */   // 标记，占1位，不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始
    /* bytes 2, 3 */
    unsigned short seq_no;               // 序列号，占 16 位。这个字段表示 RTP 数据包的序列号。
    /* bytes 4-7 */
    unsigned int timestamp;           // 时间戳 ，32位
    /* bytes 8-11 */
    unsigned int ssrc;            /* stream number is used here. */    //同步信源(SSRC)标识符， 32 位
} RTP_FIXED_HEADER;


typedef struct MPEGTS_FIXED_HEADER {
    unsigned sync_byte: 8;                      // 同步字节，用于标识一个新的MPEG-TS数据包的开始，其值通常为0x47。
    unsigned transport_error_indicator: 1;      // 传输错误指示符，如果此比特为1，表示在传输过程中该数据包至少有一个未纠正的错误
    unsigned payload_unit_start_indicator: 1;   // 有效载荷单元开始指示符，如果一个PES数据包或PSI表跨越多个TS数据包，那么这个标志位在该PES数据包或PSI表的第一个TS数据包中为1，其它相关TS数据包中为0
    unsigned transport_priority: 1;             // ：传输优先级，当为1时表示当前TS数据包具有高优先级
    unsigned PID: 13;                           // PID值，用于标识TS数据包中所携带的信息类型，如音频、视频等
    unsigned scrambling_control: 2;             // 加扰控制，用于标识TS数据包是否经过加扰处理
    unsigned adaptation_field_exist: 2;         // 自适应字段存在标志，用于指示TS数据包中是否存在自适应字段和有效载荷
    unsigned continuity_counter: 4;             // 连续性计数器，这是一个模16计数器，用于检测丢包情况
} MPEGTS_FIXED_HEADER;

void print_rtp_header(RTP_FIXED_HEADER *header) {
    printf("csrc_len: %u\n", header->csrc_len);
    printf("extension: %u\n", header->extension);
    printf("padding: %u\n", header->padding);
    printf("version: %u\n", header->version);
    printf("payload: %u\n", header->payload);
    printf("marker: %u\n", header->marker);
    printf("seq_no: %u\n", ntohs(header->seq_no)); // 注意网络字节序转换
    printf("timestamp: %lu\n", ntohl(header->timestamp)); // 注意网络字节序转换
    printf("ssrc: %lu\n", ntohl(header->ssrc)); // 注意网络字节序转换
}


int simplest_udp_parser(int port)
{
    // WSADATA 是一个结构体，它被用来存储 WSAStartup 函数调用后返回的 Windows Socket 实现的信息。
    WSADATA wsaData;
    // Windows Socket 2.2 版本
    WORD sockVersion = MAKEWORD(2,2);
    int cnt=0;

    //FILE *myout=fopen("output_log.txt","wb+");
    FILE *myout=stdout;

    FILE *fp1=fopen("output_dump.ts","wb+");

    // 初始化 Winsock 库， 成功则返回0 ，否则返回非0
    if(WSAStartup(sockVersion, &wsaData) != 0){
        return 0;
    }

    /*
     * 创建UDP 套接字
     * 这个函数需要三个参数：
     *   地址族（AF_INET 表示 IPv4 网络协议）
     * 、套接字类型（SOCK_DGRAM 表示数据报套接字，通常用于 UDP 协议）
     * 、协议类型（IPPROTO_UDP 表示 UDP 协议）
     */
    SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(serSocket == INVALID_SOCKET){
        printf("socket error !");
        return 0;
    }

    // 套接字地址 --> 网络编程中常用的一个结构体，用于处理网络通信的地址。它包含了IP地址、端口号等信息，通常用于描述互联网的地址。
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;       //地址族为 IPv4
    serAddr.sin_port = htons(port);     // 端口号  , 需转换为BE --> 本地是LE（小端序） ，网络中是BE （大端序）
    serAddr.sin_addr.S_un.S_addr = INADDR_ANY;      //   IP 地址为 INADDR_ANY，表示套接字可以接收任何网络接口上的数据包

    // 将 serSocket 绑定到地址 serAddr
    if(bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR){
        printf("bind error !");
        closesocket(serSocket);
        return 0;
    }

    sockaddr_in remoteAddr;
    int nAddrLen = sizeof(remoteAddr);

    //How to parse?
    int parse_rtp=1;
    int parse_mpegts=1;

    printf("Listening on port %d\n",port);

    char recvData[10000];

    // 设置超时
    struct timeval timeout;
    timeout.tv_sec = 10000;  // 超时时间为10秒
    timeout.tv_usec = 0;
    if (setsockopt(serSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof timeout) < 0) {
        printf("setsockopt failed\n");
    }


    while (1){
        /*
         *  serSocket：这是一个套接字描述符，它标识了数据应该从哪个套接字接收
         *  recvData：这是一个指向缓冲区的指针，接收到的数据将被存储在这里
         *  10000：这是缓冲区的大小，表示最多可以接收多少字节的数据
         *  0：这是一组标志位，通常设置为0。
         *  (sockaddr *)&remoteAddr：这是一个指向 sockaddr 结构体的指针，用于存储发送方的地址信息
         *  &nAddrLen：这是一个指向 socklen_t 类型变量的指针，用于存储发送方地址结构体的大小
         *  pktsize ： 只有出现错误的时候才会返回-1
         */
        int pktsize = recvfrom(serSocket, recvData, 10000, 0, (sockaddr *)&remoteAddr, &nAddrLen);
        if (pktsize > 0){
            //printf("Addr:%s\r\n",inet_ntoa(remoteAddr.sin_addr));
            //printf("packet size:%d\r\n",pktsize);
            //Parse RTP
            if(parse_rtp!=0){
                char payload_str[10]={0};
                RTP_FIXED_HEADER rtp_header;
                int rtp_header_size=sizeof(RTP_FIXED_HEADER);
                //RTP Header
                memcpy((void *)&rtp_header,recvData,rtp_header_size);
                //RFC3551
                char payload=rtp_header.payload;

                switch(payload){
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 18: sprintf(payload_str,"Audio");break;
                    case 31: sprintf(payload_str,"H.261");break;
                    case 32: sprintf(payload_str,"MPV");break;
                    case 33: sprintf(payload_str,"MP2T");break;
                    case 34: sprintf(payload_str,"H.263");break;
                    case 96: sprintf(payload_str,"H.264");break;
                    default:sprintf(payload_str,"other");break;
                }

                // BE --> LE
                unsigned int timestamp=ntohl(rtp_header.timestamp);     // 时间戳
                unsigned int seq_no=ntohs(rtp_header.seq_no);           // RTP数据包的序列号

                fprintf(myout,"[RTP Pkt] %5d| %5s| %10u| %5d| %5d|\n",cnt,payload_str,timestamp,seq_no,pktsize);

                //RTP Data
                char *rtp_data=recvData+rtp_header_size;        // 指针移动到 recvData缓冲区的有点载荷数据起始位置
                int rtp_data_size=pktsize-rtp_header_size;      // 有效载荷数据长度
                fwrite(rtp_data,rtp_data_size,1,fp1);           // 将有效载荷数据写入输出文件

                //Parse MPEGTS
                if(parse_mpegts!=0&&payload==33){
                    MPEGTS_FIXED_HEADER mpegts_header;
                    // 每个MPEG-TS数据包的大小通常为188字节
                    for(int i=0;i<rtp_data_size;i=i+188){
                        // 判断每个MPEG-TS数据包的第一个字节是否是同步字节  0x47
                        if(rtp_data[i]!=0x47) {
                            break;
                        }
                        //MPEGTS Header
                        memcpy((void *)&mpegts_header,rtp_data+i,sizeof(MPEGTS_FIXED_HEADER));
//                        fprintf(myout,"   [MPEGTS Pkt]\n");
                    }
                }

            }else{
                // parse mpegts
                fprintf(myout,"[UDP Pkt] %5d| %5d|\n",cnt,pktsize);
                fwrite(recvData,pktsize,1,fp1);
            }

            cnt++;
        } else { printf("time out\n"); break;}
    }
    closesocket(serSocket);
    WSACleanup();
    fclose(fp1);

    return 0;
}

// 运行不了的话，就手动链接 lws2_32
// gcc udp_rtp.cpp -o udp_rtp -lws2_32
int main(int argc, char *argv[]){
    simplest_udp_parser(8880);
}
