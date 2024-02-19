#include <cstring>
#include <fstream>
#include <cmath>
#include "Image.h"
#include "Definitions.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*
 * Constants for the biCompression field...
 */

#define BI_RGB       0             /* No compression - straight BGR data */
#define BI_RLE8      1             /* 8-bit run-length compression */
#define BI_RLE4      2             /* 4-bit run-length compression */
#define BI_BITFIELDS 3             /* RGB bitmap with RGB masks */

#pragma pack(push, 1)
struct BITMAPFILEHEADER {
    uint16_t bfType;
    uint32_t  bfSize;
    uint16_t  bfReserved1;
    uint16_t  bfReserved2;
    uint32_t bfOffBits;
};

struct BITMAPINFOHEADER {
    uint32_t biSize;
    int32_t biWidth;
    int32_t  biHeight;
    uint16_t  biPlanes;
    uint16_t  biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

BYTE* LoadBMP(int& width, int& height, long& size, const char* bmpfile)
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
}//LoadBMP

BYTE* ConvertBMPToIntensity(BYTE* Buffer, int width, int height)
{
	// first make sure the parameters are valid
	if ((Buffer == nullptr) || (width == 0) || (height == 0))
		return nullptr;

	// find the number of padding bytes

	int padding = 0;
	int scanlinebytes = width * 3;
//	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
//		padding++;
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

BYTE* ConvertIntensityToBMP(BYTE* Buffer, int width, int height, long& newsize)
{
	// first make sure the parameters are valid
	if ((Buffer == nullptr) || (width == 0) || (height == 0))
		return nullptr;

	// now we have to find with how many bytes
	// we have to pad for the next DWORD boundary	

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
		padding++;
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
}//ConvertIntensityToBMP

bool SaveBMP(BYTE* buffer, int width, int height, const char* bmpfile)
{
    auto bytes_in_row = width * 3;
    auto padding = 0;
    while((bytes_in_row + padding) % 4 != 0) padding++;

    auto scanline_size = bytes_in_row + padding;

	// declare bmp structures 
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER info;

	// andinitialize them to zero
	memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));
	memset(&info, 0, sizeof(BITMAPINFOHEADER));

	// fill the fileheader with data
	bmfh.bfType = 0x4d42;       // 0x4d42 = 'BM'
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + scanline_size * height;
	bmfh.bfOffBits = 0x36;		// number of bytes to start of bitmap bits

	// fill the infoheader

	info.biSize = sizeof(BITMAPINFOHEADER);
	info.biWidth = width;
	info.biHeight = height;
	info.biPlanes = 1;			// we only have one bitplane
	info.biBitCount = 24;		// RGB mode is 24 bits
	info.biCompression = BI_RGB;
	info.biSizeImage = 0;		// can be 0 for 24 bit images
	info.biXPelsPerMeter = 0x0ec4;     // paint and PSP use this values
	info.biYPelsPerMeter = 0x0ec4;
	info.biClrUsed = 0;			// we are in RGB mode and have no palette
	info.biClrImportant = 0;    // all colors are important

	// now we open the file to write to
    std::ofstream file(bmpfile, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		return false;
	}

	// write file header
    file.write(reinterpret_cast<const char*>(&bmfh), sizeof(BITMAPFILEHEADER));
	if (file.fail())
	{
		file.close();
		return false;
	}
	// write infoheader
    file.write(reinterpret_cast<const char*>(&info), sizeof(BITMAPINFOHEADER));
    if (file.fail())
    {
        file.close();
        return false;
    }
    // prepare image data
    auto image_data = new BYTE[scanline_size * height];
    for(auto y = 0; y < height; y++)
    {
        for(auto x = 0; x < width * 3; x += 3)
        {
            auto offset = y * scanline_size;
            auto orig_offset = (height - y - 1) * width * 3;
            image_data[offset + x]     = buffer[orig_offset + x + 2];
            image_data[offset + x + 1] = buffer[orig_offset + x + 1];
            image_data[offset + x + 2] = buffer[orig_offset + x];
        }
    }
	// write image data
    file.write(reinterpret_cast<const char *>(image_data), scanline_size * height);
    if (file.fail())
    {
        file.close();
        return false;
    }

	// and close file
	file.close();

	return true;
} //saveBMP