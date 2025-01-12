﻿#include "Panorama.h"
#include <cmath>
#include <cstring>
#include <arm_neon.h>
//#include <omp.h>
#define OFFSET 6

xy Transfer(double** H, int x, int y) {
	xy point;
	double xx, yy, zz;
	xx = round(H[0][0] * (double)x + H[0][1] * (double)y + H[0][2]);
	yy = round(H[1][0] * (double)x + H[1][1] * (double)y + H[1][2]);
	zz = round(H[2][0] * (double)x + H[2][1] * (double)y + H[2][2]);
	point.x = int(xx / zz);
	point.y = int(yy / zz);
	return point;
}

xy* ReSizePanorama(double** H, int panoWidth, int panoHeight, int Width, int Height, xy& position) {

	xy upLeft, upRight, downLeft, downRight, * Size = new xy;
	int tempWidth, tempHeight;

	tempWidth = panoWidth;
	tempHeight = panoHeight;
	upLeft = Transfer(H, 0, 0);
	upRight = Transfer(H, Width, 0);
	downLeft = Transfer(H, 0, Height);
	downRight = Transfer(H, Width, Height);
	position.x = position.y = 0;

	if ((upLeft.x < 0) || (downLeft.x < 0)) {
		if (upLeft.x < 0)
			if (downLeft.x < 0)
				if (abs(upLeft.x) >= abs(downLeft.x)) {
					tempWidth += abs(upLeft.x);
					position.x = abs(upLeft.x);
				}
				else {
					tempWidth += abs(downLeft.x);
					position.x = abs(downLeft.x);
				}
			else {
				tempWidth += abs(upLeft.x);
				position.x = abs(upLeft.x);
			}
		else {
			tempWidth += abs(downLeft.x);
			position.x = abs(downLeft.x);
		}
	}


	if ((upRight.x > panoWidth) || (downRight.x > panoWidth)) {
		if (upRight.x >= downRight.x)
			tempWidth += (upRight.x - panoWidth);
		else
			tempWidth += (downRight.x - panoWidth);
	}

	if ((upLeft.y < 0) || (upRight.y < 0)) {
		if (upLeft.y < 0)
			if (upRight.y < 0)
				if (abs(upLeft.y) >= abs(upRight.y)) {
					tempHeight += abs(upLeft.y);
					position.y = abs(upLeft.y);
				}
				else {
					tempHeight += abs(upRight.y);
					position.y = abs(upRight.y);
				}
			else {
				tempHeight += abs(upLeft.y);
				position.y = abs(upLeft.y);
			}
		else {
			tempHeight += abs(upRight.y);
			position.y = abs(upRight.y);
		}
	}

	if ((downLeft.y > panoHeight) || (downRight.y > panoHeight)) {
		if (downLeft.y >= downRight.y)
			tempHeight += ((downLeft.y - panoHeight));
		else
			tempHeight += ((downRight.y - panoHeight));
	}


	Size->x = tempWidth;
	Size->y = tempHeight;

	return Size;
}

BYTE* PanoToResized(BYTE* Raw, int Width, int Height, xy newPanoSize, xy position) {

	int newPanoSizeX = newPanoSize.x;
	int newPanoSizeY = newPanoSize.y;
	int positionX = position.x;
	int positionY = position.y;
	size_t tempR = 0, tempG = 0, tempB = 0, bufpos;
	BYTE* panoImg = new BYTE[newPanoSizeX * newPanoSizeY * 3];

//#pragma omp parallel num_threads(NUM_THREADS) shared(Height, Width, Raw) private(bufpos)
	{
//#pragma omp for schedule(dynamic) reduction(+:tempR, tempG, tempB) nowait
		for (int i = 0; i < Height; i++) {
			for (int j = 0; j < Width; j += 10) {
				bufpos = (Height - i - 1) * Width * 3 + j * 3;
				tempR += Raw[bufpos + 2];
				tempG += Raw[bufpos + 1];
				tempB += Raw[bufpos];
			}
		}
	}

	tempR /= (int(Width / 10) * Height);
	tempG /= (int(Width / 10) * Height);
	tempB /= (int(Width / 10) * Height);

//#pragma omp parallel num_threads(NUM_THREADS) shared(newPanoSizeY, newPanoSizeX, panoImg, tempR, tempG, tempB) private(bufpos)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < newPanoSizeY; i++) {
			for (int j = 0; j < newPanoSizeX; j++) {
				bufpos = (newPanoSizeY - i - 1) * newPanoSizeX * 3 + j * 3;

				panoImg[bufpos + 2] = (BYTE)tempR;
				panoImg[bufpos + 1] = (BYTE)tempG;
				panoImg[bufpos] = (BYTE)tempB;
			}
		}
	}

	size_t buf1, buf2;
//#pragma omp parallel num_threads(NUM_THREADS) shared(Height, Width, newPanoSizeY, positionY, newPanoSizeX, positionX, Raw, panoImg) private(buf1, buf2)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < Height; i++) {
			for (int j = 0; j < Width; j++) {
				buf1 = (newPanoSizeY - (positionY + i) - 1) * newPanoSizeX * 3 + (j + positionX) * 3;
				buf2 = (Height - i - 1) * Width * 3 + j * 3;

				panoImg[buf1 + 2] = Raw[buf2 + 2];
				panoImg[buf1 + 1] = Raw[buf2 + 1];
				panoImg[buf1] = Raw[buf2];
			}
		}
	}

	return panoImg;
}

BYTE* SizeExtend(BYTE* Raw, int& width, int& height, int Width, int Height) {

	bool temp = true;
	if (width >= height) {
		int n = 2;
		while (temp)
		{
			if (width == n) {
				width = n + 1;
				height = n + 1;
				temp = false;
			}
			else if (width > (n + 1) && width < (n * 2)) {
				width = n * 2 + 1;
				height = n * 2 + 1;
				temp = false;
			}
			n = n * 2;
		}
	}
	else {
		int n = 2;
		while (temp) {
			if (height == n) {
				width = n + 1;
				height = n + 1;
				temp = false;
			}
			else if (height > (n + 1) && height < (n * 2)) {
				width = n * 2 + 1;
				height = n * 2 + 1;
				temp = false;
			}
			n = n * 2;
		}
	}


	BYTE* extendedImg = new BYTE[width * height * 3]();

	int bufPos1, bufPos2;
//#pragma omp parallel num_threads(NUM_THREADS) shared(Height, Width, height, width, Raw, extendedImg) private(bufPos1, bufPos2)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < Height; i++)
			for (int j = 0; j < Width; j++) {
				bufPos1 = (height - i - 1) * width * 3 + j * 3;
				bufPos2 = (Height - i - 1) * Width * 3 + j * 3;

				extendedImg[bufPos1] = Raw[bufPos2];
				extendedImg[bufPos1 + 1] = Raw[bufPos2 + 1];
				extendedImg[bufPos1 + 2] = Raw[bufPos2 + 2];
			}

	}

	return extendedImg;
}

