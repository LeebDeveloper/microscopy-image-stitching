#pragma once
#include "Definitions.h"

BYTE* LoadBMP(int& width, int& height, long& size, const char* bmpfile);
BYTE* ConvertBMPToIntensity(BYTE* Buffer, int width, int height);
BYTE* ConvertIntensityToBMP(BYTE* Buffer, int width, int height, long& newsize);
bool SaveBMP(BYTE* buffer, int width, int height, const char* bmpfile);