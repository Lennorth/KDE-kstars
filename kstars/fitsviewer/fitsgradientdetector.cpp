/***************************************************************************
                          fitsgradientdetector.cpp  -  FITS Image
                             -------------------
    begin                : Sat March 28 2020
    copyright            : (C) 2004 by Jasem Mutlaq, (C) 2020 by Eric Dejouhanet
    email                : eric.dejouhanet@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Some code fragments were adapted from Peter Kirchgessner's FITS plugin*
 *   See http://members.aol.com/pkirchg for more details.                  *
 ***************************************************************************/

#include <math.h>
#include <cmath>

#include "fits_debug.h"
#include "fitsgradientdetector.h"

FITSStarDetector& FITSGradientDetector::configure(const QString &, const QVariant &)
{
    return *this;
}

int FITSGradientDetector::findSources(QList<Edge*> &starCenters, const QRect &boundary)
{
    switch (parent()->property("dataType").toInt())
    {
        case TBYTE:
            return findSources<uint8_t>(starCenters, boundary);

        case TSHORT:
            return findSources<int16_t>(starCenters, boundary);

        case TUSHORT:
            return findSources<uint16_t>(starCenters, boundary);

        case TLONG:
            return findSources<int32_t>(starCenters, boundary);

        case TULONG:
            return findSources<uint16_t>(starCenters, boundary);

        case TFLOAT:
            return findSources<float>(starCenters, boundary);

        case TLONGLONG:
            return findSources<int64_t>(starCenters, boundary);

        case TDOUBLE:
            return findSources<double>(starCenters, boundary);

        default:
            break;
    }

    return 0;
}

