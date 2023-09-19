/**
 * Split Y, U, V planes in YUV420P file.
 * @param url  Location of Input YUV file.
 * @param w    Width of Input YUV file.
 * @param h    Height of Input YUV file.
 * @param num  Number of frames to process.
 *
 */



#include <cstring>
#include <valarray>
#include "stdio.h"
#include "stdlib.h"


// YUV 4：2：0
// 拆分 YUV 分别存储
int simplest_yuv420_split(char *url, int w, int h, int num) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_420_y.y", "wb+");
    FILE *fp2 = fopen("output_420_u.y", "wb+");
    FILE *fp3 = fopen("output_420_v.y", "wb+");

    //  申请内存空间    w*h+w*h*0.25+w*h*0.25 = w*h*3/2   刚好一帧的所需的字节大小
    // malloc() 申请的内存空间单位为字节 ，刚好 YUV 三个分量分别各自占用一个字节
    unsigned char *pic = (unsigned char *) malloc(w * h * 3 / 2);

    for (int i = 0; i < num; i++) {

        //  在 fp 中读取一帧的数据存储到  pic 中，  因为 *pic 只够存储一帧数据
        // 每次for循环都会从 fp中读取下一帧数据 覆盖到 pic 中
        fread(pic, 1, w * h * 3 / 2, fp);


        //420P视屏 采用了 Planar 平面格式进行存储。 也就是 YUV 4:2:0
        // 这种存储方式是先连续存储所有像素点的 Y 分量，然后存储所有像素点的 U 分量，最后是所有像素点的 V 分量

        //Y
        // 先取出所有Y分量， 每一个像素中都存储一个Y分量
        fwrite(pic, 1, w * h, fp1);
        //U
        // 然后取出U分量， 每四个像素公用一个U 分量 ， V分量同理
        // 将  pic 指针指向U分量的起始位置
        fwrite(pic + w * h, 1, w * h / 4, fp2);
        //V
        // pic+w*h+w*h/4 = pic+w*h*5/4  即指向V分量起始位置
        fwrite(pic + w * h * 5 / 4, 1, w * h / 4, fp3);
    }

    free(pic);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);

    return 0;
}

// YUV 4：4：4
int simplest_yuv444_split(char *url, int w, int h, int num) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_444p_y.y", "wb+");
    FILE *fp2 = fopen("output_444p_u.y", "wb+");
    FILE *fp3 = fopen("output_444p_v.y", "wb+");
    unsigned char *pic = (unsigned char *) malloc(w * h * 3);
    for (int i = 0; i < num; ++i) {
        fread(pic, 1, w * h * 3, fp);
        fwrite(pic, 1, w * h, fp1);
        fwrite(pic + w * h, 1, w * h, fp2);
        fwrite(pic + w * h * 2, 1, w * h, fp3);
    }
    free(pic);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    return 0;
}

// YUV 4:2:2
//,..... 一样的套路


// 将 420 转换为灰度图
int simplest_yuv420_gray(char *url, int w, int h, int num) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_420_gray.yuv", "wb+");
    unsigned char *pic = (unsigned char *) malloc(w * h * 3 / 2);

    for (int i = 0; i < num; i++) {
        fread(pic, 1, w * h * 3 / 2, fp);
        //Gray
        memset(pic + w * h, 128, w * h / 2);
        fwrite(pic, 1, w * h * 3 / 2, fp1);
    }

    free(pic);
    fclose(fp);
    fclose(fp1);
    return 0;
}

//Y = 0.299R + 0.587G + 0.114B
//U = -0.168R - 0.330G + 0.498B + 128
//V = 0.449R - 0.435G - 0.083B + 128
// U = 171  v = 243
// 444 转换为灰度图
int simplest_yuv444_gray(char *url, int w, int h, int frame) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_444_gray.yuv", "wb+");
    unsigned char *pic = (unsigned char *) malloc(w * h * 3);
    for (int i = 0; i < frame; ++i) {
        fread(pic, 1, w * h * 3, fp);
        memset(pic + w * h, 171, w * h);
        memset(pic + w * h * 2, 243, w * h);
        fwrite(pic, 1, w * h * 3, fp1);
    }
    free(pic);
    fclose(fp);
    fclose(fp1);
    return 0;
}


// 420 亮度减半  Y分量减半
int simplest_yuv420_halfy(char *url, int w, int h, int num) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_420_halfy.yuv", "wb+");
    unsigned char *pic = (unsigned char *) malloc(w * h * 3 / 2);
    for (int i = 0; i < num; ++i) {
        fread(pic, 1, w * h * 3 / 2, fp);
        for (int j = 0; j < w * h; ++j) {
            memset(pic + j, pic[j] / 2, 1);
//            unsigned char tmp = pic[j] / 2;
//            pic[j] = tmp;
        }
        fwrite(pic, 1, w * h * 3 / 2, fp1);
    }
    free(pic);
    fclose(fp);
    fclose(fp1);
    return 0;
}

