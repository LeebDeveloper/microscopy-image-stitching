#include <cstring>
#include <fstream>
#include "Image.h"
#include "Definitions.h"

/*
 * Constants for the biCompression field...
 */

#define BI_RGB       0             /* No compression - straight BGR data */
#define BI_RLE8      1             /* 8-bit run-length compression */
#define BI_RLE4      2             /* 4-bit run-length compression */
#define BI_BITFIELDS 3             /* RGB bitmap with RGB masks */

struct BITMAPFILEHEADER {
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
};

struct BITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};

BYTE* LoadBMP(int& width, int& height, long& size, const char* bmpfile)
{
	// declare bitmap structures
	BITMAPFILEHEADER bmpheader{};
	BITMAPINFOHEADER bmpinfo;
	// value to be used in ReadFile funcs
	DWORD bytesread;
	// open file to read from
    std::ifstream file(bmpfile, std::ios::binary);

    if (!file.is_open()) {
        return nullptr;
    } // coudn't open file

	// read file header
    file.read(reinterpret_cast<char *>(&bmpheader), sizeof(BITMAPFILEHEADER));
	if (file.fail()) {
		file.close();
		return nullptr;
	}

	//read bitmap info
    file.read(reinterpret_cast<char *>(&bmpinfo), sizeof(BITMAPINFOHEADER));
    if (file.fail()) {
        file.close();
        return nullptr;
    }

	// check if file is actually a bmp
	if (bmpheader.bfType != 0x4d42) { // the ASCII string "BM"
		file.close();
		return nullptr;
	}

	// get image measurements
	width = bmpinfo.biWidth;
	height = abs(bmpinfo.biHeight);

	// check if bmp is uncompressed
	if (bmpinfo.biCompression != BI_RGB) {
        file.close();
        return nullptr;
	}

	// check if we have 24 bit bmp
	if (bmpinfo.biBitCount != 24) {
        file.close();
        return nullptr;
	}

	// create buffer to hold the data
	size = bmpheader.bfSize - bmpheader.bfOffBits;
	BYTE* Buffer = new BYTE[size];
	// move file pointer to start of bitmap data
    file.seekg(bmpheader.bfOffBits, std::ios::beg);
	// read bmp data
    file.read(reinterpret_cast<char*>(Buffer), size);
	if (file.fail()) {
		delete[] Buffer;
        file.close();
        return nullptr;
	}

	// everything successful here: close file and return buffer

	file.close();

	return Buffer;
}//LoadBMP

BYTE* ConvertBMPToIntensity(BYTE* Buffer, int width, int height)
{
	// first make sure the parameters are valid
	if ((Buffer == nullptr) || (width == 0) || (height == 0))
		return nullptr;

	// find the number of padding bytes

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
		padding++;
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
			newbuf[newpos] = (BYTE)(0.11 * Buffer[bufpos + 2] + 0.59 * Buffer[bufpos + 1] + 0.3 * Buffer[bufpos]);
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
bool SaveBMP(BYTE* Buffer, int width, int height, long paddedsize, const char* bmpfile)
{
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
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paddedsize;
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
	// write image data
    file.write(reinterpret_cast<const char *>(Buffer), sizeof(paddedsize));
    if (file.fail())
    {
        file.close();
        return false;
    }

	// and close file
	file.close();

	return true;
} //saveBMP