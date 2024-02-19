#include <cstring>
#include <fstream>
#include <cmath>
#include "Image.h"
#include "Definitions.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

BYTE* LoadImage(int& width, int& height, long& size, const char* bmpfile)
{
    int w, h, ch;
    auto image_data = stbi_load(bmpfile, &w, &h, &ch, 3);

    size = w * h * 3;
    width = w;
    height = h;

    auto result = new BYTE[size];
    memcpy(result, image_data, size);

    stbi_image_free(image_data);

    return result;
}//LoadImage

BYTE* ConvertRGBToIntensity(BYTE* Buffer, int width, int height)
{
	// first make sure the parameters are valid
	if ((Buffer == nullptr) || (width == 0) || (height == 0))
		return nullptr;

	// find the number of padding bytes

	int padding = 0;
	int scanlinebytes = width * 3;
	// get the padded scanline width
	int psw = scanlinebytes + padding;

	// create new buffer
	BYTE* newbuf = new BYTE[width * height];

	// now we loop trough all bytes of the original buffer, 
	// swap the R and B bytes and the scanlines
	long bufpos = 0;
	long newpos = 0;
	for (int row = 0; row < height; row++)
		for (int column = 0; column < width; column++) {
			newpos = row * width + column;
			bufpos = (height - row - 1) * psw + column * 3;
			newbuf[newpos] = (BYTE)(0.11 * Buffer[bufpos] + 0.59 * Buffer[bufpos + 1] + 0.3 * Buffer[bufpos + 2]);
		}

	return newbuf;
}//ConvertToBMTToIntensity(..)

BYTE* ConvertIntensityToRGB(BYTE* Buffer, int width, int height, long& newsize)
{
	// first make sure the parameters are valid
	if ((Buffer == nullptr) || (width == 0) || (height == 0))
		return nullptr;

	// now we have to find with how many bytes
	// we have to pad for the next DWORD boundary	

	int padding = 0;
	int scanlinebytes = width * 3;
	// get the padded scanline width
	int psw = scanlinebytes + padding;
	// we can already store the size of the new padded buffer
	newsize = height * psw;

	// and create new buffer
	BYTE* newbuf = new BYTE[newsize];

	// fill the buffer with zero bytes then we dont have to add
	// extra padding zero bytes later on
	memset(newbuf, 0, newsize);

	// now we loop trough all bytes of the original buffer, 
	// swap the R and B bytes and the scanlines
	long bufpos = 0;
	long newpos = 0;
	for (int row = 0; row < height; row++)
		for (int column = 0; column < width; column++) {
			bufpos = row * width + column;     // position in original buffer
			newpos = (height - row - 1) * psw + column * 3;           // position in padded buffer

			newbuf[newpos] = Buffer[bufpos];       //  blue
			newbuf[newpos + 1] = Buffer[bufpos];   //  green
			newbuf[newpos + 2] = Buffer[bufpos];   //  red
		}

	return newbuf;
}//ConvertIntensityToRGB

bool SavePNG(BYTE* buffer, int width, int height, const char* bmpfile)
{
    return stbi_write_png(bmpfile, width, height, 3, buffer, width * 3) != 0;
} //saveBMP