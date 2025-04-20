#include "dispersionestimationengine.h"
#include <QtMath>
#include <QDebug>

DispersionEstimationEngine::DispersionEstimationEngine(QObject *parent)
	: QObject(parent),
	isPeakFitting(false)
{
	this->processorController = new ProcessorController(this);

	this->bestMetricValueD2 = 0;
	this->bestD2 = 0;
	this->bestMetricValueD3 = 0;
	this->bestD3 = 0;
}

void DispersionEstimationEngine::setParams(DispersionEstimatorParameters params) {
	this->params = params;
}

void DispersionEstimationEngine::startDispersionEstimation(void *frameBuffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame)
{
	emit statusUpdate(tr("Estimation process started..."));
	emit estimationProcessStarted();

	// Load processing settings
	qDebug() << "Loading processing settings...";
	this->processorController->loadSettingsFromFile(SETTINGS_PATH);
	if(this->params.useLinearAscans){
		this->processorController->settings_.processingOptions.logScale = false;
	} else {
		this->processorController->settings_.processingOptions.logScale = true;
	}
	if(this->processorController->settings_.processingOptions.useCustomResamplingCurve){
			this->processorController->loadCustomResamplingCurveFromFile(SETTINGS_PATH_RESAMPLING_FILE); //this loads the resampling curve data that is used by OCTproZ
	}
	unsigned int centerAscans = qMin(static_cast<unsigned int>(this->params.numberOfCenterAscans), linesPerFrame);
	this->processorController->settings_.samplesPerSpectrum = samplesPerLine;
	this->processorController->settings_.spectraPerFrame = centerAscans;
	this->processorController->settings_.bitDepth = bitDepth;

	// Calculate offsets for extracting the center region
	unsigned int offsetAscans = 0;
	if (centerAscans < linesPerFrame) {
		offsetAscans = (linesPerFrame - centerAscans) / 2;
	}

	// Calculate byte offsets
	size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(bitDepth)/8.0));
	size_t lineSizeBytes = samplesPerLine * bytesPerSample;
	size_t offsetBytes = offsetAscans * lineSizeBytes;
	size_t partialBytes = centerAscans * lineSizeBytes;

	// Extract center A-scans into QByteArray
	QByteArray rawData(
		reinterpret_cast<const char*>(frameBuffer) + offsetBytes,
		static_cast<int>(partialBytes)
	);

	// Initialize dispersion parameters
	this->bestD2 = 0;
	this->bestD3 = 0;
	this->bestMetricValueD2 = 0;
	this->bestMetricValueD3 = 0;
	qreal d2 = this->params.d2start;
	qreal d3 = this->params.d3start;
	qreal d3_zero = 0.0;
	qreal stepSizeD2 = qAbs(this->params.d2end - this->params.d2start) / static_cast<qreal>(this->params.numberOfDispersionSamples);
	qreal stepSizeD3 = qAbs(this->params.d3end - this->params.d3start) / static_cast<qreal>(this->params.numberOfDispersionSamples);

	// Set initial dispersion coefficients
	this->processorController->setDispersionCoefficients(d2, d3);

	// Set metric calculator parameters once
	this->calculator.setParameters(this->params);

	// Process dispersion for d2
	this->bestMetricValueD2 = 0;
	this->bestMetricValueD3 = 0;
	for (int i = 0; i < this->params.numberOfDispersionSamples; i++) {
		this->processDispersionMetric(rawData, d2, d3_zero, stepSizeD2, true);
	}

	// Process dispersion for d3
	this->bestMetricValueD2 = 0;
	this->bestMetricValueD3 = 0;
	for (int j = 0; j < this->params.numberOfDispersionSamples; j++) {
		processDispersionMetric(rawData, this->bestD2, d3, stepSizeD3, false);
	}

	// Generate Ascan without dispersion compensation and one with disp. compensation using bestD2 and bestD3 and plot both
	QVector<float> ascanWithoutDispersionCompensation = this->processFirstLineOnly(rawData, 0, 0);
	emit ascanWithoutDispersionCalculated(ascanWithoutDispersionCompensation);
	QVector<float> ascanWithBestDispersion = this->processFirstLineOnly(rawData, this->bestD2, this->bestD3);
	emit ascanWithBestDispersionCalculated(ascanWithBestDispersion);

	// transfer and display best dispersion values
	emit dispersionEstimationReady(nullptr, nullptr, &this->bestD2, &this->bestD3);
	emit bestD2Estimated(this->bestD2);
	emit bestD3Estimated(this->bestD3);

	emit statusUpdate(tr("Ready for next operation."));
}

