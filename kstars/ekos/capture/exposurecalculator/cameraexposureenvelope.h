/*
    SPDX-FileCopyrightText: 2023 Joseph McGee <joseph.mcgee@sbcglobal.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractItemModel>
#include <QVector>
#include "calculatedgainsubexposuretime.h"

namespace OptimalExposure
{
class CameraExposureEnvelope
{
    public:
        CameraExposureEnvelope() {}
        CameraExposureEnvelope(double lightPollutionElectronBaseRate, double lightPollutionForOpticFocalRatio,
                               const QVector<CalculatedGainSubExposureTime> &aSubExposureVector, double exposureTimeMin, double exposureTimeMax);

        double getLightPollutionElectronBaseRate() const;
        double getLightPollutionForOpticFocalRatio() const;
        const QVector<CalculatedGainSubExposureTime> &getASubExposureVector() const;
        double getExposureTimeMin() const;
        double getExposureTimeMax() const;

    private:
        double lightPollutionElectronBaseRate;
        double lightPollutionForOpticFocalRatio;
        QVector<CalculatedGainSubExposureTime> aSubExposureVector;
        double exposureTimeMin;
        double exposureTimeMax;
};

}
