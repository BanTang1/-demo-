#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

//分离PCM16LE双声道音频采样数据的左声道和右声道
// PCM 双声道存储方式 : (l0,r0) (l1,r1) .......
// 16  表示每个采样点占用16位
// LE Little Endian  小端序存储
int simplest_pcm16le_split(const char *url) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_l.pcm", "wb+");
    FILE *fp2 = fopen("output_r.pcm", "wb+");

//    unsigned char *sample = (unsigned char *) malloc(4);
    auto *sample = (unsigned char *) malloc(4);

    while (!feof(fp)) {
        fread(sample, 1, 4, fp);
        //L
        fwrite(sample, 1, 2, fp1);
        //R
        fwrite(sample + 2, 1, 2, fp2);
    }

    free(sample);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    return 0;
}


// 将PCM16LE双声道音频采样数据中左声道的音量降一半
/**
 * Halve volume of Left channel of 16LE PCM file
 * @param url  Location of PCM file.
 */
int simplest_pcm16le_halfvolumeleft(char *url) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_halfleft.pcm", "wb+");

    int cnt = 0;

    unsigned char *sample = (unsigned char *) malloc(4);

    while (!feof(fp)) {
        // short 占用连个字节
        short *samplenum = NULL;
        fread(sample, 1, 4, fp);
        // 只会指向 sample 中的前两个字节
        samplenum = (short *) sample;
        // 因此修改的只是前两个字节的值 , 也就是左声道的值
        *samplenum = *samplenum / 2;
        // 将左声道音量设置为0 ，实际现象是左耳机没有声音，右耳机正常 ↓
//        *samplenum = 0;
        //L
        fwrite(sample, 1, 2, fp1);
        //R
        fwrite(sample + 2, 1, 2, fp1);

        cnt++;
    }
    printf("Sample Cnt:%dKB\n", cnt * 4 / 1024);

    free(sample);
    fclose(fp);
    fclose(fp1);
    return 0;
}

//将PCM16LE双声道音频采样数据的声音速度提高一倍
// 只采样每个声道奇数点的样值
int simplest_pcm16le_doublespeed(char *url) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_doublespeed.pcm", "wb+");
    int cnt = 0;
    auto *sample = (unsigned char *) malloc(4);
    while (!feof(fp)) {
        fread(sample, 1, 4, fp);
        if ((cnt & 2) != 0) {
            fwrite(sample, 1, 4, fp1);
        }
        cnt++;
    }
    free(sample);
    fclose(fp);
    fclose(fp1);
    return 0;
}

//将PCM16LE双声道音频采样数据转换为PCM8音频采样数据
/**
 * Convert PCM-16 data to PCM-8 data.
 * @param url  Location of PCM file.
 */
int simplest_pcm16le_to_pcm8(char *url) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_8.pcm", "wb+");
    int cnt = 0;
    auto *sample = (unsigned char *) malloc(4);
    while (!feof(fp)) {
        short *samplenum16 = nullptr;
        char samplenum8 = 0;
        unsigned samplenum8_u = 0;
        fread(sample, 1, 4, fp);
        // short 两个字节    16位取值范围(-32768-32767)
        samplenum16 = (short *) sample;
        // 右移8位 将采样值转换为8位  , 有符号
        samplenum8 = (*samplenum16) >> 8;
        // 转换为无符号
        samplenum8_u = samplenum8 + 128;
        // L 写入
        fwrite(&samplenum8_u, 1, 1, fp1);
        // 找到 R声道对应的字节
        samplenum16 = (short *) (sample + 2);
        samplenum8 = *samplenum16 >> 8;
        samplenum8_u = samplenum8 + 128;
        fwrite(&samplenum8_u, 1, 1, fp1);
        cnt++;
    }
    printf("Sample Cnt:%d\n", cnt);
    free(sample);
    fclose(fp);
    fclose(fp1);
    return 0;
}


// 将从PCM16LE单声道音频采样数据中截取一部分数据
/**
 * Cut a 16LE PCM single channel file.
 * @param url        Location of PCM file.
 * @param start_num  start point
 * @param dur_num    how much point to cut
 */