template <typename T>
int FITSGradientDetector::findSources(QList<Edge*> &starCenters, const QRect &boundary)
{
    FITSData const * const data = reinterpret_cast<FITSData const *>(parent());

    if (data == nullptr)
        return 0;

    int subX = qMax(0, boundary.isNull() ? 0 : boundary.x());
    int subY = qMax(0, boundary.isNull() ? 0 : boundary.y());
    int subW = (boundary.isNull() ? data->width() : boundary.width());
    int subH = (boundary.isNull() ? data->height() : boundary.height());

    int BBP = data->getBytesPerPixel();

    uint16_t dataWidth = data->width();

    // #1 Find offsets
    uint32_t size   = subW * subH;
    uint32_t offset = subX + subY * dataWidth;

    // #2 Create new buffer
    auto * buffer = new uint8_t[size * BBP];
    // If there is no offset, copy whole buffer in one go
    if (offset == 0)
        memcpy(buffer, data->getImageBuffer(), size * BBP);
    else
    {
        uint8_t * dataPtr = buffer;
        uint8_t const * origDataPtr = data->getImageBuffer();
        uint32_t lineOffset  = 0;
        // Copy data line by line
        for (int height = subY; height < (subY + subH); height++)
        {
            lineOffset = (subX + height * dataWidth) * BBP;
            memcpy(dataPtr, origDataPtr + lineOffset, subW * BBP);
            dataPtr += (subW * BBP);
        }
    }

    // #3 Create new FITSData to hold it
    auto * boundedImage                      = new FITSData();
    FITSImage::Statistic stats;
    stats.width               = subW;
    stats.height              = subH;
    stats.dataType              = data->getStatistics().dataType;
    stats.bytesPerPixel       = data->getStatistics().bytesPerPixel;
    stats.samples_per_channel = size;
    stats.ndim                = 2;
    boundedImage->restoreStatistics(stats);

    // #4 Set image buffer and calculate stats.
    boundedImage->setImageBuffer(buffer);

    boundedImage->calculateStats(true);

    // #5 Apply Median + High Contrast filter to remove noise and move data to non-linear domain
    boundedImage->applyFilter(FITS_MEDIAN);
    boundedImage->applyFilter(FITS_HIGH_CONTRAST);

    // #6 Perform Sobel to find gradients and their directions
    QVector<float> gradients;
    QVector<float> directions;

    // TODO Must trace neighbours and assign IDs to each shape so that they can be centered massed
    // and discarded whenever necessary. It won't work on noisy images unless this is done.
    sobel<T>(boundedImage, gradients, directions);

    QVector<int> ids(gradients.size());

    int maxID = partition(subW, subH, gradients, ids);

    // Not needed anymore
    delete boundedImage;

    if (maxID == 0)
        return 0;

    typedef struct
    {
        float massX     = 0;
        float massY     = 0;
        float totalMass = 0;
    } massInfo;

    QMap<int, massInfo> masses;

    // #7 Calculate center of mass for all detected regions
    for (int y = 0; y < subH; y++)
    {
        for (int x = 0; x < subW; x++)
        {
            int index = x + y * subW;

            int regionID = ids[index];
            if (regionID > 0)
            {
                float pixel = gradients[index];

                masses[regionID].totalMass += pixel;
                masses[regionID].massX += x * pixel;
                masses[regionID].massY += y * pixel;
            }
        }
    }

    // Compare multiple masses, and only select the highest total mass one as the desired star
    int maxRegionID       = 1;
    int maxTotalMass      = masses[1].totalMass;
    double totalMassRatio = 1e6;
    for (auto key : masses.keys())
    {
        massInfo oneMass = masses.value(key);
        if (oneMass.totalMass > maxTotalMass)
        {
            totalMassRatio = oneMass.totalMass / maxTotalMass;
            maxTotalMass   = oneMass.totalMass;
            maxRegionID    = key;
        }
    }

    // If image has many regions and there is no significant relative center of mass then it's just noise and no stars
    // are probably there above a useful threshold.
    if (maxID > 10 && totalMassRatio < 1.5)
        return 0;

    auto * center  = new Edge;
    center->width = -1;
    center->x     = masses[maxRegionID].massX / masses[maxRegionID].totalMass + 0.5;
    center->y     = masses[maxRegionID].massY / masses[maxRegionID].totalMass + 0.5;
    center->HFR   = 1;

    // Maximum Radius
    int maxR = qMin(subW - 1, subH - 1) / 2;

    for (int r = maxR; r > 1; r--)
    {
        int pass = 0;

        for (float theta = 0; theta < 2 * M_PI; theta += (2 * M_PI) / 36.0)
        {
            int testX = center->x + std::cos(theta) * r;
            int testY = center->y + std::sin(theta) * r;

            // if out of bound, break;
            if (testX < 0 || testX >= subW || testY < 0 || testY >= subH)
                break;

            if (gradients[testX + testY * subW] > 0)
                //if (thresholded[testX + testY * subW] > 0)
            {
                if (++pass >= 24)
                {
                    center->width = r * 2;
                    // Break of outer loop
                    r = 0;
                    break;
                }
            }
        }
    }

    qCDebug(KSTARS_FITS) << "FITS: Weighted Center is X: " << center->x << " Y: " << center->y << " Width: " << center->width;

    // If no stars were detected
    if (center->width == -1)
    {
        delete center;
        return 0;
    }

    // 30% fuzzy
    //center->width += center->width*0.3 * (running_threshold / threshold);

    double FSum = 0, HF = 0, TF = 0;
    const double resolution = 1.0 / 20.0;

    int cen_y = qRound(center->y);

    double rightEdge = center->x + center->width / 2.0;
    double leftEdge  = center->x - center->width / 2.0;

    QVector<double> subPixels;
    subPixels.reserve(center->width / resolution);

    const T * origBuffer = reinterpret_cast<T const *>(data->getImageBuffer()) + offset;

    for (double x = leftEdge; x <= rightEdge; x += resolution)
    {
        double slice = resolution * (origBuffer[static_cast<int>(floor(x)) + cen_y * dataWidth]);
        FSum += slice;
        subPixels.append(slice);
    }

    // Half flux
    HF = FSum / 2.0;

    int subPixelCenter = (center->width / resolution) / 2;

    // Start from center
    TF            = subPixels[subPixelCenter];
    double lastTF = TF;
    // Integrate flux along radius axis until we reach half flux
    //for (double k=resolution; k < (center->width/(2*resolution)); k += resolution)
    for (int k = 1; k < subPixelCenter; k++)
    {
        TF += subPixels[subPixelCenter + k];
        TF += subPixels[subPixelCenter - k];

        if (TF >= HF)
        {
            // We overpassed HF, let's calculate from last TF how much until we reach HF

            // #1 Accurate calculation, but very sensitive to small variations of flux
            center->HFR = (k - 1 + ((HF - lastTF) / (TF - lastTF)) * 2) * resolution;

            // #2 Less accurate calculation, but stable against small variations of flux
            //center->HFR = (k - 1) * resolution;
            break;
        }

        lastTF = TF;
    }

    // Correct center for subX and subY
    center->x += subX;
    center->y += subY;

    //data->appendStar(center);
    starCenters.append(center);

    qCDebug(KSTARS_FITS) << "Flux: " << FSum << " Half-Flux: " << HF << " HFR: " << center->HFR;

    return 1;
}

