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
#include <stdlib.h>
#include <string.h>

// 1～12由H.264使⽤，24～31由H.264以外的应⽤
typedef enum {
    NALU_TYPE_SLICE = 1,                // 非IDR图像的片（Slice）。也就是P帧或B帧
    NALU_TYPE_DPA = 2,                  // 已经在H.264标准中被废弃，不再使用。
    NALU_TYPE_DPB = 3,                  // 已经在H.264标准中被废弃，不再使用。
    NALU_TYPE_DPC = 4,                  // 已经在H.264标准中被废弃，不再使用。
    NALU_TYPE_IDR = 5,                  // 即时解码刷新（Instantaneous Decoding Refresh）图像的片（Slice）。也就是I帧。
    NALU_TYPE_SEI = 6,                  // 补充增强信息（Supplemental enhancement information）。这是一种用于传输额外信息的NALU类型
    NALU_TYPE_SPS = 7,                  // 序列参数集（Sequence parameter set）. 这包含了一组编码视频序列所需的参数
    NALU_TYPE_PPS = 8,                  // 图像参数集（Picture parameter set）. 这包含了一组编码单个图像所需的参数
    NALU_TYPE_AUD = 9,                  // 访问单元分隔符（Access unit delimiter）。这是一个可选的NALU，用于标识一个新的接入单元(AU)的开始.
    NALU_TYPE_EOSEQ = 10,               // 序列结束（End of sequence）。这表示一个视频序列的结束。
    NALU_TYPE_EOSTREAM = 11,            // 流结束（End of stream）。这表示视频流的结束
    NALU_TYPE_FILL = 12,                // 填充数据（Filler data）。这是一种不包含任何信息，只用于填充比特率的NALU类型
} NaluType;

//在H264编码中，NALU是视频数据的最小单位。每个NALU都有一个IDC（nal_ref_idc）字段，这个字段表示NALU的重要性。
typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,       // 表示这个NALU是可丢弃的，也就是说，即使解码器丢弃了这个NALU，也不会影响到图像的回放。
    NALU_PRIRITY_LOW = 1,               // 这个NALU的优先级较低
    NALU_PRIORITY_HIGH = 2,             // 这个NALU的优先级较高
    NALU_PRIORITY_HIGHEST = 3,          // 这个NALU的优先级最高
} NaluPriority;


typedef struct {
    int startcodeprefix_len;      //! 起始码前缀的长度。对于参数集和图片中的第一个切片，这个值通常为4；对于其他所有内容，这个值通常为3
    unsigned len;                 //! NALU的长度（不包括起始码，因为起始码不属于NALU）
    unsigned max_size;            //! NALU缓冲区的大小
    int forbidden_bit;            //! 禁止位，应该总是为FALSE。
    int nal_reference_idc;        //! 表示NALU的优先级，对应于之前定义的 NaluPriority 枚举类型。
    int nal_unit_type;            //! 表示NALU的类型。对应于之前定义的 NaluType 枚举类型。
    char *buf;                    //! 包含了第一个字节（静止位：1位，优先级：2，类型：5位）以及后续的EBSP（原始字节序列载荷）
} NALU_t;

FILE *h264bitstream = NULL;                // 文件流

int info2 = 0, info3 = 0;

static int FindStartCode2(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1) return 0; //0x000001?
    else return 1;
}

static int FindStartCode3(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1) return 0;//0x00000001?
    else return 1;
}


