#ifndef READ_YUV_H
#define READ_YUV_H

#include <vector>
#include <string>
#include <cstdint>

// Структура для хранения YUV420 изображения
struct YUVImage {
    std::vector<uint8_t> Y;
    std::vector<uint8_t> U;
    std::vector<uint8_t> V;
    int width;
    int height;
};

// Загрузка YUV420 изображения из файла
bool loadYUVImage(const std::string& filename, YUVImage& img, int width, int height);

// Чтение одного кадра из YUV файла
bool readFrame(std::ifstream& file, YUVImage& frame);

// Запись одного кадра в YUV файл
void writeFrame(std::ofstream& file, const YUVImage& frame);

#endif // READ_YUV_H