#ifndef DISPERSIONESTIMATIONENGINE_H
#define DISPERSIONESTIMATIONENGINE_H

#include <QObject>
#include <QVector>
#include <QRect>
#include <QApplication>
#include <QtMath>
#include <QPair>
#include "dispersionestimatorparameters.h"
#include "octprocessor/processorcontroller.h"
#include "ascanmetriccalculator.h"


class DispersionEstimationEngine : public QObject
{
	Q_OBJECT
public:
	explicit DispersionEstimationEngine(QObject *parent = nullptr);


private:
	bool isPeakFitting;
	DispersionEstimatorParameters params;
	ProcessorController *processorController;
	AscanMetricCalculator calculator;
	float bestMetricValueD2;
	float bestMetricValueD3;
	double bestD2;
	double bestD3;
	double calculatedD1;

	void processDispersionMetric(QByteArray &rawData, qreal &d2, qreal &d3, qreal stepSize, bool isD2);
	QVector<float> processFirstLineOnly(QByteArray &rawData, qreal d2, qreal d3);

signals:
	void metricValueCalculatedD2(float d2, float metricValue);
	void metricValueCalculatedD3(float d3, float metricValue);
	void estimationProcessStarted();
	void info(QString);
	void error(QString);
	void statusUpdate(const QString &status);

	void ascanWithoutDispersionCalculated(QVector<float> ascan);
	void ascanWithBestDispersionCalculated(QVector<float> ascan);

	void bestD2Estimated(double estimatedD2);
	void bestD3Estimated(double estimateD3);
	void d1Calculated(double d1);
	void dispersionEstimationReady(double* d0, double* d1, double* d2, double* d3);

public slots:
	void startDispersionEstimation(void* frameBuffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void setParams(DispersionEstimatorParameters params);
};

#endif //DISPERSIONESTIMATIONENGINE