// 420 修改YUV数据中特定位置的亮度分量Y的数值，给图像添加一个“边框”的效果
int simplest_yuv420_border(char *url, int w, int h, int border, int num) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_420_border.yuv", "wb+");
    unsigned char *pic = (unsigned char *) malloc(w * h * 3 / 2);
    for (int i = 0; i < num; i++) {
        fread(pic, 1, w * h * 3 / 2, fp);
        for (int j = 0; j < h; j++) {
            for (int k = 0; k < w; k++) {
                if (k < border || k > (w - border) || j < border || j > (h - border)) {
                    pic[j * w + k] = 255;
                }
            }
        }
        fwrite(pic, 1, w * h * 3 / 2, fp1);
    }
    free(pic);
    fclose(fp);
    fclose(fp1);
    return 0;
}


// 420 生成一张YUV420P格式的灰阶测试图
int simplest_yuv420_graybar(int width, int height, int ymin, int ymax, int barnum, char *url_out) {
    int barwidth;
    float lum_inc;
    unsigned char lum_temp;
    int uv_width, uv_height;
    FILE *fp = NULL;
    unsigned char *data_y = NULL;
    unsigned char *data_u = NULL;
    unsigned char *data_v = NULL;
    int t = 0, i = 0, j = 0;

    barwidth = width / barnum;
    lum_inc = ((float) (ymax - ymin)) / ((float) (barnum - 1));
    uv_width = width / 2;
    uv_height = height / 2;

    data_y = (unsigned char *) malloc(width * height);
    data_u = (unsigned char *) malloc(uv_width * uv_height);
    data_v = (unsigned char *) malloc(uv_width * uv_height);

    if ((fp = fopen(url_out, "wb+")) == NULL) {
        printf("Error: Cannot create file!");
        return -1;
    }

    //Output Info
    printf("Y, U, V value from picture's left to right:\n");
    for (t = 0; t < (width / barwidth); t++) {
        lum_temp = ymin + (char) (t * lum_inc);
        printf("%3d, 128, 128\n", lum_temp);
    }
    //Gen Data
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            t = i / barwidth;
            lum_temp = ymin + (char) (t * lum_inc);
            data_y[j * width + i] = lum_temp;
        }
    }
    for (j = 0; j < uv_height; j++) {
        for (i = 0; i < uv_width; i++) {
            data_u[j * uv_width + i] = 128;
        }
    }
    for (j = 0; j < uv_height; j++) {
        for (i = 0; i < uv_width; i++) {
            data_v[j * uv_width + i] = 128;
        }
    }
    fwrite(data_y, width * height, 1, fp);
    fwrite(data_u, uv_width * uv_height, 1, fp);
    fwrite(data_v, uv_width * uv_height, 1, fp);
    fclose(fp);
    free(data_y);
    free(data_u);
    free(data_v);
    return 0;
}


// 420  计算两个YUV420P像素数据的PSNR
// PSNR是最基本的视频质量评价方法。本程序中的函数可以对比两张YUV图片中亮度分量Y的PSNR
// PSNR（Peak Signal to Noise Ratio，峰值信噪比）是最基础的视频质量评价方法。
// 它的取值一般在20-50之间，值越大代表受损图片越接近原图片。
// 它通过对原始图像和失真图像进行像素的逐点对比，计算两幅图像像素点之间的误差，并由这些误差最终确定失真图像的质量评分。
// 该方法由于计算简便、数学意义明确，在图像处理领域中应用最为广泛。
int simplest_yuv420_psnr(char *url1, char *url2, int w, int h, int num) {
    FILE *fp1 = fopen(url1, "rb+");
    FILE *fp2 = fopen(url2, "rb+");
    unsigned char *pic1 = (unsigned char *) malloc(w * h);
    unsigned char *pic2 = (unsigned char *) malloc(w * h);

    for (int i = 0; i < num; i++) {
        fread(pic1, 1, w * h, fp1);
        fread(pic2, 1, w * h, fp2);

        double mse_sum = 0, mse = 0, psnr = 0;
        for (int j = 0; j < w * h; j++) {
            mse_sum += pow((double) (pic1[j] - pic2[j]), 2);
        }
        mse = mse_sum / (w * h);
        psnr = 10 * log10(255.0 * 255.0 / mse);
        printf("%5.3f\n", psnr);

        fseek(fp1, w * h / 2, SEEK_CUR);
        fseek(fp2, w * h / 2, SEEK_CUR);

    }

    free(pic1);
    free(pic2);
    fclose(fp1);
    fclose(fp2);
    return 0;
}


// 分离RGB24像素数据中的R、G、B分量 将RGB24数据中的R、G、B三个分量分离开来并保存成三个文件
// RGB24格式的每个像素的三个分量是连续存储的。
// 一帧宽高分别为w、h的RGB24图像一共占用w*h*3 Byte的存储空间。
// RGB24格式规定首先存储第一个像素的R、G、B，然后存储第二个像素的R、G、B…以此类推。
int simplest_rgb24_split(char *url, int w, int h, int num) {
    FILE *fp = fopen(url, "rb+");
    FILE *fp1 = fopen("output_r.y", "wb+");
    FILE *fp2 = fopen("output_g.y", "wb+");
    FILE *fp3 = fopen("output_b.y", "wb+");
    unsigned char *pic = (unsigned char *) malloc(w * h * 3);
    for (int i = 0; i < num; ++i) {
        fread(pic, 1, w * h * 3, fp);
        for (int j = 0; j < w * h * 3; j = j + 3) {
            //R
            fwrite(pic + j, 1, 1, fp1);
            // G
            fwrite(pic + j + 1, 1, 1, fp2);
            // B
            fwrite(pic + j + 2, 1, 1, fp3);
        }
    }
    free(pic);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    return 0;
}