/* CannyDetector, Implementation of Canny edge detector in Qt/C++.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: https://github.com/hipersayanX/CannyDetector
 */

template <typename T>
void FITSGradientDetector::sobel(FITSData const *data, QVector<float> &gradient, QVector<float> &direction) const
{
    if (data == nullptr)
        return;

    FITSImage::Statistic const &stats = data->getStatistics();

    //int size = image.width() * image.height();
    gradient.resize(stats.samples_per_channel);
    direction.resize(stats.samples_per_channel);

    for (int y = 0; y < stats.height; y++)
    {
        size_t yOffset    = y * stats.width;
        const T * grayLine = reinterpret_cast<T const *>(data->getImageBuffer()) + yOffset;

        const T * grayLine_m1 = y < 1 ? grayLine : grayLine - stats.width;
        const T * grayLine_p1 = y >= stats.height - 1 ? grayLine : grayLine + stats.width;

        float * gradientLine  = gradient.data() + yOffset;
        float * directionLine = direction.data() + yOffset;

        for (int x = 0; x < stats.width; x++)
        {
            int x_m1 = x < 1 ? x : x - 1;
            int x_p1 = x >= stats.width - 1 ? x : x + 1;

            int gradX = grayLine_m1[x_p1] + 2 * grayLine[x_p1] + grayLine_p1[x_p1] - grayLine_m1[x_m1] -
                        2 * grayLine[x_m1] - grayLine_p1[x_m1];

            int gradY = grayLine_m1[x_m1] + 2 * grayLine_m1[x] + grayLine_m1[x_p1] - grayLine_p1[x_m1] -
                        2 * grayLine_p1[x] - grayLine_p1[x_p1];

            gradientLine[x] = qAbs(gradX) + qAbs(gradY);

            /* Gradient directions are classified in 4 possible cases
             *
             * dir 0
             *
             * x x x
             * - - -
             * x x x
             *
             * dir 1
             *
             * x x /
             * x / x
             * / x x
             *
             * dir 2
             *
             * \ x x
             * x \ x
             * x x \
             *
             * dir 3
             *
             * x | x
             * x | x
             * x | x
             */
            if (gradX == 0 && gradY == 0)
                directionLine[x] = 0;
            else if (gradX == 0)
                directionLine[x] = 3;
            else
            {
                qreal a = 180. * atan(qreal(gradY) / gradX) / M_PI;

                if (a >= -22.5 && a < 22.5)
                    directionLine[x] = 0;
                else if (a >= 22.5 && a < 67.5)
                    directionLine[x] = 2;
                else if (a >= -67.5 && a < -22.5)
                    directionLine[x] = 1;
                else
                    directionLine[x] = 3;
            }
        }
    }
}

int FITSGradientDetector::partition(int width, int height, QVector<float> &gradient, QVector<int> &ids) const
{
    int id = 0;

    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int index = x + y * width;
            float val = gradient[index];
            if (val > 0 && ids[index] == 0)
            {
                trace(width, height, ++id, gradient, ids, x, y);
            }
        }
    }

    // Return max id
    return id;
}

void FITSGradientDetector::trace(int width, int height, int id, QVector<float> &image, QVector<int> &ids, int x, int y) const
{
    int yOffset      = y * width;
    float * cannyLine = image.data() + yOffset;
    int * idLine      = ids.data() + yOffset;

    if (idLine[x] != 0)
        return;

    idLine[x] = id;

    for (int j = -1; j < 2; j++)
    {
        int nextY = y + j;

        if (nextY < 0 || nextY >= height)
            continue;

        float * cannyLineNext = cannyLine + j * width;

        for (int i = -1; i < 2; i++)
        {
            int nextX = x + i;

            if (i == j || nextX < 0 || nextX >= width)
                continue;

            if (cannyLineNext[nextX] > 0)
            {
                // Trace neighbors.
                trace(width, height, id, image, ids, nextX, nextY);
            }
        }
    }
}
