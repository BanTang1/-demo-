/**
* AAC原始码流（又称为“裸流”）是由一个一个的ADTS frame组成的。
 * 结构： ...|ADTS frame|ADTS frame|ADTS frame|......
 * 其中每个ADTS frame之间通过syncword（同步字）进行分隔。同步字为0xFFF（二进制“111111111111”）。
 * AAC码流解析的步骤就是首先从码流中搜索0x0FFF，分离出ADTS frame；然后再分析ADTS frame的首部各个字段。
 * 本文的程序即实现了上述的两个步骤。
版权声明：本文为CSDN博主「雷霄骅」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/leixiaohua1020/article/details/50535042
*/


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


int getADTSframe(unsigned char* buffer, int buf_size, unsigned char* data ,int* data_size){
    int size = 0;

    if(!buffer || !data || !data_size ){
        return -1;
    }

    while(1){
        // ADTS header占56bit，也就是7个byte
        if(buf_size  < 7 ){
            return -1;
        }
        //Sync words
        // 可查看：https://www.cnblogs.com/daner1257/p/10709233.html#:~:text=ID%EF%BC%9A%E8%A1%A8%E7%A4%BA%E4%BD%BF%E7%94%A8%E7%9A%84MPEG%E7%9A%84%E7%89%88%E6%9C%AC%EF%BC%8C0%E8%A1%A8%E7%A4%BAMPEG-4%EF%BC%8C1%E8%A1%A8%E7%A4%BAMPEG-2,layer%EF%BC%9A%E5%90%8Csyncword%EF%BC%8C%E5%80%BC%E5%9B%BA%E5%AE%9A%EF%BC%8C%E9%83%BD%E6%98%AF00%20protection_absent%EF%BC%9A%E6%98%AF%E5%90%A6%E6%9C%89%E5%90%8C%E6%AD%A5%E6%A0%A1%E9%AA%8C%EF%BC%8C%E5%A6%82%E6%9E%9C%E6%9C%89%E5%80%BC%E6%98%AF0%EF%BC%8C%E6%B2%A1%E6%9C%89%E6%98%AF1
        // 在AAC编码中，同步字节是用来标识帧头在比特流中的位置的。
        // 在AAC的ADTS（Audio Data Transport Stream）格式中，同步字为12比特的“1111 1111 1111”   0xFFF
        // buffer[0] == 0xff  : 判断第一个字节是不是为FF
        // (buffer[1] & 0xf0) == 0xf0 ： 将第二个字节的前四位取出来，后四位置为0 ， 与0xf0比较，可以判断是前四位是否为0xF
        // 因此此判断条件可以判断出 同步字节
        if((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0) ){
            size |= ((buffer[3] & 0x03) <<11);     //high 2 bit
            size |= buffer[4]<<3;                //middle 8 bit
            size |= ((buffer[5] & 0xe0)>>5);        //low 3bit
            break;
        }
        --buf_size;
        ++buffer;
    }
    if(buf_size < size){
        return 1;
    }

    memcpy(data, buffer, size);
    *data_size = size;

    return 0;
}

int simplest_aac_parser(char *url)
{
    int data_size = 0;
    int size = 0;
    int cnt=0;
    int offset=0;

    //FILE *myout=fopen("output_log.txt","wb+");
    FILE *myout=stdout;

    // aacframe是用来存储单个AAC帧的数据。一般来说，一个AAC帧的数据量不会超过1024*5字节。所以，这里分配了1024*5字节的内存给aacframe。
    unsigned char *aacframe=(unsigned char *)malloc(1024*5);
    /*
     * aacbuffer是用来存储从文件中读取的所有AAC数据。由于一个AAC文件可能会包含很多帧，所以需要更大的内存来存储。
     * 这里分配了1024*1024字节（即1MB）的内存给aacbuffer，以确保有足够的空间来存储从文件中读取的数据。
     */
    unsigned char *aacbuffer=(unsigned char *)malloc(1024*1024);

    FILE *ifile = fopen(url, "rb");
    if(!ifile){
        printf("Open file error");
        return -1;
    }

    printf("-----+- ADTS Frame Table -+------+\n");
    printf(" NUM | Profile | Frequency| Size |\n");
    printf("-----+---------+----------+------+\n");

    while(!feof(ifile)){
        // 返回读取的数据的大小， 此处为整个文件的大小 480002 byte
        data_size = fread(aacbuffer+offset, 1, 1024*1024-offset, ifile);
        unsigned char* input_data = aacbuffer;

        while(1)
        {
            int ret=getADTSframe(input_data, data_size, aacframe, &size);
            if(ret==-1){
                break;
            }else if(ret==1){
                memcpy(aacbuffer,input_data,data_size);
                offset=data_size;
                break;
            }

            //  用于存储音频配置文件（Profile）的信息。
            //  在AAC编码中，Profile描述了音频的编码复杂性。例如，Main、LC（Low Complexity）和 SSR（Scalable Sample Rate）等。
            char profile_str[10]={0};
            //  用于存储采样频率（Sampling Frequency）的信息。采样频率是指每秒钟对声音信号进行采样的次数。
            char frequence_str[10]={0};

            //  取出 profile  , 第二个字节的头两位，共2位
            unsigned char profile=aacframe[2]&0xC0;
            //  右移 6位  ，取出对应2位的的实际数值
            profile=profile>>6;
            switch(profile){
                case 0: sprintf(profile_str,"Main");break;
                case 1: sprintf(profile_str,"LC");break;
                case 2: sprintf(profile_str,"SSR");break;
                default:sprintf(profile_str,"unknown");break;
            }
            // 取出 sampling_frequency_index , 第二个字节的 3-6位置  共4位
            unsigned char sampling_frequency_index=aacframe[2]&0x3C;
            // 右移 2位 ， 取出对应4位的实际数值
            sampling_frequency_index=sampling_frequency_index>>2;
            switch(sampling_frequency_index){
                case 0: sprintf(frequence_str,"96000Hz");break;
                case 1: sprintf(frequence_str,"88200Hz");break;
                case 2: sprintf(frequence_str,"64000Hz");break;
                case 3: sprintf(frequence_str,"48000Hz");break;
                case 4: sprintf(frequence_str,"44100Hz");break;
                case 5: sprintf(frequence_str,"32000Hz");break;
                case 6: sprintf(frequence_str,"24000Hz");break;
                case 7: sprintf(frequence_str,"22050Hz");break;
                case 8: sprintf(frequence_str,"16000Hz");break;
                case 9: sprintf(frequence_str,"12000Hz");break;
                case 10: sprintf(frequence_str,"11025Hz");break;
                case 11: sprintf(frequence_str,"8000Hz");break;
                default:sprintf(frequence_str,"unknown");break;
            }


            fprintf(myout,"%5d| %8s|  %8s| %5d|\n",cnt,profile_str ,frequence_str,size);
            data_size -= size;
            input_data += size;
            cnt++;
        }

    }
    fclose(ifile);
    free(aacbuffer);
    free(aacframe);

    return 0;
}



int main(int argc, char *argv[]){
    simplest_aac_parser("nocturne.aac");
}