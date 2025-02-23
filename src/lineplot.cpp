#include "lineplot.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QMenu>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

// Constructor: create two graphs (graph(0) for d2 and graph(1) for d3) and set default settings.
LinePlot::LinePlot(QWidget *parent)
	: QCustomPlot(parent),
	  m_dataPointCounter(0),
	  m_maxDataPoints(10000)	// You may adjust this value as needed.
{
	//Add two graphs: one for the d2 metric and one for the d3 metric.
	this->addGraph();
	this->addGraph();

	//Set default colors.
	this->graph(0)->setPen(QPen(QColor(55, 100, 250)));	// Default color for d2.
	this->graph(1)->setPen(QPen(QColor(250, 50, 50)));		// Default color for d3.

	//Set default curve names.
	this->graph(0)->setName("a");
	this->graph(1)->setName("b");

	//Set axis labels.
	this->xAxis->setLabel("x");
	this->yAxis->setLabel("y");

	//Config legend
	this->legend->setVisible(true);
	this->legend->setBrush(QBrush(QColor(255,255,255,100)));
	this->legend->setBorderPen(Qt::NoPen);
	this->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);
	QFont legendFont = font();
	legendFont.setPointSize(8);
	this->legend->setFont(legendFont);
	this->legend->setRowSpacing(-3);
	this->legend->setMinimumSize(QSize(0, 0));
	this->legend->setMaximumSize(QSize(100000, 100000));

	// Enable interactions (zooming, dragging, etc.).
	setupInteractions();
}

LinePlot::~LinePlot()
{
}

void LinePlot::LinePlot::setXAxisLabel(const QString &label)
{
	this->xAxis->setLabel(label);
	replot();
}

void LinePlot::LinePlot::setYAxisLabel(const QString &label)
{
	this->yAxis->setLabel(label);
	replot();
}

void LinePlot::setFirstCurveName(const QString &name)
{
	this->graph(0)->setName(name);
	replot();
}

void LinePlot::setSecondCurveName(const QString &name)
{
	this->graph(1)->setName(name);
	replot();
}

void LinePlot::setFirstCurveColor(const QColor &color)
{
	QPen pen(color);
	pen.setWidth(1);
	this->graph(0)->setPen(pen);
	replot();
}

void LinePlot::setSecondCurveColor(const QColor &color)
{
	QPen pen(color);
	pen.setWidth(1);
	this->graph(1)->setPen(pen);
	replot();
}

void LinePlot::updateFirstCurve(double x, double y)
{
	// Append the new data point.
	m_d2X.append(x);
	m_d2Y.append(y);
	m_dataPointCounter++;

	// Update graph(0) with the new data.
	this->graph(0)->setData(m_d2X, m_d2Y, true);

	// Optional: Remove old data if exceeding maximum allowed points.
	if (m_d2X.size() > m_maxDataPoints)
	{
		m_d2X.remove(0);
		m_d2Y.remove(0);
	}

	// Rescale axes to include new data and update the plot.
	this->rescaleAxes();
	replot();
}

void LinePlot::updateSecondCurve(double x, double y)
{
	m_d3X.append(x);
	m_d3Y.append(y);
	m_dataPointCounter++;

	this->graph(1)->setData(m_d3X, m_d3Y, true);

	if (m_d3X.size() > m_maxDataPoints)
	{
		m_d3X.remove(0);
		m_d3Y.remove(0);
	}

	this->rescaleAxes();
	replot();
}

void LinePlot::setFirstCurve(QVector<float> curveData)
{
	m_d2X.clear();
	m_d2Y.clear();

	for (int i = 0; i < curveData.size(); ++i)
	{
		m_d2X.append(i);	// X-axis uses indices.
		m_d2Y.append(curveData[i]); // Y-values come from the provided vector.
	}

	this->graph(0)->setData(m_d2X, m_d2Y, true);

	this->rescaleAxes();
	replot();
}

void LinePlot::setSecondCurve(QVector<float> curveData)
{
	m_d3X.clear();
	m_d3Y.clear();

	for (int i = 0; i < curveData.size(); ++i)
	{
		m_d3X.append(i);	// X-axis uses indices.
		m_d3Y.append(curveData[i]); // Y-values come from the provided vector.
	}

	this->graph(1)->setData(m_d3X, m_d3Y, true);

	this->rescaleAxes();
	replot();
}

void LinePlot::clearPlot()
{
	m_d2X.clear();
	m_d2Y.clear();
	m_d3X.clear();
	m_d3Y.clear();
	m_dataPointCounter = 0;

	if (this->graph(0))
		this->graph(0)->data()->clear();
	if (this->graph(1))
		this->graph(1)->data()->clear();

	replot();
}

bool LinePlot::savePlotDataToFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Truncate))
		return false;

	QTextStream stream(&file);

	stream << "Dispersion Parameter;D2 Metric;D3 Metric\n";

	int count = qMax(m_d2X.size(), m_d3X.size());
	for (int i = 0; i < count; i++)
	{
		double param = (i < m_d2X.size()) ? m_d2X[i] : 0;
		QString d2Val = (i < m_d2Y.size()) ? QString::number(m_d2Y[i]) : "";
		QString d3Val = (i < m_d3Y.size()) ? QString::number(m_d3Y[i]) : "";
		stream << QString::number(param) << ";" << d2Val << ";" << d3Val << "\n";
	}

	file.close();
	return true;
}

void LinePlot::mouseDoubleClickEvent(QMouseEvent *event)
{
	this->rescaleAxes();
	replot();
	QCustomPlot::mouseDoubleClickEvent(event);
}

void LinePlot::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	QAction *saveAction = menu.addAction("Save Plot Data...");
	connect(saveAction, &QAction::triggered, this, [this]() {
		QString fileName = QFileDialog::getSaveFileName(this, "Save Plot Data", QString(), "CSV Files (*.csv)");
		if (!fileName.isEmpty())
		{
			if (!savePlotDataToFile(fileName))
				QMessageBox::warning(this, "Save Error", "Could not save plot data to file.");
		}
	});
	menu.exec(event->globalPos());
}

void LinePlot::setupInteractions()
{
	this->setInteraction(QCP::iRangeDrag, true);
	this->setInteraction(QCP::iRangeZoom, true);
	this->setInteraction(QCP::iSelectAxes, true);
}
