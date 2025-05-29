#ifndef OVERLAY_H
#define OVERLAY_H

#include "read_yuv.h"

// Наложение Y-плоскости
void overlayYPlane(YUVImage& frame, const YUVImage& overlay, int startX, int startY, int startRow, int endRow);

// Наложение U/V-плоскостей
void overlayUVPlanes(YUVImage& frame, const YUVImage& overlay, int startX, int startY, int startBlock, int endBlock);

// Основная функция наложения с многопоточностью
void overlayImage(YUVImage& frame, const YUVImage& overlay, int startX, int startY);

#endif // OVERLAY_H