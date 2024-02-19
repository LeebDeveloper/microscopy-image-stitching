#pragma once
#include "Definitions.h"

BYTE* LoadImage(int& width, int& height, long& size, const char* bmpfile);
BYTE* ConvertRGBToIntensity(BYTE* Buffer, int width, int height);
BYTE* ConvertIntensityToRGB(BYTE* Buffer, int width, int height, long& newsize);
bool SavePNG(BYTE* buffer, int width, int height, const char* bmpfile);