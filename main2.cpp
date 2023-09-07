#include <cstdio>
#include <cstdlib>

//分离PCM16LE双声道音频采样数据的左声道和右声道
// PCM 双声道存储方式 : (l0,r0) (l1,r1) .......
// 16  表示每个采样点占用16位
// LE Little Endian  小端序存储
int simplest_pcm16le_split(const char *url){
    FILE *fp=fopen(url,"rb+");
    FILE *fp1=fopen("output_l.pcm","wb+");
    FILE *fp2=fopen("output_r.pcm","wb+");

    unsigned char *sample=(unsigned char *)malloc(4);

    while(!feof(fp)){
        fread(sample,1,4,fp);
        //L
        fwrite(sample,1,2,fp1);
        //R
        fwrite(sample+2,1,2,fp2);
    }

    free(sample);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    return 0;
}

int main(int argc, char *argv[]) {
    simplest_pcm16le_split("NocturneNo2inEflat_44.1k_s16le.pcm");
    return 0;
}