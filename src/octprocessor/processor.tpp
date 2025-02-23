#ifndef PROCESSOR_TPP
#define PROCESSOR_TPP

#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace OCTSignalProcessing {

template <typename T>
Processor<T>::Processor(size_t samplesPerSpectrum, size_t windowSize, int kernelRadius, size_t rollingAverageWindowSize)
	: samplesPerSpectrum_(samplesPerSpectrum),
	  windowSize_(windowSize),
	  kernelRadius_(kernelRadius),
	  rollingAverageWindowSize_(rollingAverageWindowSize),
	  PI_(static_cast<T>(3.14159265358979323846)),
	  EPSILON_(std::numeric_limits<T>::epsilon()),
	  fftIn_(nullptr),
	  fftOut_(nullptr),
	  dispersionFactor_(static_cast<T>(1.0)),
	  dispersionDirection_(1),
	  // Initialize log scaling parameters
	  logScaleCoeff_(static_cast<T>(1.0)),
	  logScaleMin_(static_cast<T>(0.0)),
	  logScaleMax_(static_cast<T>(0.0)),
	  logScaleAddend_(static_cast<T>(0.0)),
	  autoComputeLogScaleMinMax_(true),
	  resamplingCurveOptionsChanged_(true){
	// Allocate FFTW arrays
	fftIn_ = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * samplesPerSpectrum_);
	fftOut_ = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * samplesPerSpectrum_);
	fftPlan_ = fftw_plan_dft_1d(static_cast<int>(samplesPerSpectrum_), fftIn_, fftOut_, FFTW_BACKWARD, FFTW_ESTIMATE);

	// Generate window function
	generateWindow();
}

template <typename T>
Processor<T>::~Processor() {
	fftw_destroy_plan(fftPlan_);
	fftw_free(fftIn_);
	fftw_free(fftOut_);
}

template <typename T>
void Processor<T>::setProcessingOptions(const ProcessingOptions& options) {
	if (options_.useCustomResamplingCurve != options.useCustomResamplingCurve) {
		resamplingCurveOptionsChanged_ = true;
	}
	options_ = options;
}

template <typename T>
void Processor<T>::setDispersionCoefficients(const std::vector<T>& phaseCoefficients, T factor, int direction) {
	dispersionCoefficients_ = phaseCoefficients;
	dispersionFactor_ = factor;
	dispersionDirection_ = direction;
	// Compute the phase complex values
	computeDispersivePhase();
}

template <typename T>
void Processor<T>::setResamplingCoefficients(const std::vector<T>& coefficients) {
	resamplingCoefficients_ = coefficients;
	// Generate the resample curve based on the new coefficients
	//generateResampleCurve();
	generateCoefficientResamplingCurve();
	resamplingCurveOptionsChanged_ = true;
}

template <typename T>
void Processor<T>::setCustomResamplingCurve(const std::string& filePath) {
	if (filePath.size() < 1){
		return;
	}
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("Unable to open custom resampling curve file.");
	}

	customResamplingCurve_.clear();
	std::string line;

	// Skip the header line
	std::getline(file, line);

	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string sampleNumber, sampleValue;

		if (std::getline(iss, sampleNumber, ';') && std::getline(iss, sampleValue)) {
			try {
				T value = std::stod(sampleValue);
				customResamplingCurve_.push_back(value);
			} catch (const std::exception& e) {
				throw std::runtime_error("Error parsing sample value: " + sampleValue);
			}
		}
	}

	if (customResamplingCurve_.size() != samplesPerSpectrum_) {
		return; //todo: proper error handling
		throw std::runtime_error("Custom resampling curve size does not match samplesPerSpectrum.");
	}

	hasCustomResamplingCurve_ = true;
	resamplingCurveOptionsChanged_ = true;
}

template <typename T>
void Processor<T>::setLogScaleParameters(T coeff, T minVal, T maxVal, T addend, bool autoComputeMinMax) {
	logScaleCoeff_ = coeff;
	logScaleMin_ = minVal;
	logScaleMax_ = maxVal;
	logScaleAddend_ = addend;
	autoComputeLogScaleMinMax_ = autoComputeMinMax;
}

template <typename T>
void Processor<T>::generateWindow() {
	windowFunction_.resize(samplesPerSpectrum_);
	T factor = static_cast<T>(2.0 * PI_ / (samplesPerSpectrum_ - 1));

	for (size_t i = 0; i < samplesPerSpectrum_; ++i) {
		windowFunction_[i] = static_cast<T>(0.5) * (1 - std::cos(factor * i)); // Hanning window
	}
}

