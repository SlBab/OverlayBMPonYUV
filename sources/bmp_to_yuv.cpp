#include "bmp_to_yuv.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>  // Для измерения времени
#include <emmintrin.h> // SSE2

bool loadBMP(const std::string& filename, 
            BitmapFileHeader& fileHeader,
            BitmapInfoHeader& infoHeader, 
            std::vector<unsigned char>& rgbData,
            int& width, 
            int& height) {

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file.\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(BitmapFileHeader));
    if (fileHeader.bfType != 0x4D42) { // 'BM'
        std::cerr << "Not a BMP file.\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(BitmapInfoHeader));

    if (infoHeader.biBitCount != 24 || infoHeader.biCompression != 0) {
        std::cerr << "Unsupported BMP format.\n";
        return false;
    }

    width = infoHeader.biWidth;
    height = infoHeader.biHeight;
    int rowSize = (width * 3 + 3) & ~3; // Выравнивание на 4 байта
    int dataSize = rowSize * height;

    rgbData.resize(dataSize);
    file.seekg(fileHeader.bfOffBits, std::ios::beg);
    file.read(reinterpret_cast<char*>(rgbData.data()), dataSize);

    return true;
}

// Функция для вычисления Y компоненты
void processY_SSE2(const std::vector<unsigned char>& rgbData,
                       std::vector<unsigned char>& Y,
                       int width,
                       int height,
                       int numThreads) {
    
    const int totalPixels = width * height;
    auto start_single = std::chrono::high_resolution_clock::now();                    
    std::vector<std::thread> threads;

    // Константы для преобразования в Y
    const __m128i zero = _mm_setzero_si128();
    const __m128 r_coeff = _mm_set1_ps(0.299f);
    const __m128 g_coeff = _mm_set1_ps(0.587f);
    const __m128 b_coeff = _mm_set1_ps(0.114f);
    const __m128 max_val = _mm_set1_ps(255.0f);
    const __m128 min_val = _mm_set1_ps(0.0f);

    auto processY = [&](int startRow, int endRow) {
        for (int row = startRow; row < endRow; ++row) {
            const int srcRow = height - 1 - row;
            const unsigned char* srcPtr = rgbData.data() + srcRow * width * 3;
            unsigned char* dstPtr = Y.data() + row * width;
            
            int col = 0;
            // Обрабатываем по 4 пикселя за раз
            for (; col + 4 <= width; col += 4) {
                // Загружаем 4 пикселя (12 байт)
                __m128i rgb = _mm_loadu_si128((const __m128i*)(srcPtr + col * 3));
                
                // Разделяем на каналы (R, G, B)
                __m128i r = _mm_and_si128(_mm_srli_epi32(rgb, 16), _mm_set1_epi32(0xFF));
                __m128i g = _mm_and_si128(_mm_srli_epi32(rgb, 8), _mm_set1_epi32(0xFF));
                __m128i b = _mm_and_si128(rgb, _mm_set1_epi32(0xFF));
                
                // Конвертируем в float
                __m128 r_f = _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(r, zero), zero));
                __m128 g_f = _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(g, zero), zero));
                __m128 b_f = _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(b, zero), zero));
                
                // Вычисляем Y = 0.299*R + 0.587*G + 0.114*B
                __m128 y_val = _mm_mul_ps(r_f, r_coeff);
                y_val = _mm_add_ps(y_val, _mm_mul_ps(g_f, g_coeff));
                y_val = _mm_add_ps(y_val, _mm_mul_ps(b_f, b_coeff));
                
                // Ограничиваем значения [0, 255]
                y_val = _mm_min_ps(_mm_max_ps(y_val, min_val), max_val);
                
                // Конвертируем обратно в uint8_t
                __m128i y_int = _mm_cvtps_epi32(y_val);
                y_int = _mm_packs_epi32(y_int, y_int);
                y_int = _mm_packus_epi16(y_int, y_int);
                
                // Сохраняем результат
                *(int*)(dstPtr + col) = _mm_cvtsi128_si32(y_int);
            }
            
            // Обрабатываем оставшиеся пиксели
            for (int col = 0; col < width; ++col) {
                const int pixelIdx = col * 3;
                const uint8_t B = srcPtr[pixelIdx];
                const uint8_t G = srcPtr[pixelIdx + 1];
                const uint8_t R = srcPtr[pixelIdx + 2];

                float Y_val = 0.299f * R + 0.587f * G + 0.114f * B;
                Y_val = std::min(std::max(Y_val, 0.0f), 255.0f);
                dstPtr[col] = static_cast<uint8_t>(Y_val);
            }
        }
    };

    const int yRowsPerThread = height / numThreads;
    for (int t = 0; t < numThreads; ++t) {
        const int startRow = t * yRowsPerThread;
        const int endRow = (t == numThreads - 1) ? height : startRow + yRowsPerThread;
        threads.emplace_back(processY, startRow, endRow);
    }

    for (auto& th : threads) th.join();

    auto end_single = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_single = end_single - start_single;
    std::cout << "Y processing time (SSE2): " << duration_single.count() << " ms\n";
}

