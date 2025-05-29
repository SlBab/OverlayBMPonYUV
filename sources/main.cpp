#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <algorithm>
#include <cstdint>

#include "bmp_to_yuv.h"
#include "read_yuv.h"
#include "overlay.h"

int main(int argc, char* argv[]) {
    // Проверка аргументов
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <input_bmp> <input_yuv> <output_yuv> <width> <height>\n";
        return 1;
    }

    std::string input_bmp = argv[1];
    std::string input_yuv = argv[2];
    std::string output_yuv = argv[3];
    int video_width = std::stoi(argv[4]);
    int video_height = std::stoi(argv[5]);

    // Загрузка и конвертация BMP в YUV420

    BitmapFileHeader fileHeader;
    BitmapInfoHeader infoHeader;
    std::vector<uint8_t> rgbData;
    int bmpWidth = 0, bmpHeight = 0;
    
    if (!loadBMP(input_bmp, fileHeader, infoHeader, rgbData, bmpWidth, bmpHeight)) {
        std::cerr << "Failed to load BMP file.\n";
        return 1;
    }

    if (bmpWidth > video_width || bmpHeight > video_height) {
        std::cerr << "Overlay size exceeds frame size.\n";
        return 1;
    }

    // Преобразование RGB -> YUV420
    YUVImage overlay;
    overlay.width = bmpWidth;
    overlay.height = bmpHeight;

    overlay.Y.resize(bmpWidth * bmpHeight);
    overlay.U.resize((bmpWidth / 2) * (bmpHeight / 2));
    overlay.V.resize((bmpWidth / 2) * (bmpHeight / 2));

    convertRGBtoYUV420(rgbData, overlay.Y, overlay.U, overlay.V, bmpWidth, bmpHeight);

    // Открытие YUV видеоряда и наложение
    std::ifstream inputFile(input_yuv, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Failed to open input YUV file.\n";
        return 1;
    }

    YUVImage frame;
    frame.width = video_width;
    frame.height = video_height;
    frame.Y.resize(video_width * video_height);
    frame.U.resize((video_width / 2) * (video_height / 2));
    frame.V.resize((video_width / 2) * (video_height / 2));

    std::ofstream outputFile(output_yuv, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Failed to create output YUV file.\n";
        return 1;
    }

    // Сдвиг наложения изображения
    int startX = 0;
    int startY = 0;

    int frame_count = 0;
    while (readFrame(inputFile, frame)) {
        overlayImage(frame, overlay, startX, startY);
        writeFrame(outputFile, frame);
        frame_count++;
    }

    std::cout << "Successfully processed " << frame_count << " frames with overlay.\n";
    return 0;
}