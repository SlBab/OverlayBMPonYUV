#ifndef BMP_TO_YUV_H
#define BMP_TO_YUV_H

#include <vector>
#include <string>
#include <thread>
#include <algorithm>
#include <cmath>

#pragma pack(push, 1)   // Выравнивание байтов
// Структуры заголовков BMP
struct BitmapFileHeader {   
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
};

struct BitmapInfoHeader {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
};
#pragma pack(pop)

// Загрузка BMP-файла
bool loadBMP(const std::string& filename,
             BitmapFileHeader& fileHeader,
             BitmapInfoHeader& infoHeader,
             std::vector<unsigned char>& rgbData,
             int& width,
             int& height);

void processY_SSE2(const std::vector<unsigned char>& rgbData,
                   std::vector<unsigned char>& Y,
                   int width, int height, int numThreads);

void processUV_MulThr(const std::vector<unsigned char>& rgbData,
                    std::vector<unsigned char>& U,
                    std::vector<unsigned char>& V,
                    int width, int height, int numThreads);

void convertRGBtoYUV420(const std::vector<unsigned char>& rgbData, 
                        std::vector<unsigned char>& Y,
                        std::vector<unsigned char>& U, 
                        std::vector<unsigned char>& V, 
                        int width, 
                        int height);              
#endif // BMP_TO_YUV_H