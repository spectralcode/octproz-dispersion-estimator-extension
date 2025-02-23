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

//todo: estimator engine implementieren
//[x] octprocessor hinzufügen
//[x] methode für autoloading von oct params erstellen
//[x] frame signal fangen und roi anwenden [x]
//[x]oct prozessierung starten und signal mit ergebnis emitten (achtung truncate noch hinzufügen [x])
//[x]metriccalculator klasse erstellen die dann ergebnis von oct prozessierung bekommt
//metriccalculator ergebnis wird per signal inkl verwendete werte für d2 und d3 weitergegeben, bools zeigen an was altered wurde
//ergebnisse plotten
//status label in gui updaten
//alle gui connects für parameter update setzen
//alle nicht benötigten funktionen löschen
//sobald kompletter durchgang ferttig parabelfit durchführen und plotten und d2 und d3 übertragen und anzeigen
//evtl. noch einen 1d ascan plot hinzufügen damit mann sieht wie man threshold wert setzen sollte
//wichtig: load resampling curve from file in cpu processor hinzufügen!
//extension auto loading implementieren
//kineo stunden eintragen: 19.01.2025 13:00 bis 19:00


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
	void dispersionEstimationReady(double* d0, double* d1, double* d2, double* d3);

public slots:
	void startDispersionEstimation(void* frameBuffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void setParams(DispersionEstimatorParameters params);
};

#endif //DISPERSIONESTIMATIONENGINE
