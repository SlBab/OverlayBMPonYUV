#include "overlay.h"
#include <thread>
#include <algorithm>
#include <vector>
#include <cstdint>

void overlayYPlane(YUVImage& frame, 
                    const YUVImage& overlay, 
                    int startX, int startY, 
                    int startRow, 
                    int endRow) {
    for (int y = startRow; y < endRow && (startY + y) < frame.height; ++y) {
        for (int x = 0; x < overlay.width && (startX + x) < frame.width; ++x) {
            int frame_idx = (startY + y) * frame.width + (startX + x);
            int overlay_idx = y * overlay.width + x;
            frame.Y[frame_idx] = overlay.Y[overlay_idx];
        }
    }
}

void overlayUVPlanes(YUVImage& frame, 
                    const YUVImage& overlay, 
                    int startX, int startY, 
                    int startBlock, 
                    int endBlock) {
    // Рассчитываем ширину UV-компонент (в 2 раза меньше яркостной компоненты)
    int frame_uw = frame.width / 2;
    int overlay_uw = overlay.width / 2;
    int overlay_uh = overlay.height / 2;

    for (int block = startBlock; block < endBlock && block < overlay_uw * overlay_uh; ++block) {
        // Преобразуем линейный индекс блока в координаты (колонка, строка)
        int block_col = block % overlay_uw;
        int block_row = block / overlay_uw;
        // Вычисляем координаты в яркостной компоненте (умножаем на 2)
        int xBase = block_col * 2;
        int yBase = block_row * 2;

        if (startX + xBase >= frame.width || startY + yBase >= frame.height)
            continue;
        // Вычисляем индекс в UV-компонентах целевого изображения
        int frame_u_idx = (startY / 2 + block_row) * frame_uw + (startX / 2 + block_col);
        frame.U[frame_u_idx] = overlay.U[block];
        frame.V[frame_u_idx] = overlay.V[block];
    }
}

void overlayImage(YUVImage& frame, const YUVImage& overlay, int startX, int startY) {
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 8;
    std::vector<std::thread> threads;

    // Наложение Y-канала
    int y_rows = overlay.height;
    int y_rows_per_thread = y_rows / numThreads;
    
    for (int t = 0; t < numThreads; ++t) {
        int startRow = t * y_rows_per_thread;
        int endRow = (t == numThreads - 1) ? y_rows : startRow + y_rows_per_thread;
        threads.emplace_back(overlayYPlane, std::ref(frame), std::ref(overlay), startX, startY, startRow, endRow);
    }
    
    for (auto& th : threads) th.join();
    threads.clear();

    // Наложение U/V каналов
    int total_blocks = (overlay.width / 2) * (overlay.height / 2);
    int blocks_per_thread = total_blocks / numThreads;
    
    for (int t = 0; t < numThreads; ++t) {
        int startBlock = t * blocks_per_thread;
        int endBlock = (t == numThreads - 1) ? total_blocks : startBlock + blocks_per_thread;
        threads.emplace_back(overlayUVPlanes, std::ref(frame), std::ref(overlay), startX, startY, startBlock, endBlock);
    }
    
    for (auto& th : threads) th.join();
}