#ifndef OCT_SIGNAL_PROCESSING_H
#define OCT_SIGNAL_PROCESSING_H

#include <vector>
#include <complex>
#include <cstdint>
#include <fftw3.h>

namespace OCTSignalProcessing {

template <typename T>
class Processor {
public:
	// Processing options structure
	struct ProcessingOptions {
		bool removeDC = true;
		bool resample = true;
		bool useCustomResamplingCurve = false;
		bool compensateDispersion = true;
		bool applyWindow = true;
		bool computeIFFT = true;
		bool logScale = true;
	};

	// Constructor and destructor
	Processor(size_t samplesPerSpectrum, size_t windowSize = 0, int kernelRadius = 8, size_t rollingAverageWindowSize = 10);
	~Processor();

	// Set processing options
	void setProcessingOptions(const ProcessingOptions& options);

	// Set dispersion coefficients
	void setDispersionCoefficients(const std::vector<T>& phaseCoefficients, T factor = static_cast<T>(1.0), int direction = 1);

	// Set resampling polynomial coefficients
	void setResamplingCoefficients(const std::vector<T>& coefficients);

	void setCustomResamplingCurve(const std::string& filePath);

	// Set log scaling parameters
	void setLogScaleParameters(T coeff = static_cast<T>(1.0),
	                           T minVal = static_cast<T>(0.0),
	                           T maxVal = static_cast<T>(0.0),
	                           T addend = static_cast<T>(0.0),
	                           bool autoComputeMinMax = true);

	// Process raw data
	void processRawData(const void* inputData,
	                    size_t totalSamples,
	                    int inputBitDepth,
	                    size_t spectraPerFrame,
	                    std::vector<std::vector<std::vector<T>>>& processedData);

private:
	// Member variables
	size_t samplesPerSpectrum_;
	size_t windowSize_;
	int kernelRadius_;
	size_t rollingAverageWindowSize_;
	std::vector<T> windowFunction_;
	fftw_plan fftPlan_;
	fftw_complex* fftIn_;
	fftw_complex* fftOut_;
	const T PI_;
	const T EPSILON_;

	// Processing options
	ProcessingOptions options_;

	// Dispersion compensation parameters
	std::vector<T> dispersionCoefficients_;
	std::vector<std::complex<T>> phaseComplex_;
	T dispersionFactor_;
	int dispersionDirection_;

	// Resampling parameters
	std::vector<T> resamplePositions_;
	std::vector<T> resamplingCoefficients_; // Polynomial coefficients for resampling curve
	std::vector<T> coefficientResamplingCurve_;
	std::vector<T> customResamplingCurve_;
	bool hasCustomResamplingCurve_;
	bool resamplingCurveOptionsChanged_;

	// Log scaling parameters
	T logScaleCoeff_;
	T logScaleMin_;
	T logScaleMax_;
	T logScaleAddend_;
	bool autoComputeLogScaleMinMax_;

	// Private methods
	void generateWindow();
	void computeDispersivePhase();
	void generateResampleCurve();
	void generateCoefficientResamplingCurve();

	// Data conversion
	void convertInputData(const void* inputData,
	                      size_t totalSamples,
	                      int inputBitDepth,
	                      std::vector<std::complex<T>>& outputData);

	// Processing steps
	void rollingAverageDCRemoval(std::vector<std::complex<T>>& spectrum);

	void klinearizationCubic(const std::vector<std::complex<T>>& inputSpectrum,
	                         const std::vector<T>& resampleCurve,
	                         std::vector<std::complex<T>>& outputSpectrum);

	void dispersionCompensation(std::vector<std::complex<T>>& data);

	void applyWindow(std::vector<std::complex<T>>& data);

	void computeIFFT(const std::vector<std::complex<T>>& input,
	                 std::vector<std::complex<T>>& output);

	void logScale(const std::vector<std::complex<T>>& input,
	              std::vector<T>& output);

	// Helper functions
	T cubicHermiteInterpolation(const T y0, const T y1, const T y2, const T y3, const T positionBetweenY1andY2);

	static const T& clamp(const T& value, const T& low, const T& high);
};

} // namespace OCTSignalProcessing

#include "processor.tpp"

#endif // OCT_SIGNAL_PROCESSING_H