template <typename T>
void Processor<T>::computeDispersivePhase() {
	size_t spectrumSize = samplesPerSpectrum_;
	phaseComplex_.resize(spectrumSize);

	//normalization of dispersion coeffs to match the polynomial calculation of OCTproZ
	float denom = static_cast<float>((spectrumSize) - 1);
	std::vector<T> normCoeffs(4);
	normCoeffs[0] = dispersionCoefficients_[0];
	normCoeffs[1] = dispersionCoefficients_[1] / denom;
	normCoeffs[2] = dispersionCoefficients_[2] / (denom * denom);
	normCoeffs[3] = dispersionCoefficients_[3] / (denom * denom * denom);


	std::vector<T> phaseValues_debugging(spectrumSize);

	// Compute phase values using the polynomial coefficients
	for (size_t i = 0; i < spectrumSize; ++i) {
		T k = static_cast<T>(i);
		//T phaseValue = static_cast<T>(0);
		T kPower = static_cast<T>(1);

		T phaseValue = normCoeffs[0]
				+ i * (normCoeffs[1]
				+ i * (normCoeffs[2]
				+ i * normCoeffs[3]));

		// Compute the polynomial value
//		for (const T& coeff : normCoeffs) {
//			phaseValue += coeff * kPower;
//			kPower *= k;
//		}

phaseValues_debugging[i] = phaseValue;

		//T angle =  phaseValue;
		//phaseComplex_[i] = std::polar(static_cast<T>(1.0), angle * dispersionDirection_);
		phaseComplex_[i] = std::complex<T>(cos(phaseValue), sin(phaseValue) * dispersionDirection_);
	}
}

template <typename T>
void Processor<T>::generateCoefficientResamplingCurve() {
	size_t size = samplesPerSpectrum_;
	coefficientResamplingCurve_.resize(size);

	T coeff0 = resamplingCoefficients_[0];
	T coeff1 = resamplingCoefficients_[1]/((float)size-1.0);
	T coeff2 = resamplingCoefficients_[2]/((float)(size-1.0)*(float)(size-1.0));
	T coeff3 = resamplingCoefficients_[3]/((float)(size-1.0)*(float)(size-1.0)*(float)(size-1.0));

	T minIndex = static_cast<T>(0);
	T maxIndex = static_cast<T>(size - 3);

	for(size_t i = 0; i < size; ++i) {
		T x = static_cast<T>(i);
		T val = coeff0 + x * (coeff1 + x * (coeff2 + x * coeff3));
		coefficientResamplingCurve_[i] = clamp(val, minIndex, maxIndex);
	}
}

template <typename T>
void Processor<T>::generateResampleCurve() {
	size_t size = samplesPerSpectrum_;
	resamplePositions_.resize(size);

	if (options_.useCustomResamplingCurve && hasCustomResamplingCurve_) {
		// Use the custom resampling curve
		std::copy(customResamplingCurve_.begin(), customResamplingCurve_.end(), resamplePositions_.begin());
	} else {
		// Use the coefficient-based resampling curve
		std::copy(coefficientResamplingCurve_.begin(), coefficientResamplingCurve_.end(), resamplePositions_.begin());
	}
}


template <typename T>
void Processor<T>::convertInputData(const void* inputData,
                                    size_t totalSamples,
                                    int inputBitDepth,
                                    std::vector<std::complex<T>>& outputData) {
	outputData.resize(totalSamples);

	T scaleFactor = static_cast<T>(1.0);

	if (inputBitDepth <= 8) {
		const uint8_t* in = static_cast<const uint8_t*>(inputData);
		scaleFactor = static_cast<T>(1.0);// / static_cast<T>(255);
		for (size_t i = 0; i < totalSamples; ++i) {
			outputData[i] = std::complex<T>(static_cast<T>(in[i]) * scaleFactor, static_cast<T>(0));
		}
	} else if (inputBitDepth <= 16) {
		const uint16_t* in = static_cast<const uint16_t*>(inputData);
		scaleFactor = static_cast<T>(1.0);// / static_cast<T>(65535);
		for (size_t i = 0; i < totalSamples; ++i) {
			outputData[i] = std::complex<T>(static_cast<T>(in[i]) * scaleFactor, static_cast<T>(0));
		}
	} else {
		const uint32_t* in = static_cast<const uint32_t*>(inputData);
		scaleFactor = static_cast<T>(1.0);// / static_cast<T>(4294967295);
		for (size_t i = 0; i < totalSamples; ++i) {
			outputData[i] = std::complex<T>(static_cast<T>(in[i]) * scaleFactor, static_cast<T>(0));
		}
	}
}

