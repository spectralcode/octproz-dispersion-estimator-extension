#ifndef DISPERSIONESTIMATOREXTENSION_H
#define DISPERSIONESTIMATOREXTENSION_H


#include <QCoreApplication>
#include <QThread>
#include "octproz_devkit.h"
#include "dispersionestimatorform.h"
#include "dispersionestimationengine.h"

#define NUMBER_OF_BUFFERS 2


class DispersionEstimator : public Extension
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID Extension_iid)
	Q_INTERFACES(Extension)
	QThread estimatorEngineThread;

public:
	DispersionEstimator();
	~DispersionEstimator();

	virtual QWidget* getWidget() override;
	virtual void activateExtension() override;
	virtual void deactivateExtension() override;
	virtual void settingsLoaded(QVariantMap settings) override;

private:
	DispersionEstimatorForm* form;
	DispersionEstimationEngine* estimationEngine;

	int frameNr;
	int bufferNr;
	bool active;
	bool isCalculating;
	bool singleFetch;


	QVector<void*> frameBuffersRaw;
	QVector<void*> frameBuffersProcessed;
	int copyBufferId;
	size_t bytesPerFrameRaw;
	size_t bytesPerFrameProcessed;
	int lostBuffersRaw;
	int lostBuffersProcessed;
	unsigned int framesPerBuffer;
	unsigned int buffersPerVolume;

	void setupGuiConnections();
	void setupDispersionEstimatorEngine();
	void initializeFrameBuffers();
	void releaseFrameBuffers(QVector<void*> buffers);

public slots:
	void storeParameters();
	virtual void rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;
	virtual void processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;
	void receiveCommand(const QString &command, const QVariantMap &params) override;

signals:
	void newFrame(void* frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void maxFrames(int max);
	void maxBuffers(int max);
	void statusUpdate(const QString &status);
};

#endif //DISPERSIONESTIMATOREXTENSION_H
