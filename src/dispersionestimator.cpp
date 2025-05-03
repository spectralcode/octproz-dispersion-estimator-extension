#include "dispersionestimator.h"


DispersionEstimator::DispersionEstimator()
	: Extension(),
	form(new DispersionEstimatorForm()),
	estimationEngine(new DispersionEstimationEngine()),
	frameNr(0),
	active(false),
	isCalculating(false),
	singleFetch(false),
	copyBufferId(-1),
	bytesPerFrameRaw(0),
	bytesPerFrameProcessed(0),
	lostBuffersProcessed(0)
{
	qRegisterMetaType<DispersionEstimatorParameters>("DispersionEstimatorParameters");
	qRegisterMetaType<QVector<float>>("QVector<float>");

	this->setType(EXTENSION);
	this->displayStyle = SEPARATE_WINDOW;
	this->name = "Dispersion Estimator";
	this->toolTip = "Estimates values for numerical dispersion compensation";

	this->setupGuiConnections();
	this->setupDispersionEstimatorEngine();
	this->initializeFrameBuffers();
}

DispersionEstimator::~DispersionEstimator() {
	estimatorEngineThread.quit();
	estimatorEngineThread.wait();

	delete this->form;

	this->releaseFrameBuffers(this->frameBuffersProcessed);
	this->releaseFrameBuffers(this->frameBuffersRaw);
}

QWidget* DispersionEstimator::getWidget() {
	return this->form;
}

void DispersionEstimator::activateExtension() {
	//this method is called by OCTproZ as soon as user activates the extension. If the extension controls hardware components, they can be prepared, activated, initialized or started here.
	this->active = true;
}

void DispersionEstimator::deactivateExtension() {
	//this method is called by OCTproZ as soon as user deactivates the extension. If the extension controls hardware components, they can be deactivated, resetted or stopped here.
	this->active = false;
}

void DispersionEstimator::settingsLoaded(QVariantMap settings) {
	//this method is called by OCTproZ and provides a QVariantMap with stored settings/parameters.
	this->form->setSettings(settings); //update gui with stored settings
}

void DispersionEstimator::setupGuiConnections() {
	connect(this->form, &DispersionEstimatorForm::info, this, &DispersionEstimator::info);
	connect(this->form, &DispersionEstimatorForm::error, this, &DispersionEstimator::error);
	connect(this, &DispersionEstimator::maxBuffers, this->form, &DispersionEstimatorForm::setMaximumBufferNr);
	connect(this, &DispersionEstimator::maxFrames, this->form, &DispersionEstimatorForm::setMaximumFrameNr);

	//store settings
	connect(this->form, &DispersionEstimatorForm::paramsChanged, this, &DispersionEstimator::storeParameters);

	//data acquisition settings inputs from the GUI
	connect(this->form, &DispersionEstimatorForm::frameNrChanged, this, [this](int frameNr) {
		this->frameNr = frameNr;
	});
	connect(this->form, &DispersionEstimatorForm::bufferNrChanged, this, [this](int bufferNr) {
		this->bufferNr = bufferNr;
	});

	connect(this->form, &DispersionEstimatorForm::singleFetchRequested, this, [this]() {
		this->singleFetch = true;
		emit statusUpdate(tr("Waiting for data..."));
	});
}

void DispersionEstimator::setupDispersionEstimatorEngine() {
	this->estimationEngine = new DispersionEstimationEngine();
	this->estimationEngine->moveToThread(&estimatorEngineThread);
	connect(&estimatorEngineThread, &QThread::finished, this->estimationEngine, &QObject::deleteLater);
	connect(this, &DispersionEstimator::newFrame, this->estimationEngine, &DispersionEstimationEngine::startDispersionEstimation);
	connect(this->form, &DispersionEstimatorForm::paramsChanged, this->estimationEngine, &DispersionEstimationEngine::setParams);
	connect(this->estimationEngine, &DispersionEstimationEngine::info, this, &DispersionEstimator::info);
	connect(this->estimationEngine, &DispersionEstimationEngine::error, this, &DispersionEstimator::error);
	connect(this->estimationEngine, &DispersionEstimationEngine::estimationProcessStarted, this->form, &DispersionEstimatorForm::clearPlot);
	connect(this->estimationEngine, &DispersionEstimationEngine::metricValueCalculatedD2, this->form, &DispersionEstimatorForm::addDataToD2Plot);
	connect(this->estimationEngine, &DispersionEstimationEngine::metricValueCalculatedD3, this->form, &DispersionEstimatorForm::addDataToD3Plot);
	connect(this->estimationEngine, &DispersionEstimationEngine::bestD2Estimated, this->form, &DispersionEstimatorForm::displayBestD2);
	connect(this->estimationEngine, &DispersionEstimationEngine::bestD3Estimated, this->form, &DispersionEstimatorForm::displayBestD3);
	connect(this->estimationEngine, &DispersionEstimationEngine::d1Calculated, this->form, &DispersionEstimatorForm::displayDerivedD1);
	connect(this->estimationEngine, &DispersionEstimationEngine::dispersionEstimationReady, this, &DispersionEstimator::setDispCompCoeffsRequest);

	connect(this->estimationEngine, &DispersionEstimationEngine::ascanWithoutDispersionCalculated, this->form, &DispersionEstimatorForm::addAscanOneToPlot);
	connect(this->estimationEngine, &DispersionEstimationEngine::ascanWithBestDispersionCalculated, this->form, &DispersionEstimatorForm::addAscanTwoToPlot);
	connect(this->estimationEngine, &DispersionEstimationEngine::statusUpdate, this->form, &DispersionEstimatorForm::updateStatus);
	connect(this, &DispersionEstimator::statusUpdate, this->form, &DispersionEstimatorForm::updateStatus);

	estimatorEngineThread.start();


//todo parabolic peak fit, aber nicht hier sondern in estimator klasse
}

