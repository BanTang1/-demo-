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

//Important!
#pragma pack(1)


#define TAG_TYPE_SCRIPT 18
#define TAG_TYPE_AUDIO  8
#define TAG_TYPE_VIDEO  9

typedef unsigned char byte;
typedef u_int32_t uint;

typedef struct {
    byte Signature[3];          // FLV
    byte Version;               // 版本号
    byte Flags;                 // 类型 （是否有音频和视频）
    uint DataOffset;            // FLV Header Size = 9
} FLV_HEADER;

typedef struct {
    byte TagType;               // TAG 类型，  Audio、Video、Script
    byte DataSize[3];           // 表示TAG DATA 部分的大小
    byte Timestamp[3];          // 时间戳
    uint Reserved;              // 此处最高位包含了一个时间戳扩展字节， 之后的三位才是Stream ID（总为0）
} TAG_HEADER;


//reverse_bytes - turn a BigEndian byte array into a LittleEndian integer
// 大端序转小端序  ,FLV文件数据通常是以大端序存储的，而我们的电脑一般是小端序存储
uint reverse_bytes(byte *p, char c) {
    int r = 0;
    int i;
    for (i=0; i<c; i++)
        r |= ( *(p+i) << (((c-1)*8)-8*i));
    return r;
}

/**
 * Analysis FLV file
 * @param url    Location of input FLV file.
 */