void processUV_MulThr(const std::vector<unsigned char>& rgbData, 
                    std::vector<unsigned char>& U, 
                    std::vector<unsigned char>& V, 
                    int width, 
                    int height, 
                    int numThreads) {
    
    const int totalBlocks = (width / 2) * (height / 2);
    auto start_single = std::chrono::high_resolution_clock::now();                    
    std::vector<std::thread> threads;

    auto processUV = [&](int startBlock, int endBlock) {
        // Блоки 2х2
        const int blockWidth = width / 2;
        const int blockHeight = height / 2;

        for (int block = startBlock; block < endBlock; ++block) {
            const int blockCol = block % blockWidth;    // Колонка блока
            const int blockRow = block / blockWidth;    // Строка блока
            const int xBase = blockCol * 2;
            const int yBase = blockRow * 2;

            double sumR = 0, sumG = 0, sumB = 0;
            int count = 0;  // Счетчик пикселей в блоке

            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    const int x = xBase + dx;
                    const int y = yBase + dy;
                    const int srcRow = height - 1 - y;  // Переворот У
                    const int pixelIdx = srcRow * width * 3 + x * 3;

                    sumR += rgbData[pixelIdx + 2];
                    sumG += rgbData[pixelIdx + 1];
                    sumB += rgbData[pixelIdx];
                    count++;
                }
            }

            // Обработка оставшихся пикселей
            if (count > 0) {
                const double avgR = sumR / count;
                const double avgG = sumG / count;
                const double avgB = sumB / count;

                double U_val = -0.147 * avgR - 0.289 * avgG + 0.436 * avgB + 128;
                double V_val = 0.615 * avgR - 0.515 * avgG - 0.100 * avgB + 128;

                U_val = std::min(std::max(U_val, 0.0), 255.0);
                V_val = std::min(std::max(V_val, 0.0), 255.0);

                U[block] = static_cast<uint8_t>(U_val);
                V[block] = static_cast<uint8_t>(V_val);
            }
        }
    };

    const int uvBlocksPerThread = totalBlocks / numThreads;
    for (int t = 0; t < numThreads; ++t) {
        const int startBlock = t * uvBlocksPerThread;
        const int endBlock = (t == numThreads - 1) ? totalBlocks : startBlock + uvBlocksPerThread;
        threads.emplace_back(processUV, startBlock, endBlock);
    }

    for (auto& th : threads) th.join();

    auto end_single = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_single = end_single - start_single;
    std::cout << "UV processing time (threads): " << duration_single.count() << " ms\n";
}

void convertRGBtoYUV420(const std::vector<unsigned char>& rgbData,
                        std::vector<unsigned char>& Y,
                        std::vector<unsigned char>& U,
                        std::vector<unsigned char>& V,
                        int width,
                        int height){
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;                        
    auto start = std::chrono::high_resolution_clock::now();

    processY_SSE2(rgbData, Y, width, height, numThreads);
    processUV_MulThr(rgbData, U, V, width, height, numThreads);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Time: " << duration.count() << " ms\n";
}