static BYTE coeff[5][5] = { {1,4,7,4,1},
                             {4,16,26,16,4},
                             {7,26,41,26,7},
                             {4,16,26,16,4},
                             {1,4,7,4,1} }; // 273

void fill_frame(BYTE* buffer, int width, int height)
{
    auto rowbytes = width * 3;

    memset(buffer, 255, 2 * rowbytes);
    memset(buffer + (height - 2) * rowbytes, 255, 2 * rowbytes);

    for(int y = 3; y < height - 2; y++)
    {
        memset(buffer, 255, 6);
        memset(buffer + rowbytes - 6, 255, 6);
    }
}

template<typename T>
T* GaussPyramid(const BYTE* raw, int& width, int& height, T* result)
{
    BYTE* smooth = new BYTE[width * height * 3];

    fill_frame(smooth, width, height);

    unsigned int row[width * 3];

    for (auto y = 2; y < height - 2; y++)
    {
        for(auto ky = -2; ky <= 2; ky++)
        {
            for(auto x = 2; x < width - 2; x++)
            {
                auto roff = (height - (y + ky) - 1) * width * 3;
                row[x * 3] += coeff[ky + 2][0] * (raw[roff + (x - 2) * 3] + raw[roff + (x + 2) * 3])
                            + coeff[ky + 2][1] * (raw[roff + (x - 1) * 3] + raw[roff + (x + 1) * 3])
                            + coeff[ky + 2][2] * raw[roff + x * 3];

                row[x * 3 + 1] += coeff[ky + 2][0] * (raw[roff + (x - 2) * 3 + 1] + raw[roff + (x + 2) * 3 + 1])
                                + coeff[ky + 2][1] * (raw[roff + (x - 1) * 3 + 1] + raw[roff + (x + 1) * 3 + 1])
                                + coeff[ky + 2][2] * raw[roff + x * 3 + 1];

                row[x * 3 + 2] += coeff[ky + 2][0] * (raw[roff + (x - 2) * 3 + 2] + raw[roff + (x + 2) * 3 + 2])
                                + coeff[ky + 2][1] * (raw[roff + (x - 1) * 3 + 2] + raw[roff + (x + 1) * 3 + 2])
                                + coeff[ky + 2][2] * raw[roff + x * 3 + 2];
            }
        }

        auto roff = (height - y - 1) * width * 3;
        for(auto x = 2; x < width - 2; x++)
        {
            auto i = roff + x * 3;
            auto j = x * 3;
            smooth[i]     = static_cast<BYTE>(row[j] / 273);
            smooth[i + 1] = static_cast<BYTE>(row[j + 1] / 273);
            smooth[i + 2] = static_cast<BYTE>(row[j + 2] / 273);
        }
        memset(row, 0, sizeof(row[0]) * width * 3);
    }

    int newHeight = ((height - 1) / 2) + 1;
    int newWidth = ((width - 1) / 2) + 1;
    result = new T[newWidth * newHeight * 3];
    size_t gaussPos, smoothPos;

    for (int i = 0, x = 0; i < height; i += 2, x++) {
        for (int j = 0, y = 0; j < width; j += 2, y++) {
            gaussPos = (newHeight - x - 1) * newWidth * 3 + y * 3;
            smoothPos = (height - i - 1) * width * 3 + j * 3;

            result[gaussPos + 2] = (T)smooth[smoothPos + 2];
            result[gaussPos + 1] = (T)smooth[smoothPos + 1];
            result[gaussPos] = (T)smooth[smoothPos];
        }
    }

    delete[] smooth;
    height = ((height - 1) / 2) + 1;
    width = ((width - 1) / 2) + 1;

    return result;
}

template<typename T>
void copy_pixel(const T *src, int src_width, int src_height, BYTE2 *dst, int dst_width, int dst_height, int i, int j) {
    auto indexi = i / 2;
    auto indexj = j / 2;
    auto dstpos = (dst_height - i - 1) * dst_width * 3 + j * 3;
    auto srcpos = (src_height - indexi - 1) * src_width * 3 + indexj * 3;
    dst[dstpos] = (BYTE2) src[srcpos];
    dst[dstpos + 1] = (BYTE2) src[srcpos + 1];
    dst[dstpos + 2] = (BYTE2) src[srcpos + 2];
}

template<typename T>
void
copy_frame(const T *src, int src_width, int src_height, BYTE2 *dst, int dst_width, int dst_height) {
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < dst_width; j++)
        {
            copy_pixel(src, src_width, src_height, dst, dst_width, dst_height, i, j);
        }
    }

    for (int i = dst_height - 2; i < dst_height; i++)
    {
        for (int j = 0; j < dst_width; j++)
        {
            copy_pixel(src, src_width, src_height, dst, dst_width, dst_height, i, j);
        }
    }

    for (int i = 2; i < dst_height - 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            copy_pixel(src, src_width, src_height, dst, dst_width, dst_height, i, j);
        }
    }

    for (int i = 2; i < dst_height - 2; i++)
    {
        for (int j = dst_width - 2; j < dst_width; j++)
        {
            copy_pixel(src, src_width, src_height, dst, dst_width, dst_height, i, j);
        }
    }
}