int simplest_flv_parser(char *url){

    //whether output audio/video stream
    int output_a=1;
    int output_v=1;
    //-------------
    FILE *ifh=NULL,*vfh=NULL, *afh = NULL;

    //FILE *myout=fopen("output_log.txt","wb+");
    FILE *myout=stdout;

    FLV_HEADER flv;
    TAG_HEADER tagheader;
    uint previoustagsize, previoustagsize_z=0;
    uint ts=0, ts_new=0;

    ifh = fopen(url, "rb+");
    if ( ifh== NULL) {
        printf("Failed to open files!");
        return -1;
    }

    //FLV file header   直接读取9个字节   ，这就是FLV的Header
    fread((char *)&flv,1,sizeof(FLV_HEADER),ifh);

    fprintf(myout,"============== FLV Header ==============\n");
    fprintf(myout,"Signature:  0x %c %c %c\n",flv.Signature[0],flv.Signature[1],flv.Signature[2]);
    fprintf(myout,"Version:    0x %X\n",flv.Version);
    fprintf(myout,"Flags  :    0x %X\n",flv.Flags);
    fprintf(myout,"HeaderSize: 0x %X\n",reverse_bytes((byte *)&flv.DataOffset, sizeof(flv.DataOffset)));
    fprintf(myout,"========================================\n");

    //move the file pointer to the end of the header
    fseek(ifh, reverse_bytes((byte *)&flv.DataOffset, sizeof(flv.DataOffset)), SEEK_SET);
    /**
     *  本地文件流中的实际指 ： 00 00 00 09  --> 本地是 Little-Endian
     *  move1         ： 150994944 -> 09 00 00 00  -> FLV流中数据是以 Big-endian 存储的
     *  move2         :  9  -> 00 00 00 09 -> 转换成 Little-endian 之后，是我们的期望值
     */
//    printf("zh___ move1 = %d\n", flv.DataOffset);
//    printf("zh___ move2 = %d\n", reverse_bytes((byte *)&flv.DataOffset, sizeof(flv.DataOffset)));

// ===================================FLV Header 处理完毕=======================================================================

    //process each tag
    do {
        // Previous Tag Size     getw() 取一个整数（4字节） ，并将指针向前移动过去
        previoustagsize = getw(ifh);
        // 第一个Previous Tag Size = 0  , 第二个 Previous Tag Size = 562   ✔
//        printf("zh___previoustagsize = %d\n", reverse_bytes((byte *)&previoustagsize, sizeof(previoustagsize)));

        // 读取Tag Header，也就是长度 = 11 字节
        fread((void *)&tagheader,sizeof(TAG_HEADER),1,ifh);

        //int temp_datasize1=reverse_bytes((byte *)&tagheader.DataSize, sizeof(tagheader.DataSize));
        // 这实际上是将一个3字节的大端序整数转换为一个int类型的值 （第一次进来，也就是Script的值是 551 ）
        int tagheader_datasize=tagheader.DataSize[0]*65536+tagheader.DataSize[1]*256+tagheader.DataSize[2];
        // 与上面类似，取出时间戳的 int值 （第一次进来，值为 0）
        int tagheader_timestamp=tagheader.Timestamp[0]*65536+tagheader.Timestamp[1]*256+tagheader.Timestamp[2];

        // 解析刚刚读取进去的 Tag Header ： -->  type (1字节) ：Audio(0x08)、Video（0x09）、Script Data(0x12)
        char tagtype_str[10];
        switch(tagheader.TagType){
            case TAG_TYPE_AUDIO:sprintf(tagtype_str,"AUDIO");break;
            case TAG_TYPE_VIDEO:sprintf(tagtype_str,"VIDEO");break;
            case TAG_TYPE_SCRIPT:sprintf(tagtype_str,"SCRIPT");break;
            default:sprintf(tagtype_str,"UNKNOWN");break;
        }

        fprintf(myout,"[%6s] %6d %6d %6d |",tagtype_str,tagheader_datasize,tagheader_timestamp,reverse_bytes((byte *)&previoustagsize, sizeof(previoustagsize)));

        //if we are not past the end of file, process the tag
        if (feof(ifh)) {
            break;
        }

        //process tag by type
        switch (tagheader.TagType) {

            case TAG_TYPE_AUDIO:{
                char audiotag_str[100]={0};
                strcat(audiotag_str,"| ");
                char tagdata_first_byte;
                tagdata_first_byte=fgetc(ifh);
                // 看一眼 这个值到底是多少
//                printf("zh___info = %d\n",tagdata_first_byte);
                int x=tagdata_first_byte&0xF0;      // 前四位保留， 后四位归零
                x=x>>4;
                switch (x)  // 前四位：音频格式
                {
                    case 0:strcat(audiotag_str,"Linear PCM, platform endian");break;
                    case 1:strcat(audiotag_str,"ADPCM");break;
                    case 2:strcat(audiotag_str,"MP3");break;
                    case 3:strcat(audiotag_str,"Linear PCM, little endian");break;
                    case 4:strcat(audiotag_str,"Nellymoser 16-kHz mono");break;
                    case 5:strcat(audiotag_str,"Nellymoser 8-kHz mono");break;
                    case 6:strcat(audiotag_str,"Nellymoser");break;
                    case 7:strcat(audiotag_str,"G.711 A-law logarithmic PCM");break;
                    case 8:strcat(audiotag_str,"G.711 mu-law logarithmic PCM");break;
                    case 9:strcat(audiotag_str,"reserved");break;
                    case 10:strcat(audiotag_str,"AAC");break;
                    case 11:strcat(audiotag_str,"Speex");break;
                    case 14:strcat(audiotag_str,"MP3 8-Khz");break;
                    case 15:strcat(audiotag_str,"Device-specific sound");break;
                    default:strcat(audiotag_str,"UNKNOWN");break;
                }
                strcat(audiotag_str,"| ");
                x=tagdata_first_byte&0x0C;
                x=x>>2;
                switch (x)      // 接下来两位 ：采样率
                {
                    case 0:strcat(audiotag_str,"5.5-kHz");break;
                    case 1:strcat(audiotag_str,"1-kHz");break;
                    case 2:strcat(audiotag_str,"22-kHz");break;
                    case 3:strcat(audiotag_str,"44-kHz");break;
                    default:strcat(audiotag_str,"UNKNOWN");break;
                }
                strcat(audiotag_str,"| ");
                x=tagdata_first_byte&0x02;
                x=x>>1;
                switch (x)      // 采样精度
                {
                    case 0:strcat(audiotag_str,"8Bit");break;
                    case 1:strcat(audiotag_str,"16Bit");break;
                    default:strcat(audiotag_str,"UNKNOWN");break;
                }
                strcat(audiotag_str,"| ");
                x=tagdata_first_byte&0x01;
                switch (x)      // 声道类型
                {
                    case 0:strcat(audiotag_str,"Mono");break;
                    case 1:strcat(audiotag_str,"Stereo");break;
                    default:strcat(audiotag_str,"UNKNOWN");break;
                }
                fprintf(myout,"%s",audiotag_str);

                //if the output file hasn't been opened, open it.
                if(output_a!=0&&afh == NULL){
                    afh = fopen("output.mp3", "wb");
                }

                //TagData - First Byte Data
                // MP3 格式，直接将音频数据取出来即可
                int data_size=reverse_bytes((byte *)&tagheader.DataSize, sizeof(tagheader.DataSize))-1;
                if(output_a!=0){
                    //TagData+1
                    for (int i=0; i<data_size; i++)
                        fputc(fgetc(ifh),afh);

                }else{
                    for (int i=0; i<data_size; i++)
                        fgetc(ifh);
                }
                break;
            }
            case TAG_TYPE_VIDEO:{
                char videotag_str[100]={0};
                strcat(videotag_str,"| ");
                char tagdata_first_byte;
                tagdata_first_byte=fgetc(ifh);      // Tag Data 区域的 第一个字节包含了一些参数信息
                int x=tagdata_first_byte&0xF0;      // 前四位保留，后四位归零
                x=x>>4;
                switch (x)      // 前四个字节：帧类型
                {
                    case 1:strcat(videotag_str,"key frame  ");break;
                    case 2:strcat(videotag_str,"inter frame");break;
                    case 3:strcat(videotag_str,"disposable inter frame");break;
                    case 4:strcat(videotag_str,"generated keyframe");break;
                    case 5:strcat(videotag_str,"video info/command frame");break;
                    default:strcat(videotag_str,"UNKNOWN");break;
                }
                strcat(videotag_str,"| ");
                x=tagdata_first_byte&0x0F; // 取后四位
                switch (x)      // 后四个字节 ： 编码类型
                {
                    case 1:strcat(videotag_str,"JPEG (currently unused)");break;
                    case 2:strcat(videotag_str,"Sorenson H.263");break;
                    case 3:strcat(videotag_str,"Screen video");break;
                    case 4:strcat(videotag_str,"On2 VP6");break;
                    case 5:strcat(videotag_str,"On2 VP6 with alpha channel");break;
                    case 6:strcat(videotag_str,"Screen video version 2");break;
                    case 7:strcat(videotag_str,"AVC");break;
                    default:strcat(videotag_str,"UNKNOWN");break;
                }
                // 拼接输出
                fprintf(myout,"%s",videotag_str);

                // 回到  Tag Data 的开始位置
                fseek(ifh, -1, SEEK_CUR);
                //if the output file hasn't been opened, open it.
                if (vfh == NULL&&output_v!=0) {
                    //write the flv header (reuse the original file's hdr) and first previoustagsize
                    vfh = fopen("output.flv", "wb");
                    // 将FLV Header 写入 该文件
                    fwrite((char *)&flv,1, sizeof(flv),vfh);
                    // FLV 中， Previous Tag Size 表示前一个tag的长度，第一个tag的 Previous Tag Size 一定为 0
                    // 所以，第一次的时候，四个字节全为0！
                    fwrite((char *)&previoustagsize_z,1,sizeof(previoustagsize_z),vfh);
                }
#if 0
                //Change Timestamp
			//Get Timestamp
			ts = reverse_bytes((byte *)&tagheader.Timestamp, sizeof(tagheader.Timestamp));
			ts=ts*2;
			//Writeback Timestamp
			ts_new = reverse_bytes((byte *)&ts, sizeof(ts));
			memcpy(&tagheader.Timestamp, ((char *)&ts_new) + 1, sizeof(tagheader.Timestamp));
#endif


                //TagData + Previous Tag Size
                int data_size=reverse_bytes((byte *)&tagheader.DataSize, sizeof(tagheader.DataSize))+4;
                if(output_v!=0){
                    //TagHeader
                    fwrite((char *)&tagheader,1, sizeof(tagheader),vfh);
                    //TagData
                    // 前面 ifh 的文件指针停在了 TagHeader之后 ，此时从ifh中继续读取并写入到vfh文件中，
                    // 此时 ifh 文件指针 在Previous Tag Size位置，
                    // 为保证下一次能够正常解析，需在下一次解析到来前 ，将文件指针回退到Previous Tag Size开始的位置，
                    // 保证程序正常的运行
                    for (int i=0; i<data_size; i++)
                        fputc(fgetc(ifh),vfh);
                }else{
                    for (int i=0; i<data_size; i++)
                        fgetc(ifh);
                }
                //rewind 4 bytes, because we need to read the previoustagsize again for the loop's sake
                fseek(ifh, -4, SEEK_CUR);

                break;
            }
            default:
                //skip the data of this tag
                // 也就是Script Data 区域， 跳过
                fseek(ifh, reverse_bytes((byte *)&tagheader.DataSize, sizeof(tagheader.DataSize)), SEEK_CUR);
        }

        fprintf(myout,"\n");

    } while (!feof(ifh));


    fcloseall();

    return 0;
}

int main(int argc, char *argv[]) {
    simplest_flv_parser("cuc_ieschool.flv");
}