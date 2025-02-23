#include "ascanmetriccalculator.h"
#include <algorithm>
#include <cmath>

AscanMetricCalculator::AscanMetricCalculator()
{
	//default-initialize params_
	params_.sharpnessMetric = ASCAN_SHARPNESS_METRIC::SUM_ABOVE_THRESHOLD;
}

AscanMetricCalculator::AscanMetricCalculator(const DispersionEstimatorParameters &params)
	: params_(params)
{
}

void AscanMetricCalculator::setParameters(const DispersionEstimatorParameters &params)
{
	params_ = params;
}

float AscanMetricCalculator::calculateMetric(const QVector<float> &outputData, int samplesPerLine)
{
	if(samplesPerLine <= 0 || outputData.isEmpty()) {
		return 0.0f;
	}

	int totalSamples = outputData.size();
	int totalLines = totalSamples / samplesPerLine;

	float totalMetric = 0.0f;

	for(int lineIndex = 0; lineIndex < totalLines; ++lineIndex) {
		int lineStart = lineIndex * samplesPerLine;

		// Respect the 'numberOfAscanSamplesToIgnore'
		int ignoreCount = params_.numberOfAscanSamplesToIgnore;
		if(ignoreCount > 0) {
			ignoreCount = std::min(ignoreCount, samplesPerLine); // clamp
		}

		// The pointer to the first sample we want to consider in this line
		const float *lineData = outputData.constData() + lineStart + ignoreCount;
		int validLineSize = samplesPerLine - ignoreCount;
		if(validLineSize <= 0) {
			continue; // skip line if ignoring everything
		}

		float lineMetric = 0.0f;
		switch (params_.sharpnessMetric) {
		case SUM_ABOVE_THRESHOLD: {
			lineMetric = computeSumAboveThreshold(lineData, validLineSize);
			break;
		}
		case SAMPLES_ABOVE_THRESHOLD: {
			lineMetric = computeNumberOfSamplesAboveThreshold(lineData, validLineSize);
			break;
		}
		case PEAK_VALUE: {
			lineMetric = computePeakValue(lineData, validLineSize);
			break;
		}
		case MEAN_SOBEL: {
			lineMetric = computeMeanSobel(lineData, validLineSize);
			break;
		}
		default:
		//todo addd more metrics
			break;
		}

		totalMetric += lineMetric;
	}

	return totalMetric;
}

float AscanMetricCalculator::computeSumAboveThreshold(const float *lineData, int lineSize) const
{
	float sum = 0.0f;
	float thr = static_cast<float>(params_.metricThreshold);
	for(int i = 0; i < lineSize; ++i) {
		float val = lineData[i];
		if (val > thr) {
			sum += val;
		}
	}
	return sum;
}

float AscanMetricCalculator::computeNumberOfSamplesAboveThreshold(const float *lineData, int lineSize) const
{
	int numberOfSamples = 0;
	float thr = static_cast<float>(params_.metricThreshold);
	for(int i = 0; i < lineSize; ++i) {
		float val = lineData[i];
		if (val > thr) {
			numberOfSamples++;
		}
	}
	return static_cast<float>(numberOfSamples);
}

float AscanMetricCalculator::computePeakValue(const float *lineData, int lineSize) const
{
	float maxVal = 0.0f;
	for(int i = 0; i < lineSize; ++i) {
		maxVal = std::max(maxVal, lineData[i]);
	}
	return maxVal;
}

float AscanMetricCalculator::computeMeanSobel(const float *lineData, int lineSize) const
{
	if (lineSize < 3) {
		return 0.0f; // Not enough samples for a Sobel approach
	}

	float sumAbsGradient = 0.0f;
	int count = 0;
	// skip the boundary samples
	for (int i = 1; i < lineSize - 1; ++i) {
		float grad = (lineData[i+1] - lineData[i-1]) * 0.5f;
		sumAbsGradient += std::fabs(grad);
		count++;
	}

	return (count > 0) ? (sumAbsGradient / count) : 0.0f;
}