int simplest_pcm16le_cut_singlechannel(char *url, int start_num, int dur_num) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_cut.pcm", "wb+");
    FILE *fp_stat = fopen("output_cut.txt", "wb+");

    unsigned char *sample = (unsigned char *) malloc(2);

    int cnt = 0;
    while (!feof(fp)) {
        fread(sample, 1, 2, fp);
        if (cnt > start_num && cnt <= (start_num + dur_num)) {
            fwrite(sample, 1, 2, fp1);

            // little-endian 格式存储 ，低字节（即 sample[0]）和高字节（即 sample[1]）
            // “little-endian” 格式的音频样本时，我们需要交换字节的顺序才能正确计算样本值
            // 例如 16 位整数 0x1234，它被存储在内存地址 0x1000 和 0x1001 处；
            // 在 little-endian 格式中，低字节 0x34 会被存储在地址 0x1000 处，而高字节 0x12 会被存储在地址 0x1001 处;
            // 我的电脑本身是采用 little-endian格式存储的，因此需要交换字节顺序，才能正确计算样本值
            // 取出sample 高字节中的值
            short samplenum = sample[1];
            // <<8   右边放到左边
            samplenum = samplenum * 256;
            //  加上原本左边的值   ，即正确交换字节顺序
            samplenum = samplenum + sample[0];
            // %6d 6个字符宽度的整数， 方便对其以便于阅读
            fprintf(fp_stat, "%6d,", samplenum);
            // 一行显示10个
            if (cnt % 10 == 0)
                fprintf(fp_stat, "\n", samplenum);
        }
        cnt++;
    }

    printf("cnt:%d", cnt);

    free(sample);
    fclose(fp);
    fclose(fp1);
    fclose(fp_stat);
    return 0;
}


//将PCM16LE双声道音频采样数据转换为WAVE格式音频数据
/**
 * Convert PCM16LE raw data to WAVE format
 * @param pcmpath      Input PCM file.
 * @param channels     Channel number of PCM file.
 * @param sample_rate  Sample rate of PCM file.
 * @param wavepath     Output WAVE file.
 */