template<typename T>
BYTE2* Expand(T* gauss, int width, int height)
{
    int Filter[5][5] = { {1,4,7,4,1},
                         {4,16,26,16,4},
                         {7,26,41,26,7},
                         {4,16,26,16,4},
                         {1,4,7,4,1} }; //273

    auto expwidth = (width - 1) * 2 + 1;
    auto expheight = (height - 1) * 2 + 1;

    BYTE2* expand = new BYTE2[expwidth * expheight * 3];

    int expoff, gaussoff;
    int indexi, indexj;
    int temp1 = 0, temp2 = 0, temp3 = 0;

    copy_frame(gauss, width, height, expand, expwidth, expheight);

    for (int i = 2; i < expheight - 2; i++)
    {
        for (int j = 2; j < expwidth - 2; j++)
        {
            for (int x = -2; x <= 2; x++)
            {
                for (int y = -2; y <= 2; y++)
                {
                    if ((i + x) % 2 == 0 && (j + y) % 2 == 0) {
                        indexi = (i + x) / 2;
                        indexj = (j + y) / 2;
                        gaussoff = (height - indexi - 1) * width * 3 + indexj * 3;
                        temp1 += Filter[x + 2][y + 2] * (int)gauss[gaussoff];
                        temp2 += Filter[x + 2][y + 2] * (int)gauss[gaussoff + 1];
                        temp3 += Filter[x + 2][y + 2] * (int)gauss[gaussoff + 2];
                    }
                }
            }

            temp1 /= 273;
            temp2 /= 273;
            temp3 /= 273;
            expoff = (expheight - i - 1) * expwidth * 3 + j * 3;
            expand[expoff] = BYTE2(temp1 * 4);
            expand[expoff + 1] = BYTE2(temp2 * 4);
            expand[expoff + 2] = BYTE2(temp3 * 4);
        }
    }

    return expand;
}

template<typename T>
BYTE2* Difference(BYTE* Gauss1, T* Gauss2, int width, int height) {

	BYTE2* ExpandGauss = Expand(Gauss2, width, height);
	width = ((width - 1) * 2) + 1;
	height = ((height - 1) * 2) + 1;

	size_t bufpos;
	BYTE2* Laplace = new BYTE2[width * height * 3];

//#pragma omp parallel num_threads(NUM_THREADS) shared(Gauss1, ExpandGauss, Laplace, height, width) private(bufpos)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				bufpos = (height - i - 1) * width * 3 + j * 3;

				Laplace[bufpos + 2] = BYTE2(Gauss1[bufpos + 2]) - ExpandGauss[bufpos + 2];
				Laplace[bufpos + 1] = BYTE2(Gauss1[bufpos + 1]) - ExpandGauss[bufpos + 1];
				Laplace[bufpos] = BYTE2(Gauss1[bufpos]) - ExpandGauss[bufpos];
			}
		}
	}

	delete[] ExpandGauss;
	return Laplace;
}


BYTE GetMeanPiksel(BYTE* Raw, int i, int j, xy Size, int rgb) {
	BYTE piksel[9], temp;

	if (rgb == 1) {
		piksel[0] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + (j - 1) * 3 + 2];
		piksel[1] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + j * 3 + 2];
		piksel[2] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + (j + 1) * 3 + 2];
		piksel[3] = Raw[(Size.y - i - 1) * Size.x * 3 + (j - 1) * 3 + 2];
		piksel[4] = Raw[(Size.y - i - 1) * Size.x * 3 + j * 3 + 2];
		piksel[5] = Raw[(Size.y - i - 1) * Size.x * 3 + (j + 1) * 3 + 2];
		piksel[6] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + (j - 1) * 3 + 2];
		piksel[7] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + j * 3 + 2];
		piksel[8] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + (j + 1) * 3 + 2];
	}

	else if (rgb == 2) {
		piksel[0] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + (j - 1) * 3 + 1];
		piksel[1] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + j * 3 + 1];
		piksel[2] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + (j + 1) * 3 + 1];
		piksel[3] = Raw[(Size.y - i - 1) * Size.x * 3 + (j - 1) * 3 + 1];
		piksel[4] = Raw[(Size.y - i - 1) * Size.x * 3 + j * 3 + 1];
		piksel[5] = Raw[(Size.y - i - 1) * Size.x * 3 + (j + 1) * 3 + 1];
		piksel[6] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + (j - 1) * 3 + 1];
		piksel[7] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + j * 3 + 1];
		piksel[8] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + (j + 1) * 3 + 1];

	}
	else if (rgb == 3) {
		piksel[0] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + (j - 1) * 3];
		piksel[1] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + j * 3];
		piksel[2] = Raw[(Size.y - (i - 1) - 1) * Size.x * 3 + (j + 1) * 3];
		piksel[3] = Raw[(Size.y - i - 1) * Size.x * 3 + (j - 1) * 3];
		piksel[4] = Raw[(Size.y - i - 1) * Size.x * 3 + j * 3];
		piksel[5] = Raw[(Size.y - i - 1) * Size.x * 3 + (j + 1) * 3];
		piksel[6] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + (j - 1) * 3];
		piksel[7] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + j * 3];
		piksel[8] = Raw[(Size.y - (i + 1) - 1) * Size.x * 3 + (j + 1) * 3];
	}

	for (int x = 0; x < 9; x++) {
		for (int y = x + 1; y < 9; y++) {

			if (piksel[x] > piksel[y]) {
				temp = piksel[x];
				piksel[x] = piksel[y];
				piksel[y] = temp;
			}
		}
	}

	return piksel[4];
}

void GoruntuDuzelt(BYTE* Raw, xy Size, BYTE* tempRaw, int Width, int Height) {

	size_t tempR = 0, tempG = 0, tempB = 0, bufpos;

//#pragma omp parallel num_threads(NUM_THREADS) shared(Height, Width, tempRaw, tempR, tempG, tempB) private(bufpos)
	{
//#pragma omp for schedule(dynamic) reduction(+:tempR, tempG, tempB) nowait
		for (int i = 0; i < Height; i++) {
			for (int j = 0; j < Width; j += 10) {
				bufpos = (Height - i - 1) * Width * 3 + j * 3;

				tempR += tempRaw[bufpos + 2];
				tempG += tempRaw[bufpos + 1];
				tempB += tempRaw[bufpos];
			}
		}
	}

	BYTE meanR = BYTE(tempR / (int(Width / 10) * Height));
	BYTE meanG = BYTE(tempG / (int(Width / 10) * Height));
	BYTE meanB = BYTE(tempB / (int(Width / 10) * Height));


//#pragma omp parallel num_threads(NUM_THREADS) private(bufpos)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 1; i < Size.y - 1; i++) {
			for (int j = 1; j < Size.x - 1; j++) {
				bufpos = (Size.y - i - 1) * Size.x * 3 + j * 3;

				if ((meanR == Raw[bufpos + 2]) && (meanG == Raw[bufpos + 1]) && (meanB == Raw[bufpos])) {
					Raw[bufpos + 2] = GetMeanPiksel(Raw, i, j, Size, 1);
					Raw[bufpos + 1] = GetMeanPiksel(Raw, i, j, Size, 2);
					Raw[bufpos] = GetMeanPiksel(Raw, i, j, Size, 3);
				}
			}
		}
	}

}


