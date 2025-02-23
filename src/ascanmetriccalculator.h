#ifndef ASCANMETRICCALCULATOR_H
#define ASCANMETRICCALCULATOR_H

#include <QVector>
#include "dispersionestimatorparameters.h"

class AscanMetricCalculator
{
public:
	AscanMetricCalculator();
	explicit AscanMetricCalculator(const DispersionEstimatorParameters &params);

	void setParameters(const DispersionEstimatorParameters &params);
	float calculateMetric(const QVector<float> &outputData, int samplesPerLine);

private:
	DispersionEstimatorParameters params_;

	float computeSumAboveThreshold(const float *lineData, int lineSize) const;
	float computeNumberOfSamplesAboveThreshold(const float *lineData, int lineSize) const;
	float computePeakValue(const float *lineData, int lineSize) const;
	float computeMeanSobel(const float *lineData, int lineSize) const;
};

#endif // ASCANMETRICCALCULATOR_H
