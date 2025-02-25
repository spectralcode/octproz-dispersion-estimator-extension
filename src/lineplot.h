#ifndef LINEPLOT_H
#define LINEPLOT_H

#include "qcustomplot.h"

// LinePlot is a custom QCustomPlot widget tailored for dispersion-metric plotting.
// It displays two curves (for d2 and d3) and supports interactive features such as
// zooming, dragging, double-click reset, and saving plot data to a CSV file.
class LinePlot : public QCustomPlot
{
	Q_OBJECT
public:
	// Constructor and destructor.
	explicit LinePlot(QWidget *parent = nullptr);
	~LinePlot();

	void setXAxisLabel(const QString &label);
	void setYAxisLabel(const QString &label);

	void setFirstCurveName(const QString &name);
	void setSecondCurveName(const QString &name);

	void setFirstCurveColor(const QColor &color);
	void setSecondCurveColor(const QColor &color);

	// Update the plot with a new data point for the d2 curve.
	// 'x' represents the dispersion parameter value (d2) and 'y' its metric value.
	void updateFirstCurve(double x, double y);
	// Update the plot with a new data point for the d3 curve.
	// 'x' represents the dispersion parameter value (d3) and 'y' its metric value.
	void updateSecondCurve(double x, double y);

	void setFirstCurve(QVector<float> curveData);
	void setSecondCurve(QVector<float> curveData);

	// Clear all plot data.
	void clearPlot();

	// Save the current plot data to a CSV file.
	bool savePlotDataToFile(const QString &fileName);

protected:
	// Override to allow double-click events to reset the view.
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	// Override to display a context menu (with a save option).
	void contextMenuEvent(QContextMenuEvent *event) override;

private:
	// Data storage for the d2 curve.
	QVector<double> m_d2X;
	QVector<double> m_d2Y;
	// Data storage for the d3 curve.
	QVector<double> m_d3X;
	QVector<double> m_d3Y;

	// Counter for the total number of data points added.
	int m_dataPointCounter;
	// Maximum allowed data points (optional auto-cleaning/scrolling).
	int m_maxDataPoints;

	// Configure interactive features such as zooming and dragging.
	void setupInteractions();
};

#endif // LINEPLOT_H
