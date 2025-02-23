#ifndef DISPERSIONESTIMATORFORM_H
#define DISPERSIONESTIMATORFORM_H

#include <QWidget>
#include <QRect>
#include "dispersionestimatorparameters.h"
#include "lineplot.h"

namespace Ui {
class DispersionEstimatorForm;
}

class DispersionEstimatorForm : public QWidget
{
	Q_OBJECT

public:
	explicit DispersionEstimatorForm(QWidget *parent = 0);
	~DispersionEstimatorForm();

	void setSettings(QVariantMap settings);
	void getSettings(QVariantMap* settings);

	LinePlot* getLinePlot(){return this->linePlot;}

	Ui::DispersionEstimatorForm* ui;

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;


public slots:
	void setMaximumFrameNr(int maximum);
	void setMaximumBufferNr(int maximum);
	void addDataToD2Plot(qreal d2, float metricValue);
	void addDataToD3Plot(qreal d3, float metricValue);
	void clearPlot();
	void displayBestD2(double d2);
	void displayBestD3(double d3);

	void addAscanOneToPlot(QVector<float> ascan);
	void addAscanTwoToPlot(QVector<float> ascan);
	void updateStatus(const QString &status);

private slots:
	void toggleUIVisibility();

private:
	LinePlot* linePlot;
	LinePlot* ascanPlot;
	DispersionEstimatorParameters parameters;
	bool firstRun;

	void connectUiControls();
	void setupPlot();

signals:
	void paramsChanged(DispersionEstimatorParameters);
	void frameNrChanged(int);
	void bufferNrChanged(int);
	void featureChanged(int);
	void bufferSourceChanged(BUFFER_SOURCE);
	void roiChanged(QRect);
	void singleFetchRequested();
	void autoFetchRequested(bool isRequested);
	void nthBufferChanged(int nthBuffer);
	void fitModeLogarithmEnabled(bool enabled);
	void info(QString);
	void error(QString);
};

#endif //DISPERSIONESTIMATORFORM_H
