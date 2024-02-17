#include "Image_Stitching.h"
#include "Definitions.h"
#include "Image.h"
#include "Panorama.h"
#include "Process.h"
#include "Correlation.h"
#include "Dots.h"
#include "Homography.h"


void MaxPOC(const double *POC, xy *pocDot, int width, int height) {

    double max = -10000000.0;
    int i = 0;
    for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++) {
            i = row * width + col;

            if (POC[i] > max) {
                max = POC[i];
                pocDot->y = row;
                pocDot->x = col;
            }
        }
}

int LineUpdate(int lineCounter, int cornerID, xy *vec) {

    switch (cornerID) {
        case 0:
        case 1:
            lineCounter += vec->y;
            return lineCounter;

        case 2:
        case 3:
            lineCounter -= vec->y;
            if (lineCounter < 0) lineCounter = 0;
            return lineCounter;

        default:
            return lineCounter;
    }
}

void stitch_images(const std::vector<path_t> &input_files, const path_t &output_file) {
    BYTE *buffer1, *buffer2;
    bool isFirstLine = true;
    int currCornerID;
    xy *currVec = new xy;
    xy *prevVec = new xy;
    xy *pocDot = new xy;
    xy *prevPanoSize = new xy;
    xy *currPanoSize;
    BYTE2 **LaplacePyramid1;
    BYTE2 **LaplacePyramid2;
    BYTE *panorama1;
    BYTE *panorama2;
    BYTE *intensity1;
    BYTE *intensity2;

    auto **outReal = new double *[2];
    auto **outImag = new double *[2];
    auto **match1 = new double *[3];
    auto **match2 = new double *[3];

    long size;
    int width, height;
    int width1, height1;
    xy position;

    int imgCounter = 0;
    int lineCounter = 0;

    while (imgCounter < input_files.size() - 1) {

        if (imgCounter == 0) {
            const auto &path1 = input_files[imgCounter];
            // LoadBMP can read only 24 bit image depth
            buffer1 = LoadBMP(width, height, size, path1.c_str());
            intensity1 = ConvertBMPToIntensity(buffer1, width, height);

            outReal[0] = new double[width * height];
            outImag[0] = new double[width * height];
            FFT2D(intensity1, outReal[0], outImag[0], width, height);


            const auto &path2 = input_files[imgCounter + 1];
            // LoadBMP can read only 24 bit image depth
            buffer2 = LoadBMP(width, height, size, path2.c_str());
            intensity2 = ConvertBMPToIntensity(buffer2, width, height);

            outReal[1] = new double[width * height];
            outImag[1] = new double[width * height];
            FFT2D(intensity2, outReal[1], outImag[1], width, height);


            // Phase Only Correlation result
            double *POC = PhaseCorrelation(outReal[0], outImag[0], outReal[1], outImag[1], width, height);
            MaxPOC(POC, pocDot, width, height);

            delete[] outReal[0];
            delete[] outImag[0];
            delete[] POC;

            currCornerID = ZoneDetection(intensity1, intensity2, width, height, currVec, pocDot);
            xy *img1Dots = Rand4Dots(currCornerID, pocDot, width, height);

            if (img1Dots != nullptr) {
                {
                    xy *img2Dots = MatchingDots(currCornerID, img1Dots, currVec);
                    lineCounter = LineUpdate(lineCounter, currCornerID, currVec);

                    for (int i = 0; i < 3; i++) {
                        match1[i] = new double[4];
                        match2[i] = new double[4];
                    }
                    for (int i = 0; i < 4; i++) {
                        match1[0][i] = double(img1Dots[i].x);
                        match1[1][i] = double(img1Dots[i].y);
                        match1[2][i] = 1.0;
                        match2[0][i] = double(img2Dots[i].x);
                        match2[1][i] = double(img2Dots[i].y);
                        match2[2][i] = 1.0;
                    }

                    double **homography = homography2d(match1, match2, 4);

                    LaplacePyramid1 = new BYTE2 *[4];
                    LaplacePyramid2 = new BYTE2 *[4];
                    currPanoSize = ReSizePanorama(homography, width, height, width, height, position);

                    if (lineCounter == 0)
                        isFirstLine = true;
                    else isFirstLine = false;

                    LaplacePyramid(homography, buffer1, buffer2, width, height, width, height, LaplacePyramid1,
                                   LaplacePyramid2, width1, height1, *currPanoSize, position);
                    panorama1 = PanaromicImage(homography, width, height, *currPanoSize, position, LaplacePyramid1,
                                               LaplacePyramid2, width1, height1, width, height, isFirstLine,
                                               currCornerID, currVec);

                    prevPanoSize->x = currPanoSize->x;
                    prevPanoSize->y = currPanoSize->y;
                    prevVec->x = 0;
                    prevVec->y = 0;
                    UpdatePrevVec(currCornerID, prevVec, currVec);

                    delete[] img2Dots;
                    for (size_t i = 0; i < 3; i++)
                        delete[] homography[i];
                    delete[] homography;
                }

                delete[] intensity1;
                intensity1 = intensity2;
                delete[] currPanoSize;
                delete[] buffer1;
                delete[] img1Dots;
            }
        } else {
            buffer1 = buffer2;
            outReal[0] = outReal[1];
            outImag[0] = outImag[1];

            const auto &path = input_files[imgCounter + 1];
            buffer2 = LoadBMP(width, height, size, path.c_str());
            intensity2 = ConvertBMPToIntensity(buffer2, width, height);

            outReal[1] = new double[width * height];
            outImag[1] = new double[width * height];

            FFT2D(intensity2, outReal[1], outImag[1], width, height);

            // Phase Only Correlation result
            double *POC = PhaseCorrelation(outReal[0], outImag[0], outReal[1], outImag[1], width, height);
            MaxPOC(POC, pocDot, width, height);

            delete[] outReal[0];
            delete[] outImag[0];
            delete[] POC;


            currCornerID = ZoneDetection(intensity1, intensity2, width, height, currVec, pocDot);
            xy *img1Dots = Rand4Dots(currCornerID, pocDot, width, height);

            if (img1Dots != nullptr) {
                xy *img2Dots = MatchingDots(currCornerID, img1Dots, currVec);
                lineCounter = LineUpdate(lineCounter, currCornerID, currVec);
                xy *img1PanoDots = PanoDots(prevVec, currCornerID, img1Dots);

                for (int i = 0; i < 4; i++) {
                    match1[0][i] = double(img1PanoDots[i].x);
                    match1[1][i] = double(img1PanoDots[i].y);
                    match1[2][i] = 1.0;
                    match2[0][i] = double(img2Dots[i].x);
                    match2[1][i] = double(img2Dots[i].y);
                    match2[2][i] = 1.0;
                }

                double **homography = homography2d(match1, match2, 4);
                currPanoSize = ReSizePanorama(homography, prevPanoSize->x, prevPanoSize->y, width, height,
                                              position);

                if (lineCounter == 0)
                    isFirstLine = true;
                else isFirstLine = false;

                LaplacePyramid(homography, panorama1, buffer2, prevPanoSize->x, prevPanoSize->y, width, height,
                               LaplacePyramid1, LaplacePyramid2, width1, height1, *currPanoSize, position);
                panorama2 = PanaromicImage(homography, prevPanoSize->x, prevPanoSize->y, *currPanoSize, position,
                                           LaplacePyramid1, LaplacePyramid2, width1, height1, width, height,
                                           isFirstLine, currCornerID, currVec);

                UpdatePrevVec(currCornerID, prevVec, currVec);


                delete[] img1PanoDots;
                delete[] img2Dots;

                for (int i = 0; i < 3; i++)
                    delete[] homography[i];
                delete[] homography;
            }

            prevPanoSize->x = currPanoSize->x;
            prevPanoSize->y = currPanoSize->y;

            delete[] panorama1;
            panorama1 = panorama2;
            delete[] intensity1;
            intensity1 = intensity2;

            delete[] currPanoSize;
            delete[] buffer1;
            delete[] img1Dots;
        }
        imgCounter++;
    }
    if (panorama1 != nullptr) {
        SaveBMP(
                panorama1,
                prevPanoSize->x,
                prevPanoSize->y,
                prevPanoSize->x * prevPanoSize->y * 3,
                output_file.c_str());

        delete[] panorama1;
    }
    for (size_t i = 0; i < 3; i++) {
        delete[] match1[i];
        delete[] match2[i];
    }
}