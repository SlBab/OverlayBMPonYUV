#include "read_yuv.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>

bool loadYUVImage(const std::string& filename, YUVImage& img, int width, int height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open overlay file: " << filename << "\n";
        return false;
    }

    size_t y_size = width * height;
    size_t uv_size = (width / 2) * (height / 2);

    img.Y.resize(y_size);
    img.U.resize(uv_size);
    img.V.resize(uv_size);
    img.width = width;
    img.height = height;

    file.read(reinterpret_cast<char*>(img.Y.data()), y_size);
    if (!file) return false;

    file.read(reinterpret_cast<char*>(img.U.data()), uv_size);
    if (!file) return false;

    file.read(reinterpret_cast<char*>(img.V.data()), uv_size);
    if (!file) return false;

    return true;
}

bool readFrame(std::ifstream& file, YUVImage& frame) {
    size_t y_size = frame.width * frame.height;
    size_t uv_size = (frame.width / 2) * (frame.height / 2);

    if (!file.read(reinterpret_cast<char*>(frame.Y.data()), y_size)) return false;
    if (!file.read(reinterpret_cast<char*>(frame.U.data()), uv_size)) return false;
    if (!file.read(reinterpret_cast<char*>(frame.V.data()), uv_size)) return false;

    return true;
}

void writeFrame(std::ofstream& file, const YUVImage& frame) {
    size_t y_size = frame.width * frame.height;
    size_t uv_size = (frame.width / 2) * (frame.height / 2);

    file.write(reinterpret_cast<const char*>(frame.Y.data()), y_size);
    file.write(reinterpret_cast<const char*>(frame.U.data()), uv_size);
    file.write(reinterpret_cast<const char*>(frame.V.data()), uv_size);
}