void DispersionEstimationEngine::processDispersionMetric(QByteArray &rawData, qreal &d2, qreal &d3, qreal stepSize, bool isD2)
{
	// Update the coefficient being tested
	this->processorController->setDispersionCoefficients(d2, d3);

	// Process QByteArray with raw data
	QVector<float> outputData;
	qDebug() << "Processing OCT data...";
	emit statusUpdate(tr("Processing OCT data..."));
	bool success = this->processorController->processData(rawData, outputData);
	if (!success) {
		qDebug() << "Processing failed!";
		emit statusUpdate(tr("Processing Ofailed"));
		return;
	}

	// Calculate Ascan sharpness metric
	qDebug() << "Calculating metric value...";
	int samplesPerLine = static_cast<int>(this->processorController->settings_.samplesPerSpectrum / 2);
	float metricValue = this->calculator.calculateMetric(outputData, samplesPerLine);

	// Emit metric value signal based on the coefficient being changed
	if (isD2) {
		if(this->bestMetricValueD2 < metricValue){
			this->bestMetricValueD2 = metricValue;
			this->bestD2 = d2;
		}
		emit metricValueCalculatedD2(d2, metricValue);
		d2 += stepSize;
	} else {
		if(this->bestMetricValueD3 < metricValue){
			this->bestMetricValueD3 = metricValue;
			this->bestD3 = d3;
		}
		emit metricValueCalculatedD3(d3, metricValue);
		d3 += stepSize;
	}

	QCoreApplication::processEvents();
}

QVector<float> DispersionEstimationEngine::processFirstLineOnly(QByteArray &rawData, qreal d2, qreal d3)
{
	this->processorController->setDispersionCoefficients(d2, d3);
	size_t originalSpectraPerFrame = this->processorController->settings_.spectraPerFrame;
	size_t originalFramesPerVolume = this->processorController->settings_.framesPerVolume;
	this->processorController->settings_.spectraPerFrame = 1;
	this->processorController->settings_.framesPerVolume = 1;
	size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(this->processorController->settings_.bitDepth) / 8.0));
	size_t samplesPerSpectrum = this->processorController->settings_.samplesPerSpectrum;
	size_t lineSizeBytes = samplesPerSpectrum * bytesPerSample;
	
	unsigned int centerAscans = qMin(static_cast<unsigned int>(this->params.numberOfCenterAscans), static_cast<unsigned int>(originalSpectraPerFrame));
	unsigned int offsetAscans = 0;
	
	if (centerAscans < static_cast<unsigned int>(originalSpectraPerFrame)) {
		offsetAscans = static_cast<unsigned int>((originalSpectraPerFrame - static_cast<size_t>(centerAscans)) / 2);
	}
	
	size_t offsetBytes = static_cast<size_t>(offsetAscans) * lineSizeBytes;
	
	QByteArray firstCenterAscanData(
		rawData.constData() + offsetBytes,
		static_cast<int>(lineSizeBytes)
	);
	
	QVector<float> outputData;
	qDebug() << "Processing first center A-scan...";
	bool success = this->processorController->processData(firstCenterAscanData, outputData);
	this->processorController->settings_.spectraPerFrame = originalSpectraPerFrame;
	this->processorController->settings_.framesPerVolume = originalFramesPerVolume;
	
	if (!success) {
		qDebug() << "Processing failed!";
		emit statusUpdate(tr("Processing results failed!"));
		return QVector<float>();
	}
	return outputData;
}