BYTE* ImageToResized(double** H, BYTE* Raw, int Width, int Height, xy Size, xy position) {

	int sizeX = Size.x;
	int sizeY = Size.y;
	int positionX = position.x;
	int positionY = position.y;

	size_t tempR = 0, tempG = 0, tempB = 0, bufpos;
	BYTE* resizedImg = new BYTE[sizeX * sizeY * 3];

//#pragma omp parallel num_threads(NUM_THREADS) shared(Height, Width, Raw, tempR, tempG, tempB) private(bufpos)
	{
//#pragma omp for schedule(dynamic) reduction(+:tempR, tempG, tempB) nowait
		for (int i = 0; i < Height; i++) {
			for (int j = 0; j < Width; j += 10) {
				bufpos = (Height - i - 1) * Width * 3 + j * 3;
				tempR += Raw[bufpos + 2];
				tempG += Raw[bufpos + 1];
				tempB += Raw[bufpos];
			}
		}
	}

	tempR /= (int(Width / 10) * Height);
	tempG /= (int(Width / 10) * Height);
	tempB /= (int(Width / 10) * Height);

//#pragma omp parallel num_threads(NUM_THREADS) shared(sizeY, sizeX, resizedImg, tempR, tempG, tempB) private(bufpos)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < sizeY; i++) {
			for (int j = 0; j < sizeX; j++) {
				bufpos = (sizeY - i - 1) * sizeX * 3 + j * 3;

				resizedImg[bufpos + 2] = BYTE(tempR);
				resizedImg[bufpos + 1] = BYTE(tempG);
				resizedImg[bufpos] = BYTE(tempB);
			}
		}
	}


//#pragma omp parallel num_threads(NUM_THREADS) shared(Height, Width, sizeY, positionY, sizeX, positionX, Raw, resizedImg)
	{
		xy point;
		int buf; int rawBuf;
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < Height; i++) {
			for (int j = 0; j < Width; j++) {
				point = Transfer(H, j, i);
				buf = (sizeY - (positionY + point.y) - 1) * sizeX * 3 + (point.x + positionX) * 3;

				if (buf > 0) {
					rawBuf = (Height - i - 1) * Width * 3 + j * 3;
					resizedImg[buf] = Raw[rawBuf];
					resizedImg[buf + 1] = Raw[rawBuf + 1];
					resizedImg[buf + 2] = Raw[rawBuf + 2];
				}
			}
		}
	}

	return resizedImg;
}

void LaplacePyramid(double** H, BYTE* panoImg1, BYTE* img2, int panoWidth, int panoHeight, int Width, int Height, BYTE2** LaplacePyramid1, BYTE2** LaplacePyramid2, int& width, int& height, xy newPanoSize, xy panoPosition)
{

	BYTE* Raw1 = PanoToResized(panoImg1, panoWidth, panoHeight, newPanoSize, panoPosition);
	width = newPanoSize.x;
	height = newPanoSize.y;

	BYTE* Gauss1 = SizeExtend(Raw1, width, height, newPanoSize.x, newPanoSize.y);
	delete[] Raw1;
	int twidth = width, theight = height;

	BYTE* Gauss2 = nullptr;
	Gauss2 = GaussPyramid(Gauss1, width, height, Gauss2);
	BYTE2* Laplace1 = Difference(Gauss1, Gauss2, width, height);
	delete[] Gauss1;

	BYTE* Gauss3 = nullptr;
	Gauss3 = GaussPyramid(Gauss2, width, height, Gauss3);
	BYTE2* Laplace2 = Difference(Gauss2, Gauss3, width, height);
	delete[] Gauss2;

	BYTE2* Gauss4 = nullptr;
	Gauss4 = GaussPyramid(Gauss3, width, height, Gauss4);
	BYTE2* Laplace3 = Difference(Gauss3, Gauss4, width, height);
	delete[] Gauss3;

	BYTE2* Laplace4 = Gauss4;

	LaplacePyramid1[0] = Laplace1;
	LaplacePyramid1[1] = Laplace2;
	LaplacePyramid1[2] = Laplace3;
	LaplacePyramid1[3] = Laplace4;


	BYTE* Raw2 = ImageToResized(H, img2, Width, Height, newPanoSize, panoPosition);

	// GoruntuDuzelt(Raw2, newPanoSize, img2, Width, Height);	// Sebebi anlasilamadi sonra irdele
	width = newPanoSize.x;
	height = newPanoSize.y;


	BYTE* Gauss1_ = SizeExtend(Raw2, width, height, newPanoSize.x, newPanoSize.y);
	delete[] Raw2;

	BYTE* Gauss2_ = nullptr;
	Gauss2_ = GaussPyramid(Gauss1_, width, height, Gauss2_);
	BYTE2* Laplace1_ = Difference(Gauss1_, Gauss2_, width, height);
	delete[] Gauss1_;

	BYTE* Gauss3_ = nullptr;
	Gauss3_ = GaussPyramid(Gauss2_, width, height, Gauss3_);
	BYTE2* Laplace2_ = Difference(Gauss2_, Gauss3_, width, height);
	delete[] Gauss2_;

	BYTE2* Gauss4_ = nullptr;
	Gauss4_ = GaussPyramid(Gauss3_, width, height, Gauss4_);
	BYTE2* Laplace3_ = Difference(Gauss3_, Gauss4_, width, height);
	delete[] Gauss3_;

	BYTE2* Laplace4_ = Gauss4_;

	LaplacePyramid2[0] = Laplace1_;
	LaplacePyramid2[1] = Laplace2_;
	LaplacePyramid2[2] = Laplace3_;
	LaplacePyramid2[3] = Laplace4_;

	width = twidth;
	height = theight;
}

BYTE2* SumLaplace(BYTE2* Laplace1, BYTE2* Laplace2, int width, int height) {

//#pragma omp parallel num_threads(NUM_THREADS)
	{
		int bufpos;
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				bufpos = (height - i - 1) * width * 3 + j * 3;
				Laplace1[bufpos + 2] += Laplace2[bufpos + 2];
				Laplace1[bufpos + 1] += Laplace2[bufpos + 1];
				Laplace1[bufpos] += Laplace2[bufpos];
			}
		}
	}

	return Laplace1;
}