//将RGB24格式像素数据封装为BMP图像
// BMP图像内部实际上存储的就是RGB数据。本程序实现了对RGB像素数据的封装处理。通过本程序中的函数，可以将RGB数据封装成为一张BMP图像。
/**
 * Convert RGB24 file to BMP file
 * @param rgb24path    Location of input RGB file.
 * @param width        Width of input RGB file.
 * @param height       Height of input RGB file.
 * @param url_out      Location of Output BMP file.
 */
/**
 * Convert RGB24 file to BMP file
 * @param rgb24path    Location of input RGB file.
 * @param width        Width of input RGB file.
 * @param height       Height of input RGB file.
 * @param url_out      Location of Output BMP file.
 */
int simplest_rgb24_to_bmp(const char *rgb24path, int width, int height, const char *bmppath) {
    typedef struct {
        u_int32_t imageSize;
        u_int32_t blank;
        u_int32_t startPosition;
    } BmpHead;

    typedef struct {
        u_int32_t Length;
        u_int32_t width;
        u_int32_t height;
        u_int16_t colorPlane;
        u_int16_t bitColor;
        u_int32_t zipFormat;
        u_int32_t realSize;
        u_int32_t xPels;
        u_int32_t yPels;
        u_int32_t colorUse;
        u_int32_t colorImportant;
    } InfoHead;

    int i = 0, j = 0;
    BmpHead m_BMPHeader = {0};
    InfoHead m_BMPInfoHeader = {0};
    char bfType[2] = {'B', 'M'};
    int header_size = sizeof(bfType) + sizeof(BmpHead) + sizeof(InfoHead);
    unsigned char *rgb24_buffer = NULL;
    FILE *fp_rgb24 = NULL, *fp_bmp = NULL;

    if ((fp_rgb24 = fopen(rgb24path, "rb")) == NULL) {
        printf("Error: Cannot open input RGB24 file.\n");
        return -1;
    }
    if ((fp_bmp = fopen(bmppath, "wb")) == NULL) {
        printf("Error: Cannot open output BMP file.\n");
        return -1;
    }

    rgb24_buffer = (unsigned char *) malloc(width * height * 3);
    fread(rgb24_buffer, 1, width * height * 3, fp_rgb24);

    m_BMPHeader.imageSize = 3 * width * height + header_size;
    m_BMPHeader.startPosition = header_size;

    m_BMPInfoHeader.Length = sizeof(InfoHead);
    m_BMPInfoHeader.width = width;
    //BMP storage pixel data in opposite direction of Y-axis (from bottom to top).
    m_BMPInfoHeader.height = -height;
    m_BMPInfoHeader.colorPlane = 1;
    m_BMPInfoHeader.bitColor = 24;
    m_BMPInfoHeader.realSize = 3 * width * height;

    fwrite(bfType, 1, sizeof(bfType), fp_bmp);
    fwrite(&m_BMPHeader, 1, sizeof(m_BMPHeader), fp_bmp);
    fwrite(&m_BMPInfoHeader, 1, sizeof(m_BMPInfoHeader), fp_bmp);

    //BMP save R1|G1|B1,R2|G2|B2 as B1|G1|R1,B2|G2|R2
    //It saves pixel data in Little Endian
    //So we change 'R' and 'B'
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            char temp = rgb24_buffer[(j * width + i) * 3 + 2];
            rgb24_buffer[(j * width + i) * 3 + 2] = rgb24_buffer[(j * width + i) * 3 + 0];
            rgb24_buffer[(j * width + i) * 3 + 0] = temp;
        }
    }
    fwrite(rgb24_buffer, 3 * width * height, 1, fp_bmp);
    fclose(fp_rgb24);
    fclose(fp_bmp);
    free(rgb24_buffer);
    printf("Finish generate %s!\n", bmppath);
    return 0;
}


int main(int argc, char *argv[]) {
//    simplest_yuv420_split("carphone_qcif_420p.yuv", 176, 144, 382);
//    simplest_yuv444_split("carphone_qcif_444p.yuv", 176, 144, 382);
//    simplest_yuv444_gray("carphone_qcif_444p.yuv", 176, 144, 382);
//    simplest_yuv420_halfy("carphone_qcif_420p.yuv", 176, 144, 382);
//    simplest_yuv420_border("carphone_qcif_420p.yuv", 176, 144, 10, 382);
//    simplest_yuv420_graybar(640, 360,0,255,10,"graybar_640x360.yuv");
//    simplest_rgb24_split("rgb24_cie1931.rgb", 500, 500, 1);
    simplest_rgb24_to_bmp("lena_256x256_rgb24.rgb",256,256,"output_lena.bmp");
}
