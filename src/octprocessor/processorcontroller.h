#ifndef PROCESSORCONTROLLER_H
#define PROCESSORCONTROLLER_H

#define SETTINGS_DIR QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
#define SETTINGS_FILE_NAME "settings.ini"
#define SETTINGS_PATH SETTINGS_DIR + "/" + SETTINGS_FILE_NAME
#define SETTINGS_PATH_BACKGROUND_FILE SETTINGS_DIR + "/background.csv"
#define SETTINGS_PATH_RESAMPLING_FILE SETTINGS_DIR + "/resampling.csv"

#include <QObject>
#include <QVector>
#include <QStandardPaths>
#include "processor.h"

class ProcessorController : public QObject {
	Q_OBJECT

public:
	struct ProcessingSettings {
		int bitDepth;
		size_t samplesPerSpectrum;
		size_t spectraPerFrame;
		size_t framesPerVolume;
		size_t rollingAverageWindowSize;

		// Processing options
		OCTSignalProcessing::Processor<float>::ProcessingOptions processingOptions;

		// Dispersion coefficients
		std::vector<float> dispersionCoefficients;

		// Resampling coefficients
		std::vector<float> resamplingCoefficients;

		// File path to custom resampling curve
		std::string filePathCustomResamplingCurve;

		// Log scaling parameters
		float logScaleCoeff;
		float logScaleMin;
		float logScaleMax;
		float logScaleAddend;
		bool autoComputeLogScaleMinMax;
	};

	ProcessingSettings settings_;

	explicit ProcessorController(QObject *parent = nullptr);
	void setProcessingSettings(const ProcessingSettings& settings);
	void setDispersionCoefficients(qreal d2, qreal d3);
	void loadSettingsFromFile(QString filePath);
	void loadCustomResamplingCurveFromFile(QString filePath);

	bool processData(const QByteArray& rawData, QVector<float>& outputData);

private:

};

#endif // PROCESSORCONTROLLER_H