int GetAnnexbNALU(NALU_t *nalu) {
    // 已读取的字节长度
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;

    // 分配 nalu中设置的最大缓冲区大小的内存空间
    if ((Buf = (unsigned char *) calloc(nalu->max_size, sizeof(char))) == NULL)
        printf("GetAnnexbNALU: Could not allocate Buf memory\n");

    // 初始化起始码长度 (假设为3字节)
    nalu->startcodeprefix_len = 3;

    // fread() 方法如果成功会返回 读取的字节数，否则则代表失败
    // 此时已经从.h264文件中读取了三个字节到Buf缓冲区中
    if (3 != fread(Buf, 1, 3, h264bitstream)) {
        free(Buf);
        return 0;
    }
    // 判断起始码 是否是  3字节  0x000001?
    info2 = FindStartCode2(Buf);
    if (info2 != 1) {  // 如果不是3字节， 就再从文件中读取一个字节
        if (1 != fread(Buf + 3, 1, 1, h264bitstream)) {
            free(Buf);
            return 0;
        }
        // 判断是否为4字节
        info3 = FindStartCode3(Buf);
        if (info3 != 1) { // 如果不是4字节，出现异常情况，结束程序
            free(Buf);
            return -1;
        } else {
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    } else {
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }
    StartCodeFound = 0;
    info2 = 0;
    info3 = 0;
    // 寻找下一个起始码
    while (!StartCodeFound) {
        // 指针到达文件末尾， 不需要再找了
        if (feof(h264bitstream)) {
            nalu->len = (pos - 1) - nalu->startcodeprefix_len;
            memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
            // 抛开起始码的第一个字节
            nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
            nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
            free(Buf);
            return pos - 1;
        }
        // fgetc 读取一个无符号字符并将文件指针向前推进   （如果读取完最后一个字符，pos依然会自增，因此要表示实际pos的值需要 -1）
        Buf[pos++] = fgetc(h264bitstream);
        info3 = FindStartCode3(&Buf[pos - 4]);
        if (info3 != 1)
            info2 = FindStartCode2(&Buf[pos - 3]);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }

    // 在这里，我们找到了另一个启动代码（并且读取启动代码字节的长度超过我们应该有的长度。因此，返回文件）
    // 此时，找到了下一个起始码， 需要减去起始码的长度，才是一个NALU该有的长度， 保存此值
    rewind = (info3 == 1) ? -4 : -3;

    // 将文件指针移动到下一个起始码之前 （这才是一个NALU）
    if (0 != fseek(h264bitstream, rewind, SEEK_CUR)) {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf.
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code
    // 此处的开始代码、完整的 NALU 和下一个启动代码位于 Buf 中。
    // Buf 的大小是 pos，pos+rewind 是不包括下一个启动代码的字节数，
    // （pos+rewind）-startcodeprefix_len 是不包括起始码的 NALU 的大小
    nalu->len = (pos + rewind) - nalu->startcodeprefix_len;
    memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
    free(Buf);

    return (pos + rewind);
}

/**
 * Analysis H.264 Bitstream
 * @param url    Location of input H.264 bitstream file.
 */
int simplest_h264_parser(char *url) {

    NALU_t *n;
    //  缓冲区大小  字节
    int buffersize = 100000;

    //FILE *myout=fopen("output_log.txt","wb+");
    // 打开一个标准输出流
    FILE *myout = stdout;

    h264bitstream = fopen(url, "rb+");
    if (h264bitstream == NULL) {
        printf("Open file error\n");
        return 0;
    }
    // 分配内存并初始化
    n = (NALU_t *) calloc(1, sizeof(NALU_t));
    if (n == NULL) {
        printf("Alloc NALU Error\n");
        return 0;
    }

    n->max_size = buffersize;
    n->buf = (char *) calloc(buffersize, sizeof(char));
    if (n->buf == NULL) {
        free(n);
        printf("AllocNALU: n->buf");
        return 0;
    }

    int data_offset = 0;
    int nal_num = 0;
    printf("-----+-------- NALU Table ------+---------+\n");
    printf(" NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
    printf("-----+---------+--------+-------+---------+\n");

    while (!feof(h264bitstream)) {
        int data_lenth;
        data_lenth = GetAnnexbNALU(n);
        char type_str[20] = {0};
        switch (n->nal_unit_type) {
            case NALU_TYPE_SLICE:
                sprintf(type_str, "SLICE");
                break;
            case NALU_TYPE_DPA:
                sprintf(type_str, "DPA");
                break;
            case NALU_TYPE_DPB:
                sprintf(type_str, "DPB");
                break;
            case NALU_TYPE_DPC:
                sprintf(type_str, "DPC");
                break;
            case NALU_TYPE_IDR:
                sprintf(type_str, "IDR");
                break;
            case NALU_TYPE_SEI:
                sprintf(type_str, "SEI");
                break;
            case NALU_TYPE_SPS:
                sprintf(type_str, "SPS");
                break;
            case NALU_TYPE_PPS:
                sprintf(type_str, "PPS");
                break;
            case NALU_TYPE_AUD:
                sprintf(type_str, "AUD");
                break;
            case NALU_TYPE_EOSEQ:
                sprintf(type_str, "EOSEQ");
                break;
            case NALU_TYPE_EOSTREAM:
                sprintf(type_str, "EOSTREAM");
                break;
            case NALU_TYPE_FILL:
                sprintf(type_str, "FILL");
                break;
        }
        char idc_str[20] = {0};
        switch (n->nal_reference_idc >> 5) {
            case NALU_PRIORITY_DISPOSABLE:
                sprintf(idc_str, "DISPOS");
                break;
            case NALU_PRIRITY_LOW:
                sprintf(idc_str, "LOW");
                break;
            case NALU_PRIORITY_HIGH:
                sprintf(idc_str, "HIGH");
                break;
            case NALU_PRIORITY_HIGHEST:
                sprintf(idc_str, "HIGHEST");
                break;
        }

        fprintf(myout, "%5d| %8d| %7s| %6s| %8d|\n", nal_num, data_offset, idc_str, type_str, n->len);

        data_offset = data_offset + data_lenth;
        nal_num++;
    }

    //Free
    if (n) {
        if (n->buf) {
            free(n->buf);
            n->buf = NULL;
        }
        free(n);
    }
    return 0;
}


int main(int argc, char *argv[]) {
    simplest_h264_parser("sintel.h264");
}