BYTE* Filter(BYTE* Raw, double** H, xy position, int Width, int Height, xy newSize, int width, int height, int orWidth, int orHeight, bool isFirstLine, int currCornerID, xy* currVec) {

	/*
	* p1 ******* p3
	  *			 *
	  *	  img2	 *
	  *			 *
	  p2 ******* p4
	*/

	xy point1 = Transfer(H, 0, 0);				
	xy point2 = Transfer(H, 0, orHeight);		
	xy point3 = Transfer(H, orWidth, 0);		
	xy point4 = Transfer(H, orWidth, orHeight); 

	size_t bufpos;

	/*
	* currCornerID
	*
		0 ********* 1
		* 		    *
		*  currVec  *
		* 		    *
		2 ********* 3
	*/

	if (isFirstLine == true) {

		if (currCornerID == 0 && currVec->x != 0 && currVec->y == 0)
		{

			int xExtend = abs(newSize.x - Width);
			if (xExtend == 0) {
				if (point3.x != Width) {
					point3.x -= OFFSET;
					point4.x -= OFFSET;
				}
			}
			if (point2.y != Height) {
				point2.y -= OFFSET;
				point4.y -= OFFSET;
			}

			int xStart = point1.x;
			int xEnd = point4.x - currVec->x;
			int yStart = point1.y;
			int yEnd = point2.y;

			double xLength = xEnd - point1.x;
			double yLength = point2.y - yStart;
			double xRate = xLength / yLength;

			int xTemp = 0, yTemp = 0;
			for (int row = yStart; row <= yEnd; row++) {
				for (int col = xStart + xTemp; col <= point4.x; col++)
				{
					bufpos = (height - row - 1) * width * 3 + col * 3;
					Raw[bufpos + 2] = 0;
					Raw[bufpos + 1] = 0;
					Raw[bufpos] = 0;
				}
				yTemp++;
				xTemp = int(xRate * yTemp);
			}

		}

		else if (currCornerID == 2 || currCornerID == 3) {

			if (currCornerID == 2 && currVec->x == 0 && currVec->y != 0) {

				int yExtend = abs(newSize.y - Height);
				if (yExtend > 0) {
					point1.y = 0;
					point2.y += yExtend;
					point3.y = 0;
					point4.y += yExtend;
				}
				else {
					yExtend = currVec->y;
				}

				double xLength = (point4.x - point2.x) / 2;
				double yLength = point2.y - point1.y - yExtend;
				double xRate = xLength / yLength;
				int xTemp = 0, yTemp = 0;

				int xStart = point1.x;
				int xEnd = point3.x;
				int yStart = point1.y + yExtend;
				int yEnd = point2.y;


				for (int row = point1.y; row < yStart; row++) {
					for (int col = xStart; col <= xEnd; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}

				for (int row = yStart; row <= yEnd; row++) {
					for (int col = xStart + xTemp; col <= xEnd - xTemp; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}

			}

			else if (currCornerID == 2 && currVec->x != 0 && currVec->y != 0)
			{

				int yExtend = abs(newSize.y - Height);
				int xExtend = abs(newSize.x - Width);
				if (yExtend > 0) {
					point1.y = 0;
					point2.y += yExtend;
					point3.y = 0;
					point4.y += yExtend;
				}
				
				yExtend = currVec->y;
				

				if (xExtend == 0) {
					xExtend = currVec->x;
					if (point3.x != Width) {
						point3.x -= OFFSET;
						point4.x -= OFFSET;
					}
					point2.y -= OFFSET;
					point4.y -= OFFSET;
				}


				int xStart = point1.x;
				int xEnd = point3.x;
				int yStart = point1.y + yExtend;
				int yEnd = point2.y;

				double xLength = point3.x - point1.x - xExtend;
				double yLength = yEnd - yStart;
				double xRate = xLength / yLength;
				int xTemp = 0, yTemp = 0;

				for (int row = point1.y; row < yStart; row++) {
					for (int col = point1.x; col <= point3.x; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}

				for (int row = yStart; row <= yEnd; row++) {
					for (int col = xStart + xTemp; col <= xEnd; col++)
					{
						if (col >= point1.x) {

							bufpos = (height - row - 1) * width * 3 + col * 3;
							Raw[bufpos + 2] = 0;
							Raw[bufpos + 1] = 0;
							Raw[bufpos] = 0;
						}
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}

			}
			
			else
			{

				int yExtend = abs(newSize.y - Height);
				int xExtend = abs(newSize.x - Width);

				if (xExtend > 0) {
					point1.x = 0;
					point2.x = 0;
					point3.x += xExtend;
					point4.x += xExtend;
				}
				if (yExtend > 0) {
					point1.y = 0;
					point2.y += yExtend;
					point3.y = 0;
					point4.y += yExtend;
				}

				if (xExtend == 0) {
					xExtend = currVec->x;
					if (point1.x != 0) {
						point1.x += OFFSET;
						point2.x += OFFSET;
					}
					if (point3.x != Width) {
						point3.x -= OFFSET;
						point4.x -= OFFSET;
					}
					if (yExtend > 0 && point2.y != Height) {
						point2.y -= OFFSET;
						point4.y -= OFFSET;
					}
				}
				if (yExtend == 0) {
					yExtend = currVec->y;
					if (point1.y != 0) {
						point1.y += OFFSET;
						point3.y += OFFSET;
					}
					if (point2.y != Height) {
						point2.y -= OFFSET;		
						point4.y -= OFFSET;
					}
					if (xExtend > 0 && point3.x != Width) {
						point3.x -= OFFSET;
						point4.x -= OFFSET;
					}
				};


				int xStart = point1.x;
				int xEnd = point2.x + xExtend;
				int yStart = point1.y;
				int yEnd = point1.y + yExtend;

				double xLength = point3.x - xEnd;
				double yLength = point2.y - yStart - yExtend;
				double xRate = xLength / yLength;

				for (int row = yStart; row < yEnd; row++) {
					for (int col = xStart; col <= point3.x; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}

				int xTemp = 0, yTemp = int(yLength);
				for (int row = yEnd; row <= point2.y; row++) {

					yTemp--;
					xTemp = int(xRate * yTemp);
					for (int col = xStart; col <= xEnd + xTemp; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}
			}
		}
		else
		{
			int xExtend = abs(newSize.x - Width);
			if (xExtend > 0) {

				point1.x = 0;
				point2.x = 0;
				point3.x += xExtend;
				point4.x += xExtend;
			}
			else {
				xExtend = currVec->x;
				point2.y -= OFFSET;
				point4.y -= OFFSET;
				if (point1.x != 0) {
					point1.x += OFFSET;
					point2.x += OFFSET;
				}
			}

			int xStart = point1.x;
			int xEnd = point2.x + xExtend;
			int yStart = point1.y;
			int yEnd = point2.y;

			double xLength = point3.x - xEnd;
			double yLength = point2.y - yStart;
			double xRate = xLength / yLength;

			int xTemp = 0, yTemp = int(yLength);
			for (int row = yStart; row <= yEnd; row++) {

				yTemp--;
				xTemp = int(xRate * yTemp);
				for (int col = xStart; col <= xEnd + xTemp; col++)
				{
					bufpos = (height - row - 1) * width * 3 + col * 3;
					Raw[bufpos + 2] = 0;
					Raw[bufpos + 1] = 0;
					Raw[bufpos] = 0;
				}
			}
		}
	}
	else {

		if (currVec->x != 0 && currVec->y == 0) {

			if (currCornerID == 0) {

				int xExtend = abs(newSize.x - Width);

				if (xExtend == 0) {
					xExtend = currVec->x;

					if (point3.x != Width) {
						point3.x -= OFFSET;
						point4.x -= OFFSET;
					}
				}

				point1.y += OFFSET;
				point3.y += OFFSET;

				if (point2.y != Height) {
					point2.y -= OFFSET;
					point4.y -= OFFSET;
				}


				int xStart = point3.x - xExtend;
				int xEnd = point3.x;
				int yStart = point1.y;
				int yEnd = point2.y;

				double xLength = xStart - point1.x;
				double yLength = yEnd - yStart;
				double xRate = xLength / yLength;

				int xTemp = 0, yTemp = 0;
				for (int row = yStart; row <= yEnd; row++) {
					for (int col = xStart - xTemp; col <= xEnd; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}
			}

			else {

				int xExtend = abs(newSize.x - Width);
				if (xExtend > 0) {
					point1.x = 0;
					point2.x = 0;
					point3.x += xExtend;
					point4.x += xExtend;
				}
				else {
					xExtend = currVec->x;
					point1.y += OFFSET;
					point3.y += OFFSET;
					if (point1.x != 0) {
						point1.x += OFFSET;
						point2.x += OFFSET;
					}
				}

				int xStart = point1.x;
				int xEnd = point1.x + xExtend;
				int yStart = point1.y;
				int yEnd = point2.y;

				double xLength = point3.x - point1.x - xExtend;
				double yLength = point2.y - yStart;
				double xRate = xLength / yLength;

				int xTemp = 0, yTemp = 0;
				for (int row = yStart; row <= yEnd; row++) {
					for (int col = xStart; col <= xEnd + xTemp; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}
			}
		}

		else if (currVec->x == 0 && currVec->y != 0) {

			if (currCornerID == 0) {

				int yExtend = abs(newSize.y - Height);
				if (yExtend == 0) {
					if (point2.y != Height) {
						point2.y -= OFFSET;
						point4.y -= OFFSET;
					}
				}
				if (point1.x != 0) {
					point1.x += OFFSET;
					point2.x += OFFSET;
				}
				if (point3.x != Width) {
					point3.x -= OFFSET;
					point4.x -= OFFSET;
				}


				double xLength = (point3.x - point1.x) / 2;
				double yLength = point2.y - point1.y - currVec->y;
				double xRate = xLength / yLength;
				int xTemp = 0, yTemp = 0;

				int xStart = point1.x + int(xLength);
				int xEnd = point1.x + int(xLength);
				int yStart = point1.y;
				int yEnd = point1.y + int(yLength);

				for (int row = yStart; row < yEnd; row++) {
					for (int col = xStart - xTemp; col <= xEnd + xTemp; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}

				for (int row = yEnd; row <= point2.y; row++) {
					for (int col = point1.x; col <= point3.x; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}
			}

			else {

				point1.y += OFFSET;
				point3.y += OFFSET;

				if (point1.x != 0) {
					point1.x += OFFSET;
					point2.x += OFFSET;
				}
				if (point3.x != Width) {
					point3.x -= OFFSET;
					point4.x -= OFFSET;
				}

				double xLength = (point4.x - point2.x) / 2;
				double yLength = point2.y - point1.y - currVec->y;
				double xRate = xLength / yLength;
				int xTemp = 0, yTemp = 0;

				int xStart = point1.x;
				int xEnd = point3.x;
				int yStart = point1.y + currVec->y;
				int yEnd = point2.y;

				for (int row = point1.y; row < yStart; row++) {
					for (int col = xStart; col <= xEnd; col++) {

						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}


				for (int row = yStart; row <= yEnd; row++) {
					for (int col = xStart + xTemp; col <= xEnd - xTemp; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}
			}
		}

		else if (currVec->x != 0 && currVec->y != 0) {

			if (currCornerID == 0) {

				int xExtend = abs(newSize.x - Width);
				int yExtend = abs(newSize.y - Height);
				if (xExtend > 0 && yExtend > 0) {
					point1.y += OFFSET;
					point3.y += OFFSET;
					point1.x += OFFSET;
					point2.x += OFFSET;
				}
				else if (xExtend > 0 && yExtend == 0) {
					point1.y += OFFSET;
					point3.y += OFFSET;

					if (point2.y != Height) {
						point2.y -= OFFSET;
						point4.y -= OFFSET;
					}
					point1.x += OFFSET;
					point2.x += OFFSET;
				}
				else if (xExtend == 0 && yExtend > 0) {
					if (point3.x != Width) {
						point3.x -= OFFSET;
						point4.x -= OFFSET;
					}
					point1.x += OFFSET;
					point2.x += OFFSET;
					point1.y += OFFSET;
					point3.y += OFFSET;
				}
				else {
					point1.y += OFFSET;
					point3.y += OFFSET;

					if (point2.y != Height) {
						point2.y -= OFFSET;
						point4.y -= OFFSET;
					}

					point1.x += OFFSET;
					point2.x += OFFSET;

					if (point3.x != Width) {
						point3.x -= OFFSET;
						point4.x -= OFFSET;
					}
				}


				int xStart = point3.x - currVec->x;
				int xEnd = point3.x;
				int yStart = point3.y;
				int yEnd = point4.y - currVec->y;

				double xLength = xStart - point1.x;
				double yLength = yEnd - yStart;
				double xRate = xLength / yLength;
				int xTemp = 0, yTemp = 0;

				for (size_t row = yStart; row < yEnd; row++) {
					for (size_t col = xStart - xTemp; col <= xEnd; col++)
					{

						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}
				for (size_t row = yEnd; row <= point4.y; row++) {
					for (size_t col = point1.x; col <= point4.x; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}

			}

			else if (currCornerID == 1) {

				int xExtend = abs(newSize.x - Width);
				int yExtend = abs(newSize.y - Height);
				if (xExtend > 0 && yExtend > 0) {
					point1.x = 0;
					point2.x = 0;
					point3.x += xExtend;
					point4.x += xExtend;

					point1.y += OFFSET;
					point3.y += OFFSET;
					point3.x -= OFFSET;
					point4.x -= OFFSET;
				}
				else if (xExtend > 0 && yExtend == 0) {
					point1.x = 0;
					point2.x = 0;
					point3.x += xExtend;
					point4.x += xExtend;

					point1.y += OFFSET;
					point3.y += OFFSET;
					if (point2.y != Height) {
						point2.y -= OFFSET;
						point4.y -= OFFSET;
					}
					point3.x -= OFFSET;
					point4.x -= OFFSET;
				}
				else if (xExtend == 0 && yExtend > 0) {
					xExtend = currVec->x;

					point3.x -= OFFSET;
					point4.x -= OFFSET;

					if (point2.x != 0) {
						point2.x += OFFSET;
						point1.x += OFFSET;
					}
					point1.y += OFFSET;
					point3.y += OFFSET;
				}
				else {
					xExtend = currVec->x;

					if (point1.x != 0) {
						point1.x += OFFSET;
						point2.x += OFFSET;
					}
					point3.x -= OFFSET;
					point4.x -= OFFSET;
					point1.y += OFFSET;
					point3.y += OFFSET;

					if (point2.y != Height) {
						point2.y -= OFFSET;
						point4.y -= OFFSET;
					}
				}


				int xStart = point1.x;
				int xEnd = point1.x + xExtend;
				int yStart = point1.y;
				int yEnd = point4.y - currVec->y;

				double xLength = point3.x - xEnd;
				double yLength = yEnd - yStart;
				double xRate = xLength / yLength;
				int xTemp = 0, yTemp = 0;

				for (size_t row = yStart; row < yEnd; row++) {
					for (size_t col = xStart; col <= xEnd + xTemp; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}
				for (size_t row = yEnd; row <= point4.y; row++) {
					for (size_t col = xStart; col <= point4.x; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}
			}

			else if (currCornerID == 2) {

				int xExtend = abs(newSize.x - Width);
				if (xExtend == 0) {
					if (point3.x != Width) {
						point3.x -= OFFSET;
						point4.x -= OFFSET;
					}
				}

				point1.y += OFFSET;
				point3.y += OFFSET;
				point2.y -= OFFSET;
				point4.y -= OFFSET;
				point1.x += OFFSET;
				point2.x += OFFSET;

				int xStart = point1.x;
				int xEnd = point3.x;
				int yStart = point1.y + currVec->y;
				int yEnd = point2.y;

				double xLength = point3.x - point1.x - currVec->x;
				double yLength = yEnd - yStart;
				double xRate = xLength / yLength;
				int xTemp = 0, yTemp = 0;

				for (int row = point1.y; row < yStart; row++) {
					for (int col = point1.x; col <= point3.x; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}

				for (int row = yStart; row <= yEnd; row++) {
					for (int col = xStart + xTemp; col <= xEnd; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
					yTemp++;
					xTemp = int(xRate * yTemp);
				}
			}

			else {

				
				int xExtend = abs(newSize.x - Width);
				if (xExtend > 0) {
					
					point1.x = 0;
					point2.x = 0;
					point3.x += xExtend;
					point4.x += xExtend;
				}
				
				else {
					xExtend = currVec->x;
					
					if (point1.x != 0) {
						point1.x += OFFSET;
						point2.x += OFFSET;
					}
				}

				point1.y += OFFSET;
				point3.y += OFFSET;
				point2.y -= OFFSET;
				point4.y -= OFFSET;
				point3.x -= OFFSET;
				point4.x -= OFFSET;


				int xStart = point1.x;
				int xEnd = point2.x + xExtend;
				int yStart = point1.y;
				int yEnd = point1.y + currVec->y;

				double xLength = point3.x - xEnd;
				double yLength = point2.y - yStart - currVec->y;
				double xRate = xLength / yLength;

				for (size_t row = yStart; row < yEnd; row++) {
					for (size_t col = xStart; col <= point3.x; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}

				int xTemp = 0, yTemp = int(yLength);
				for (size_t row = yEnd; row <= point2.y; row++) {

					yTemp--;
					xTemp = int(xRate * yTemp);
					for (size_t col = xStart; col <= xEnd + xTemp; col++)
					{
						bufpos = (height - row - 1) * width * 3 + col * 3;
						Raw[bufpos + 2] = 0;
						Raw[bufpos + 1] = 0;
						Raw[bufpos] = 0;
					}
				}
			}
		}
	}

	return Raw;
}


BYTE* PanaromicImage(double** H, int Width, int Height, xy Size, xy position, BYTE2** LaplacePyramid1, BYTE2** LaplacePyramid2, int& width, int& height, int orWidth, int orHeight, bool isFirstLine, int currCornerID, xy* currVec)
{
	int bufpos1, bufpos2;
	BYTE* M = new BYTE[width * height * 3];

	memset(M, (BYTE)255, height * width * 3);

	M = Filter(M, H, position, Width, Height, Size, width, height, orWidth, orHeight, isFirstLine, currCornerID, currVec);

	BYTE* MGauss1 = M;
	BYTE2* SLaplace1 = new BYTE2[width * height * 3];
//#pragma omp parallel num_threads(NUM_THREADS) private(bufpos1)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				bufpos1 = (height - i - 1) * width * 3 + j * 3;
				SLaplace1[bufpos1 + 2] = BYTE2(LaplacePyramid1[0][bufpos1 + 2] * (MGauss1[bufpos1 + 2] / 255.0) + LaplacePyramid2[0][bufpos1 + 2] * ((255 - MGauss1[bufpos1 + 2]) / 255.0));
				SLaplace1[bufpos1 + 1] = BYTE2(LaplacePyramid1[0][bufpos1 + 1] * (MGauss1[bufpos1 + 1] / 255.0) + LaplacePyramid2[0][bufpos1 + 1] * ((255 - MGauss1[bufpos1 + 1]) / 255.0));
				SLaplace1[bufpos1] = BYTE2(LaplacePyramid1[0][bufpos1] * (MGauss1[bufpos1] / 255.0) + LaplacePyramid2[0][bufpos1] * ((255 - MGauss1[bufpos1]) / 255.0));
			}
		}
	}


	BYTE* MGauss2 = nullptr;
	MGauss2 = GaussPyramid(MGauss1, width, height, MGauss2);
	delete[] MGauss1;
	delete[] LaplacePyramid1[0];
	delete[] LaplacePyramid2[0];

	BYTE2* SLaplace2 = new BYTE2[width * height * 3];
//#pragma omp parallel num_threads(NUM_THREADS) private(bufpos1)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				bufpos1 = (height - i - 1) * width * 3 + j * 3;
				SLaplace2[bufpos1 + 2] = BYTE2(LaplacePyramid1[1][bufpos1 + 2] * (MGauss2[bufpos1 + 2] / 255.0) + LaplacePyramid2[1][bufpos1 + 2] * ((255 - MGauss2[bufpos1 + 2]) / 255.0));
				SLaplace2[bufpos1 + 1] = BYTE2(LaplacePyramid1[1][bufpos1 + 1] * (MGauss2[bufpos1 + 1] / 255.0) + LaplacePyramid2[1][bufpos1 + 1] * ((255 - MGauss2[bufpos1 + 1]) / 255.0));
				SLaplace2[bufpos1] = BYTE2(LaplacePyramid1[1][bufpos1] * (MGauss2[bufpos1] / 255.0) + LaplacePyramid2[1][bufpos1] * ((255 - MGauss2[bufpos1]) / 255.0));
			}
		}
	}

	BYTE* MGauss3 = nullptr;
	MGauss3 = GaussPyramid(MGauss2, width, height, MGauss3);
	delete[] MGauss2;
	delete[] LaplacePyramid1[1];
	delete[] LaplacePyramid2[1];

	BYTE2* SLaplace3 = new BYTE2[width * height * 3];
//#pragma omp parallel num_threads(NUM_THREADS) private(bufpos1)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				bufpos1 = (height - i - 1) * width * 3 + j * 3;
				SLaplace3[bufpos1 + 2] = BYTE2(LaplacePyramid1[2][bufpos1 + 2] * (MGauss3[bufpos1 + 2] / 255.0) + LaplacePyramid2[2][bufpos1 + 2] * ((255 - MGauss3[bufpos1 + 2]) / 255.0));
				SLaplace3[bufpos1 + 1] = BYTE2(LaplacePyramid1[2][bufpos1 + 1] * (MGauss3[bufpos1 + 1] / 255.0) + LaplacePyramid2[2][bufpos1 + 1] * ((255 - MGauss3[bufpos1 + 1]) / 255.0));
				SLaplace3[bufpos1] = BYTE2(LaplacePyramid1[2][bufpos1] * (MGauss3[bufpos1] / 255.0) + LaplacePyramid2[2][bufpos1] * ((255 - MGauss3[bufpos1]) / 255.0));

			}
		}
	}

	BYTE* MGauss4 = nullptr;
	MGauss4 = GaussPyramid(MGauss3, width, height, MGauss4);
	delete[] MGauss3;
	delete[] LaplacePyramid1[2];
	delete[] LaplacePyramid2[2];

	BYTE2* SLaplace4 = new BYTE2[width * height * 3];
//#pragma omp parallel num_threads(NUM_THREADS) private(bufpos1)
	{
//#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				bufpos1 = (height - i - 1) * width * 3 + j * 3;
				SLaplace4[bufpos1 + 2] = BYTE2(LaplacePyramid1[3][bufpos1 + 2] * (MGauss4[bufpos1 + 2] / 255.0) + LaplacePyramid2[3][bufpos1 + 2] * ((255 - MGauss4[bufpos1 + 2]) / 255.0));
				SLaplace4[bufpos1 + 1] = BYTE2(LaplacePyramid1[3][bufpos1 + 1] * (MGauss4[bufpos1 + 1] / 255.0) + LaplacePyramid2[3][bufpos1 + 1] * ((255 - MGauss4[bufpos1 + 1]) / 255.0));
				SLaplace4[bufpos1] = BYTE2(LaplacePyramid1[3][bufpos1] * (MGauss4[bufpos1] / 255.0) + LaplacePyramid2[3][bufpos1] * ((255 - MGauss4[bufpos1]) / 255.0));

			}
		}
	}

	delete[] MGauss4;
	delete[] LaplacePyramid1[3];
	delete[] LaplacePyramid2[3];


	BYTE2* EL4 = Expand(SLaplace4, width, height);
	width = ((width - 1) * 2) + 1;
	height = ((height - 1) * 2) + 1;
	SLaplace3 = SumLaplace(SLaplace3, EL4, width, height);

	delete[] SLaplace4;
	delete[] EL4;

	BYTE2* EL3 = Expand(SLaplace3, width, height);
	width = ((width - 1) * 2) + 1;
	height = ((height - 1) * 2) + 1;
	SLaplace2 = SumLaplace(SLaplace2, EL3, width, height);

	delete[] SLaplace3;
	delete[] EL3;

	BYTE2* EL2 = Expand(SLaplace2, width, height);
	width = ((width - 1) * 2) + 1;
	height = ((height - 1) * 2) + 1;
	SLaplace1 = SumLaplace(SLaplace1, EL2, width, height);

	delete[] SLaplace2;
	delete[] EL2;

	int sizeX = Size.x;
	int sizeY = Size.y;
	BYTE* panorama = new BYTE[sizeX * sizeY * 3];

//#pragma omp parallel shared(sizeX, sizeY, width, height, panorama, SLaplace1) private(bufpos1, bufpos2)
	{
//ß#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < sizeY; i++) {
			for (int j = 0; j < sizeX; j++) {
				bufpos1 = (sizeY - i - 1) * sizeX * 3 + j * 3;
				bufpos2 = (height - i - 1) * width * 3 + j * 3;

				panorama[bufpos1 + 2] = (BYTE)SLaplace1[bufpos2 + 2];
				panorama[bufpos1 + 1] = (BYTE)SLaplace1[bufpos2 + 1];
				panorama[bufpos1] = (BYTE)SLaplace1[bufpos2];
			}
		}
	}

	delete[] SLaplace1;
	return panorama;
}