void DispersionEstimator::initializeFrameBuffers() {
	this->frameBuffersRaw.resize(NUMBER_OF_BUFFERS);
	this->frameBuffersProcessed.resize(NUMBER_OF_BUFFERS);
	for(int i = 0; i < NUMBER_OF_BUFFERS; i++){
		this->frameBuffersRaw[i] = nullptr;
		this->frameBuffersProcessed[i] = nullptr;
	}
}

void DispersionEstimator::releaseFrameBuffers(QVector<void *> buffers) {
	for (int i = 0; i < buffers.size(); i++) {
		if (buffers[i] != nullptr) {
			free(buffers[i]);
			buffers[i] = nullptr;
		}
	}
}

void DispersionEstimator::storeParameters() {
	//update settingsMap, so parameters can be reloaded into gui at next start of application
	this->form->getSettings(&this->settingsMap);
	emit storeSettings(this->name, this->settingsMap);
}

void DispersionEstimator::rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	if(this->active){
		if((!this->isCalculating && this->singleFetch && this->rawGrabbingAllowed)){
			//check if current buffer is selected. If it is not selected discard it and do nothing (just return).
			if(this->bufferNr>static_cast<int>(buffersPerVolume-1)){
				this->bufferNr = static_cast<int>(buffersPerVolume-1);
			}
			if(!(this->bufferNr == -1 || this->bufferNr == static_cast<int>(currentBufferNr))){
				return;
			}

			this->isCalculating = true;

			//calculate size of single frame
			size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(bitDepth)/8.0));
			size_t bytesPerFrame = samplesPerLine*linesPerFrame*bytesPerSample;

			//check if number of frames per buffer has changed and emit maxFrames to update gui
			if(this->framesPerBuffer != framesPerBuffer){
				emit maxFrames(framesPerBuffer-1);
				this->framesPerBuffer = framesPerBuffer;
			}
			//check if number of buffers per volume has changed and emit maxBuffers to update gui
			if(this->buffersPerVolume != buffersPerVolume){
				emit maxBuffers(buffersPerVolume-1);
				this->buffersPerVolume = buffersPerVolume;
			}

			//check if buffer size changed and allocate buffer memory
			if(this->frameBuffersRaw[0] == nullptr || this->bytesPerFrameProcessed != bytesPerFrame){
				if(bitDepth == 0 || samplesPerLine == 0 || linesPerFrame == 0 || framesPerBuffer == 0){
					emit error(this->name + ":  " + tr("Invalid data dimensions!"));
					return;
				}
				//(re)create copy buffers
				if(this->frameBuffersRaw[0] != nullptr){
					this->releaseFrameBuffers(this->frameBuffersRaw);
				}
				for (int i = 0; i < this->frameBuffersRaw.size(); i++) {
					this->frameBuffersRaw[i] = static_cast<void*>(malloc(bytesPerFrame));
				}
				this->bytesPerFrameProcessed = bytesPerFrame;
			}

			//copy single frame of received data and emit it for further processing
			this->copyBufferId = (this->copyBufferId+1)%NUMBER_OF_BUFFERS;
			char* frameInBuffer = static_cast<char*>(buffer);
			if(this->frameNr>static_cast<int>(framesPerBuffer-1)){this->frameNr = static_cast<int>(framesPerBuffer-1);}
			memcpy(this->frameBuffersRaw[this->copyBufferId], &(frameInBuffer[bytesPerFrame*this->frameNr]), bytesPerFrame);
			emit newFrame(this->frameBuffersRaw[this->copyBufferId], bitDepth, samplesPerLine, linesPerFrame);

			this->isCalculating = false;
			this->singleFetch = false;
		}
	}
}

void DispersionEstimator::processedDataReceived(void* buffer, unsigned bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	Q_UNUSED(buffer)
	Q_UNUSED(bitDepth)
	Q_UNUSED(samplesPerLine)
	Q_UNUSED(linesPerFrame)
	Q_UNUSED(framesPerBuffer)
	Q_UNUSED(buffersPerVolume)
	Q_UNUSED(currentBufferNr)
}

void DispersionEstimator::receiveCommand(const QString &command, const QVariantMap &params) {
	Q_UNUSED(params)
	if(command == "startSingleFetch"){
		emit this->form->singleFetchRequested();
	}
}