// 值得注意的是：！！！！！  最好使用平台无关类型，如 uint16_t、uint32_t、uint64_t 等等
int simplest_pcm16le_to_wave(const char *pcmpath, int channels, int sample_rate, const char *wavepath) {

    typedef struct WAVE_HEADER {
        char fccID[4];                      // 用于存储 RIFF chunk 的标识符。对于 WAV 文件，这个值通常为 "RIFF"
        uint32_t dwSize;               // 文件的总大小, 不包括 fccID 和 dwSize 本身
        char fccType[4];                    // 用于存储文件的类型。对于 WAV 文件，这个值通常为 "WAVE"
    } WAVE_HEADER;

    typedef struct WAVE_FMT {
        char fccID[4];                      // 用于存储 Format chunk 的标识符。对于 WAV 文件，这个值通常为 "fmt "
        uint32_t dwSize;               // Format chunk 的大小（不包括 fccID 和 dwSize 本身）2+2+4+4+2+2=16
        uint16_t wFormatTag;          // 表示音频数据的格式。对于 PCM 数据，这个值为 1
        uint16_t wChannels;           // 表示音频数据的声道数.对于立体声数据，这个值为 2
        uint32_t dwSamplesPerSec;      // 表示音频数据的采样率,  CD 音质的音频数据，这个值为 44100
        uint32_t dwAvgBytesPerSec;     // 表示音频数据的平均字节数每秒。这个值等于 dwSamplesPerSec * wBlockAlign
        uint16_t wBlockAlign;         // 表示音频数据的块对齐大小。这个值等于 (uiBitsPerSample * wChannels) / 8
        uint16_t uiBitsPerSample;     // 表示音频数据的位深度，此处为16
    } WAVE_FMT;

    typedef struct WAVE_DATA {
        char fccID[4];                      // 用于存储 Data chunk 的标识符。对于 WAV 文件，这个值通常为 "data"
        uint32_t dwSize;               // 表示 Data chunk 的大小（不包括 fccID 和 dwSize 本身），即音频数据的字节数
    } WAVE_DATA;

    // 初始化文件
    FILE *fp, *fpout;
    fp = fopen(pcmpath, "rb");
    fpout = fopen(wavepath, "wb+");
    if (fp == nullptr || fpout == nullptr) {
        printf("open pcm file error or create wav file error\n");
    }

    // 实例化 三个结构体
    WAVE_HEADER pcmHEADER;
    WAVE_FMT pcmFORMAT;
    WAVE_DATA pcmData;

    unsigned short m_pcmData;

    // WAVE_HEADER
    memcpy(pcmHEADER.fccID, "RIFF", strlen("RIFF"));
    memcpy(pcmHEADER.fccType, "WAVE", strlen("WAVE"));
    fseek(fpout, sizeof(WAVE_HEADER), SEEK_SET);       // 将WAVE_HEADER 的位置预留出来

    // WAVE_FMT
    memcpy(pcmFORMAT.fccID, "fmt ", strlen("fmt "));  // ""fmt " 保证四个字节
    pcmFORMAT.dwSize = 16;
    pcmFORMAT.wFormatTag = 1;
    pcmFORMAT.wChannels = channels;
    pcmFORMAT.dwSamplesPerSec = sample_rate;
    pcmFORMAT.uiBitsPerSample = 16;
    pcmFORMAT.wBlockAlign = pcmFORMAT.uiBitsPerSample * pcmFORMAT.wChannels / 8;
    pcmFORMAT.dwAvgBytesPerSec = pcmFORMAT.dwSamplesPerSec * pcmFORMAT.wBlockAlign;

    // WAVE_FMT 数据拼凑完成，可以先写入文件对应位置
    fwrite(&pcmFORMAT, sizeof(WAVE_FMT), 1, fpout);

    // WAVE_DATA
    memcpy(pcmData.fccID, "data", strlen("data"));
    pcmData.dwSize = 0;
    fseek(fpout, sizeof(WAVE_DATA), SEEK_CUR);       // 将 WAVE_DATA的位置预留出来

    // 计算 pcm数据的大小，并将数据先写入文件对应位置
    /**
     *  feof判断文件结束是通过读取函数fread/fscanf等返回错误来识别的，故而判断文件是否结束应该是在读取函数之后进行判断。
     *  比如，在while循环读取一个文件时，如果是在读取函数之前进行判断，则如果文件最后一行是空白行，可能会造成内存错误。
     *  因此，就应该这样写，如果按照之前的写法会多两个奇奇怪怪的字节!!!!!!
     */
    fread(&m_pcmData, sizeof(uint16_t), 1, fp);
    while (!feof(fp)) {
        pcmData.dwSize += 2;
        fwrite(&m_pcmData, sizeof(uint16_t), 1, fpout);
        fread(&m_pcmData, sizeof(uint16_t), 1, fp);
    }

    // 计算 pcmHEADER.dwSize 的值
    pcmHEADER.dwSize = 44 + pcmData.dwSize - 8;

    // 重置文件指针 ，将指针移动到文件的开头
    rewind(fpout);

    // 将还未写入的块写入文件对应的位置 （FORMAT 与 PCM 数据已经写入了）
    fwrite(&pcmHEADER, sizeof(WAVE_HEADER), 1, fpout);
    fseek(fpout, sizeof(WAVE_FMT), SEEK_CUR);
    fwrite(&pcmData, sizeof(WAVE_DATA), 1, fpout);

    fclose(fp);
    fclose(fpout);

    return 0;
}


// 查看自己的电脑是小端字节序还是大端字节序
int isLEorBE() {
    union {
        int n;
        char ch;
    } data;
    data.n = 1;
    if (data.ch == 1) {
        printf("Little-endian\\n");
    } else {
        printf("Big-endian\\n");
    }
}

int main(int argc, char *argv[]) {
//    simplest_pcm16le_split("NocturneNo2inEflat_44.1k_s16le.pcm");
//    simplest_pcm16le_halfvolumeleft("NocturneNo2inEflat_44.1k_s16le.pcm");
//    simplest_pcm16le_doublespeed("NocturneNo2inEflat_44.1k_s16le.pcm");
//    simplest_pcm16le_to_pcm8("NocturneNo2inEflat_44.1k_s16le.pcm");
    simplest_pcm16le_cut_singlechannel("drum.pcm",2360,120);
//    simplest_pcm16le_to_wave("NocturneNo2inEflat_44.1k_s16le.pcm", 2, 44100, "output_nocturne.wav");

    return 0;
}