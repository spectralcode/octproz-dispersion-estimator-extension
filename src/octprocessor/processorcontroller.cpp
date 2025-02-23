#include "processorcontroller.h"
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

ProcessorController::ProcessorController(QObject *parent)
	: QObject(parent) {
	// Initialize default settings if needed
}

void ProcessorController::setProcessingSettings(const ProcessingSettings& settings) {
	settings_ = settings;
}

void ProcessorController::setDispersionCoefficients(qreal d2, qreal d3) {
	this->settings_.dispersionCoefficients[2] = static_cast<float>(d2);
	this->settings_.dispersionCoefficients[3] = static_cast<float>(d3);
}

void ProcessorController::loadSettingsFromFile(QString filePath) {
	// Check if the file exists; if not, you can bail out or create a default ini
	if (!QFileInfo::exists(filePath)) {
		qDebug() << "Settings file does not exist:" << filePath;
		return;
	}

	QSettings settingsFile(filePath, QSettings::IniFormat);

	//load data dimension settings
	settingsFile.beginGroup("Virtual OCT System");
	settings_.bitDepth = settingsFile.value("bit_depth", 12).toInt();
	settings_.samplesPerSpectrum = settingsFile.value("width", 1024).toUInt();
	settings_.spectraPerFrame = settingsFile.value("height", 512).toUInt();
	settings_.framesPerVolume = settingsFile.value("buffers_per_volume", 1).toUInt();
	settingsFile.endGroup();

	//load processing settings
	settingsFile.beginGroup("processing");
	settings_.rollingAverageWindowSize = settingsFile.value("background_removal_window_size", 64).toUInt();
	settings_.processingOptions.removeDC = settingsFile.value("background_removal", false).toBool();
	settings_.processingOptions.resample = settingsFile.value("resampling", false).toBool();
	settings_.processingOptions.useCustomResamplingCurve = settingsFile.value("custom_resampling", false).toBool();
	settings_.filePathCustomResamplingCurve = settingsFile.value("custom_resampling_filepath", "").toString().toStdString();


	//todo: maybe implement all interpolation methods in the CPU processing class that are available in the GPU processing
	//int interpolationIdx = settingsFile.value("resampling_interpolation", 0).toInt(); // e.g., 0=LINEAR, 1=CUBIC, etc.
	//settings_.processingOptions.interpolationMethod = static_cast<InterpolationMethod>(interpolationIdx);

	double c0 = settingsFile.value("resampling_c0", 0.0).toDouble();
	double c1 = settingsFile.value("resampling_c1", 0.0).toDouble();
	double c2 = settingsFile.value("resampling_c2", 0.0).toDouble();
	double c3 = settingsFile.value("resampling_c3", 0.0).toDouble();
	settings_.resamplingCoefficients = {
		static_cast<float>(c0),
		static_cast<float>(c1),
		static_cast<float>(c2),
		static_cast<float>(c3)
	};

	settings_.processingOptions.compensateDispersion = settingsFile.value("dispersion_compensation", false).toBool();
	double d0 = settingsFile.value("dispersion_compensation_d0", 0.0).toDouble();
	double d1 = settingsFile.value("dispersion_compensation_d1", 0.0).toDouble();
	double d2 = settingsFile.value("dispersion_compensation_d2", 0.0).toDouble();
	double d3 = settingsFile.value("dispersion_compensation_d3", 0.0).toDouble();
	settings_.dispersionCoefficients = {
		static_cast<float>(d0),
		static_cast<float>(d1),
		static_cast<float>(d2),
		static_cast<float>(d3)
	};

	settings_.processingOptions.applyWindow = settingsFile.value("windowing", true).toBool();

	// todo: maybe implement Bitshift / flipping B-scans in CPU processing
	//settings_.processingOptions.bitShift = settingsFile.value("bitshift", false).toBool();
	//settings_.processingOptions.flipBscans = settingsFile.value("flip_bscans", false).toBool();

	// Log scaling
	settings_.processingOptions.logScale = settingsFile.value("log", true).toBool();
	settings_.logScaleMin = settingsFile.value("min", 0.0).toDouble();
	settings_.logScaleMax = settingsFile.value("max", 100.0).toDouble();
	settings_.logScaleCoeff = settingsFile.value("coeff", 1.0).toDouble();
	settings_.logScaleAddend = settingsFile.value("addend", 0.0).toDouble();
	settings_.autoComputeLogScaleMinMax = false;

	//todo: maybe impelment Post-processing background removal in CPU processing
	//settings_.processingOptions.postProcessBackgroundRemoval = settingsFile.value("post_processing_background_removal", false).toBool();
	//settings_.postProcessBackgroundOffset = settingsFile.value("post_processing_background_removal_offset", 0.0).toDouble();
	//settings_.postProcessBackgroundWeight = settingsFile.value("post_processing_background_removal_weight", 1.0).toDouble();

	settingsFile.endGroup();

	qDebug() << "Settings loaded from:" << filePath;
}

void ProcessorController::loadCustomResamplingCurveFromFile(QString filePath){
	settings_.filePathCustomResamplingCurve = filePath.toStdString();
}

bool ProcessorController::processData(const QByteArray& rawData, QVector<float>& outputData) {
	using T = float;

	size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(settings_.bitDepth)/8.0));
	size_t totalSamples = rawData.size() / bytesPerSample;
	//size_t expectedSamples = settings_.samplesPerSpectrum * settings_.spectraPerFrame * settings_.framesPerVolume;

//	if (totalSamples != expectedSamples) {
//		QMessageBox::critical(nullptr, tr("Error"), tr("Raw data size does not match the specified parameters."));
//		return false;
//	}

	// Create the processor
	OCTSignalProcessing::Processor<T> processor(settings_.samplesPerSpectrum, settings_.rollingAverageWindowSize);

	// Set processing options
	processor.setProcessingOptions(settings_.processingOptions);

	// Set dispersion coefficients
	processor.setDispersionCoefficients(settings_.dispersionCoefficients);

	// Set resampling coefficients
	processor.setResamplingCoefficients(settings_.resamplingCoefficients);
	processor.setCustomResamplingCurve(settings_.filePathCustomResamplingCurve);

	// Set log scaling parameters
	processor.setLogScaleParameters(settings_.logScaleCoeff, settings_.logScaleMin, settings_.logScaleMax,
	                                settings_.logScaleAddend, settings_.autoComputeLogScaleMinMax);

	// Processed data output
	std::vector<std::vector<std::vector<T>>> processedData;

	// Process the raw data
	processor.processRawData(rawData.constData(), totalSamples, settings_.bitDepth, settings_.spectraPerFrame, processedData);

	// Fill outputData with processed data
	if (!processedData.empty() && !processedData[0].empty()) {
		outputData.clear();
		for (const auto& spectrum : processedData[0]) {
			outputData.append(QVector<float>::fromStdVector(spectrum));
		}
		return true;
	}

	return false;
}