template <typename T>
void Processor<T>::processRawData(const void* inputData,
                                  size_t totalSamples,
                                  int inputBitDepth,
                                  size_t spectraPerFrame,
                                  std::vector<std::vector<std::vector<T>>>& processedData) {
	// Step 1: Convert input data to std::complex<T>
	std::vector<std::complex<T>> complexData;
	convertInputData(inputData, totalSamples, inputBitDepth, complexData);

	// Organize data into frames and spectra
	size_t samplesPerSpectrum = samplesPerSpectrum_;
	size_t numFrames = totalSamples / (samplesPerSpectrum * spectraPerFrame);

	processedData.resize(numFrames);
	size_t index = 0;

	for (size_t frameIndex = 0; frameIndex < numFrames; ++frameIndex) {
		processedData[frameIndex].resize(spectraPerFrame);

		for (size_t spectrumIndex = 0; spectrumIndex < spectraPerFrame; ++spectrumIndex) {
			// Extract spectrum data
			std::vector<std::complex<T>> spectrum(complexData.begin() + index, complexData.begin() + index + samplesPerSpectrum);
			index += samplesPerSpectrum;

			// Apply processing steps
			if (options_.removeDC) {
				// Rolling average DC removal
				rollingAverageDCRemoval(spectrum);
			}

			if (options_.resample) {
				// Ensure resample curve is generated
				if (resamplePositions_.empty() || resamplingCurveOptionsChanged_) {
					generateResampleCurve();
					resamplingCurveOptionsChanged_ = false;
				}
				// K-linearization using cubic Hermite interpolation
				std::vector<std::complex<T>> resampledSpectrum;
				klinearizationCubic(spectrum, resamplePositions_, resampledSpectrum);
				spectrum = resampledSpectrum;
			}

			if (options_.compensateDispersion) {
				dispersionCompensation(spectrum);
			}

			if (options_.applyWindow) {
				applyWindow(spectrum);
			}

			std::vector<std::complex<T>> ifftOutput;
			if (options_.computeIFFT) {
				computeIFFT(spectrum, ifftOutput);
			} else {
				ifftOutput = spectrum;
			}

			std::vector<T> processedSpectrum;
			if (options_.logScale) {
				logScale(ifftOutput, processedSpectrum);
			} else {
				// Output magnitude
				processedSpectrum.resize(ifftOutput.size());
				for (size_t i = 0; i < ifftOutput.size(); ++i) {
					processedSpectrum[i] = std::abs(ifftOutput[i]);
				}
			}

			//truncate to half to remove mirror artifact
			size_t halfSize = processedSpectrum.size() / 2;
			std::vector<T> truncated(halfSize);
			for (size_t i = 0; i < halfSize; ++i) {
				truncated[i] = processedSpectrum[i];
			}

			//store processed and truncated data
			processedData[frameIndex][spectrumIndex] = std::move(truncated);
		}
	}
}

template <typename T>
void Processor<T>::rollingAverageDCRemoval(std::vector<std::complex<T>>& spectrum) {
	size_t numSamples = spectrum.size();
	size_t rollingWindowSize = rollingAverageWindowSize_;

	// Compute cumulative sum for efficient rolling average computation
	std::vector<T> cumulativeSum(numSamples + 1, static_cast<T>(0));
	for (size_t i = 0; i < numSamples; ++i) {
		cumulativeSum[i + 1] = cumulativeSum[i] + spectrum[i].real();
	}

	// Perform rolling average subtraction
	for (size_t i = 0; i < numSamples; ++i) {
		size_t startIdx = (i >= rollingWindowSize - 1) ? i - (rollingWindowSize - 1) : 0;
		size_t endIdx = std::min(i + rollingWindowSize, numSamples - 1);

		T sum = cumulativeSum[endIdx + 1] - cumulativeSum[startIdx];
		size_t windowSize = endIdx - startIdx + 1;
		T rollingAverage = sum / static_cast<T>(windowSize);

		// Subtract rolling average
		spectrum[i] -= std::complex<T>(rollingAverage, static_cast<T>(0));
	}
}

template <typename T>
void Processor<T>::klinearizationCubic(const std::vector<std::complex<T>>& inputSpectrum,
                                       const std::vector<T>& resampleCurve,
                                       std::vector<std::complex<T>>& outputSpectrum) {
	size_t width = resampleCurve.size();
	outputSpectrum.resize(width);

	size_t inputSize = inputSpectrum.size();

	for (size_t j = 0; j < width; ++j) {
		T nx = resampleCurve[j];
		int n1 = static_cast<int>(nx);
		int n0 = std::abs(n1 - 1);
		int n2 = n1 + 1;
		int n3 = n2 + 1;

		// Handle boundary conditions
		n0 = std::min(n0, static_cast<int>(inputSize) - 1);
		n1 = std::min(n1, static_cast<int>(inputSize) - 1);
		n2 = std::min(n2, static_cast<int>(inputSize) - 1);
		n3 = std::min(n3, static_cast<int>(inputSize) - 1);

		T y0 = inputSpectrum[n0].real();
		T y1 = inputSpectrum[n1].real();
		T y2 = inputSpectrum[n2].real();
		T y3 = inputSpectrum[n3].real();

		T interpolatedValue = cubicHermiteInterpolation(y0, y1, y2, y3, nx - n1);

		outputSpectrum[j] = std::complex<T>(interpolatedValue, static_cast<T>(0));
	}
}

