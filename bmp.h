#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// for Linux platform, plz make sure the size of data type is correct for BMP spec.
// if you use this on Windows or other platforms, plz pay attention to this.
typedef int LONG;
typedef unsigned char BYTE;
typedef unsigned int DWORD;
typedef unsigned short WORD;

// __attribute__((packed)) on non-Intel arch may cause some unexpected error, plz be informed.

typedef struct tagBITMAPFILEHEADER
{
    WORD    bfType; // 2  /* Magic identifier */
    DWORD   bfSize; // 4  /* File size in bytes */
    WORD    bfReserved1; // 2
    WORD    bfReserved2; // 2
    DWORD   bfOffBits; // 4 /* Offset to image data, bytes */ 
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD    biSize; // 4 /* Header size in bytes */
    LONG     biWidth; // 4 /* Width of image */
    LONG     biHeight; // 4 /* Height of image */
    WORD     biPlanes; // 2 /* Number of colour planes */
    WORD     biBitCount; // 2 /* Bits per pixel */
    DWORD    biCompress; // 4 /* Compression type */
    DWORD    biSizeImage; // 4 /* Image size in bytes */
    LONG     biXPelsPerMeter; // 4
    LONG     biYPelsPerMeter; // 4 /* Pixels per meter */
    DWORD    biClrUsed; // 4 /* Number of colours */ 
    DWORD    biClrImportant; // 4 /* Important colours */ 
} __attribute__((packed)) BITMAPINFOHEADER;

void SaveBitmap(char *filename, uint8_t *data, int width, int height) 
{
    int size = width * height * 3;
    BITMAPFILEHEADER bmpHeader = { 0 };
    bmpHeader.bfType = 0x4D42; // 'BM' //('M' << 8) | 'B'
    bmpHeader.bfReserved1 = 0;
    bmpHeader.bfReserved2 = 0;
    bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpHeader.bfSize = bmpHeader.bfOffBits + size;


    // printf("sizeof(BITMAPINFOHEADER) = %d\n", sizeof(BITMAPINFOHEADER));
    BITMAPINFOHEADER bmpInfo = { 0 };
    bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.biWidth = width;
    bmpInfo.biHeight = -height;  // 反转图片
    bmpInfo.biPlanes = 1;
    bmpInfo.biBitCount = 24;  // bit(s) per pixel, 24 is true color
    bmpInfo.biCompress = 0;
    bmpInfo.biSizeImage = 0;
    bmpInfo.biXPelsPerMeter = 100;
    bmpInfo.biYPelsPerMeter = 100;
    bmpInfo.biClrUsed = 0;
    bmpInfo.biClrImportant = 0;

    FILE *fp;
    if (!(fp = fopen(filename,"wb"))) return 0;

    fwrite(&bmpHeader, 1, sizeof(BITMAPFILEHEADER), fp);
    fwrite(&bmpInfo, 1, sizeof(BITMAPINFOHEADER), fp);
    fwrite(data, 1, size, fp);
    fclose(fp);
}