template <typename T>
void Processor<T>::dispersionCompensation(std::vector<std::complex<T>>& data) {
	size_t dataSize = data.size();

	if (phaseComplex_.empty() || phaseComplex_.size() != dataSize) {
		// Recompute phase complex values if necessary
		computeDispersivePhase();
	}

	//full complex multiplication
//	for (size_t i = 0; i < dataSize; ++i) {
//		data[i] *= phaseComplex_[i];
//	}

	// Perform only real part of complex multiplication
	// this works because imaginary part of phase is always 0
	for (size_t i = 0; i < dataSize; ++i) {
		T inReal = data[i].real();
		T phaseReal = phaseComplex_[i].real();
		T phaseImag = phaseComplex_[i].imag();

		T outReal = inReal * phaseReal;
		T outImag = inReal * phaseImag;

		data[i] = std::complex<T>(outReal, outImag);
	}
}

template <typename T>
void Processor<T>::applyWindow(std::vector<std::complex<T>>& data) {
	for (size_t i = 0; i < data.size(); ++i) {
		data[i] *= windowFunction_[i];
	}
}

template <typename T>
void Processor<T>::computeIFFT(const std::vector<std::complex<T>>& input,
                               std::vector<std::complex<T>>& output) {
	// Copy input data to FFTW input array
	for (size_t i = 0; i < samplesPerSpectrum_; ++i) {
		fftIn_[i][0] = input[i].real();
		fftIn_[i][1] = input[i].imag();
	}

	// Execute the IFFT
	fftw_execute(fftPlan_);

	// Normalize and copy output data
	output.resize(samplesPerSpectrum_);
	T normFactor = static_cast<T>(1) / static_cast<T>(samplesPerSpectrum_);
	for (size_t i = 0; i < samplesPerSpectrum_; ++i) {
		output[i] = std::complex<T>(fftOut_[i][0], fftOut_[i][1]) * normFactor;
	}
}

template <typename T>
void Processor<T>::logScale(const std::vector<std::complex<T>>& input,
                            std::vector<T>& output) {
	size_t size = input.size();
	output.resize(size);

	T coeff = logScaleCoeff_;
	T minVal = logScaleMin_;
	T maxVal = logScaleMax_;
	T addend = logScaleAddend_;
	T outputAscanLength = static_cast<T>(size);

	// Compute magnitude squared and initial value array
	std::vector<T> valueArray(size);
	for (size_t i = 0; i < size; ++i) {
		T realComponent = input[i].real();
		T imaginaryComponent = input[i].imag();
		T magnitudeSquared = realComponent * realComponent + imaginaryComponent * imaginaryComponent;// + EPSILON_;
		T value = static_cast<T>(10.0) * std::log10(magnitudeSquared / outputAscanLength);
		valueArray[i] = value;
	}

	// Auto-compute min and max if enabled
	if (autoComputeLogScaleMinMax_) {
		minVal = *std::min_element(valueArray.begin(), valueArray.end());
		maxVal = *std::max_element(valueArray.begin(), valueArray.end());
	}

	T range = maxVal - minVal;// + EPSILON_; // Avoid division by zero

	for (size_t i = 0; i < size; ++i) {
		T normalizedValue = (valueArray[i] - minVal) / range;
		output[i] = coeff * (normalizedValue + addend);
	}
}

template <typename T>
T Processor<T>::cubicHermiteInterpolation(const T y0, const T y1, const T y2, const T y3, const T positionBetweenY1andY2) {
	const T a = -y0 + 3.0 * (y1 - y2) + y3;
	const T b = 2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3;
	const T c = -y0 + y2;

	const T pos = positionBetweenY1andY2;
	const T pos2 = pos * pos;

	return static_cast<T>(0.5) * pos * (a * pos2 + b * pos + c) + y1;
}

template <typename T>
const T& Processor<T>::clamp(const T& value, const T& low, const T& high) {
	return (value < low) ? low : (high < value) ? high : value;
}

} // namespace OCTSignalProcessing

#endif // PROCESSOR_TPP
