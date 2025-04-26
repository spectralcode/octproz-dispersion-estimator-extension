#include "dispersionestimatorform.h"
#include "ui_dispersionestimatorform.h"
#include <QtGlobal>

DispersionEstimatorForm::DispersionEstimatorForm(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DispersionEstimatorForm) {
	ui->setupUi(this);

	this->firstRun = true;

	// Fill the image metric comboBox with the available metrics
	this->ui->comboBox_imageMetric->clear();
	this->ui->comboBox_imageMetric->addItem(tr("Sum Above Threshold"), static_cast<int>(SUM_ABOVE_THRESHOLD));
	this->ui->comboBox_imageMetric->addItem(tr("Samples Above Threshold"), static_cast<int>(SAMPLES_ABOVE_THRESHOLD));
	this->ui->comboBox_imageMetric->addItem(tr("Peak Value"), static_cast<int>(PEAK_VALUE));
	this->ui->comboBox_imageMetric->addItem(tr("Mean Sobel"), static_cast<int>(MEAN_SOBEL));

	this->connectUiControls();
	this->setupPlot();
	this->installEventFilter(this);
}

DispersionEstimatorForm::~DispersionEstimatorForm() {
	delete ui;
}

void DispersionEstimatorForm::setSettings(QVariantMap settings) {
	//update parameters struct
	if (!settings.isEmpty()) {
		this->parameters.bufferNr = settings.value(DISPERSION_ESTIMATOR_BUFFER_NR).toInt();
		this->parameters.frameNr = settings.value(DISPERSION_ESTIMATOR_FRAME_NR).toInt();
		this->parameters.numberOfCenterAscans = settings.value(DISPERSION_ESTIMATOR_NUMBER_OF_CENTER_ASCANS).toInt();
		this->parameters.useLinearAscans = settings.value(DISPERSION_ESTIMATOR_USE_LINEAR_ASCANS).toBool();
		this->parameters.numberOfAscanSamplesToIgnore = settings.value(DISPERSION_ESTIMATOR_NUMBER_OF_ASCAN_SAMPLES_TO_IGNORE).toInt();
		this->parameters.sharpnessMetric = static_cast<ASCAN_SHARPNESS_METRIC>(settings.value(DISPERSION_ESTIMATOR_SHARPNESS_METRIC).toInt());
		this->parameters.metricThreshold = settings.value(DISPERSION_ESTIMATOR_METRIC_THRESHOLD).toReal();
		this->parameters.d2start = settings.value(DISPERSION_ESTIMATOR_D2_START).toReal();
		this->parameters.d2end = settings.value(DISPERSION_ESTIMATOR_D2_END).toReal();
		this->parameters.d3start = settings.value(DISPERSION_ESTIMATOR_D3_START).toReal();
		this->parameters.d3end = settings.value(DISPERSION_ESTIMATOR_D3_END).toReal();
		this->parameters.numberOfDispersionSamples = settings.value(DISPERSION_ESTIMATOR_NUMBER_OF_DISPERSION_SAMPLES).toInt();
		this->parameters.windowState = settings.value(DISPERSION_ESTIMATOR_WINDOW_STATE).toByteArray();
		this->parameters.guiVisible = settings.value(DISPERSION_ESTIMATOR_GUI_TOGGLE, true).toBool();

		// Update the UI elements accordingly.
		this->ui->spinBox_buffer->setValue(parameters.bufferNr);
		this->ui->spinBox_frame->setValue(parameters.frameNr);
		this->ui->spinBox_numberOfAscans->setValue(parameters.numberOfCenterAscans);
		this->ui->checkBox_useLinear->setChecked(parameters.useLinearAscans);
		this->ui->spinBox_samplesToIgnore->setValue(parameters.numberOfAscanSamplesToIgnore);
		this->ui->comboBox_imageMetric->setCurrentIndex(static_cast<int>(parameters.sharpnessMetric));
		this->ui->doubleSpinBox_metricThreshold->setValue(parameters.metricThreshold);
		this->ui->doubleSpinBox_d2Start->setValue(parameters.d2start);
		this->ui->doubleSpinBox_d2End->setValue(parameters.d2end);
		this->ui->doubleSpinBox_d3Start->setValue(parameters.d3start);
		this->ui->doubleSpinBox_d3End->setValue(parameters.d3end);
		this->ui->spinBox_numberOfDispersionSamples->setValue(parameters.numberOfDispersionSamples);

		// Set GUI elements visibility based on the stored toggle status
		ui->widget_settings_area->setVisible(this->parameters.guiVisible);
		if (!this->parameters.guiVisible) {
			this->adjustSize();
			this->setFixedSize(this->size());
		} else {
			this->setMinimumSize(0, 0);
			this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
			this->adjustSize();
		}
		// Restore the saved geometry after layout adjustments
		this->restoreGeometry(this->parameters.windowState);
	}
}

void DispersionEstimatorForm::getSettings(QVariantMap* settings) {
	if (!settings) {
		return;
	}

	settings->insert(DISPERSION_ESTIMATOR_BUFFER_NR, this->parameters.bufferNr);
	settings->insert(DISPERSION_ESTIMATOR_FRAME_NR, this->parameters.frameNr);
	settings->insert(DISPERSION_ESTIMATOR_NUMBER_OF_CENTER_ASCANS, this->parameters.numberOfCenterAscans);
	settings->insert(DISPERSION_ESTIMATOR_USE_LINEAR_ASCANS, this->parameters.useLinearAscans);
	settings->insert(DISPERSION_ESTIMATOR_NUMBER_OF_ASCAN_SAMPLES_TO_IGNORE, this->parameters.numberOfAscanSamplesToIgnore);
	settings->insert(DISPERSION_ESTIMATOR_SHARPNESS_METRIC, static_cast<int>(this->parameters.sharpnessMetric));
	settings->insert(DISPERSION_ESTIMATOR_METRIC_THRESHOLD, this->parameters.metricThreshold);
	settings->insert(DISPERSION_ESTIMATOR_D2_START, this->parameters.d2start);
	settings->insert(DISPERSION_ESTIMATOR_D2_END, this->parameters.d2end);
	settings->insert(DISPERSION_ESTIMATOR_D3_START, this->parameters.d3start);
	settings->insert(DISPERSION_ESTIMATOR_D3_END, this->parameters.d3end);
	settings->insert(DISPERSION_ESTIMATOR_NUMBER_OF_DISPERSION_SAMPLES, this->parameters.numberOfDispersionSamples);
	settings->insert(DISPERSION_ESTIMATOR_WINDOW_STATE, this->parameters.windowState);
	settings->insert(DISPERSION_ESTIMATOR_GUI_TOGGLE, this->parameters.guiVisible);
}

bool DispersionEstimatorForm::eventFilter(QObject *watched, QEvent *event) {
	if (watched == this) {
		if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
			if(this->isVisible()) {
				this->parameters.windowState = this->saveGeometry(); //todo: on windows this stores the window position one title bar height below its current position. check how it behaves on a jetson nano and maybe save window->pos and size instead of geometry
				emit paramsChanged(this->parameters);
			}
		}
	}
	return QWidget::eventFilter(watched, event);
}

void DispersionEstimatorForm::showEvent(QShowEvent *event) {
	QWidget::showEvent(event);

#ifdef Q_OS_WIN
	//on startup the qwidget window is positioned one title bar height below its previous position (probably because storeGeometry seems to save the window geometry without the title bar)
	//this moves the window back to its original position
	//todo: test if this is also necessary for linux jetson nano
	static bool firstShow = true;
	if (firstShow) {
		int titleBarHeight = geometry().y()-frameGeometry().y(); //info: this calculation here is used because QStyle::PM_DockWidgetTitleMargin gives a slightly too small value on windows 10
		//move(this->frameGeometry().topLeft() - QPoint(0, titleBarHeight));
		//this manual position adjustment is no longer necessary because the parent is now set to the main window OCTproZ and the form is correctly positioned on startup.
		//todo: verify behavior on Jetson Nano and remove this if not needed.
		firstShow = false;
	}
#endif
}

void DispersionEstimatorForm::setMaximumFrameNr(int maximum) {
	this->ui->horizontalSlider_frame->setMaximum(maximum);
	this->ui->spinBox_frame->setMaximum(maximum);
}

void DispersionEstimatorForm::setMaximumBufferNr(int maximum) {
	this->ui->spinBox_buffer->setMaximum(maximum);
}

void DispersionEstimatorForm::addDataToD2Plot(qreal d2, float metricValue) {
	this->linePlot->updateFirstCurve(d2, static_cast<double>(metricValue));
}

void DispersionEstimatorForm::addDataToD3Plot(qreal d3, float metricValue) {
	this->linePlot->updateSecondCurve(d3, static_cast<double>(metricValue));
}

void DispersionEstimatorForm::clearPlot() {
	this->linePlot->clearPlot();
}

void DispersionEstimatorForm::displayBestD2(double d2) {
	this->ui->label_resultD2->setText(QString::number(d2));
}

void DispersionEstimatorForm::displayBestD3(double d3) {
	this->ui->label_resultD3->setText(QString::number(d3));
}

void DispersionEstimatorForm::addAscanOneToPlot(QVector<float> ascan) {
	this->ui->widget_linePlot_ascans->setFirstCurve(ascan);
}

void DispersionEstimatorForm::addAscanTwoToPlot(QVector<float> ascan) {
	this->ui->widget_linePlot_ascans->setSecondCurve(ascan);
}

void DispersionEstimatorForm::updateStatus(const QString &status) {
	this->ui->label_status->setText(status);
}

void DispersionEstimatorForm::toggleUIVisibility() {
	// Toggle visibility state
	this->parameters.guiVisible = !this->parameters.guiVisible;

	// Hide or show the settings area and status label.
	ui->widget_settings_area->setVisible(this->parameters.guiVisible);

	if (!this->parameters.guiVisible) {
		// Recalculate the layout size and force the widget to shrink to its minimum size.
		this->adjustSize();
		// Fix the window size to the new minimum size.
		this->setFixedSize(this->size());
	} else {
		// Remove the fixed size constraint so the window can expand again.
		this->setMinimumSize(0, 0);
		this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
		this->adjustSize();
	}
	emit paramsChanged(this->parameters);
}

void DispersionEstimatorForm::connectUiControls() {
	this->ui->spinBox_buffer->setMaximum(2);
	this->ui->spinBox_buffer->setMinimum(-1);
	this->ui->spinBox_buffer->setSpecialValueText(tr("All"));
	connect(this->ui->spinBox_buffer, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int bufferNr) {
		this->parameters.bufferNr = bufferNr;
		emit bufferNrChanged(bufferNr);
		emit paramsChanged(this->parameters);
	});

	connect(ui->checkBox_useLinear, &QCheckBox::toggled,
		this, [this](bool checked) {
			this->parameters.useLinearAscans = checked;
			emit paramsChanged(parameters);
		});

	//frame slider and spinBox
	connect(this->ui->horizontalSlider_frame, &QSlider::valueChanged, this->ui->spinBox_frame, &QSpinBox::setValue);
	connect(this->ui->spinBox_frame, QOverload<int>::of(&QSpinBox::valueChanged), this->ui->horizontalSlider_frame, &QSlider::setValue);
	connect(this->ui->horizontalSlider_frame, &QSlider::valueChanged, this, [this](int frameNr) {
		this->parameters.frameNr = frameNr;
		emit frameNrChanged(frameNr);
		emit paramsChanged(this->parameters);
	});
	this->setMaximumFrameNr(512);

	// Number of center A-scans
	connect(ui->spinBox_numberOfAscans, QOverload<int>::of(&QSpinBox::valueChanged),
		this, [this](int value) {
			this->parameters.numberOfCenterAscans = value;
			emit paramsChanged(this->parameters);
		});

	// Samples to ignore
	connect(ui->spinBox_samplesToIgnore, QOverload<int>::of(&QSpinBox::valueChanged),
		this, [this](int value) {
			this->parameters.numberOfAscanSamplesToIgnore = value;
			emit paramsChanged(this->parameters);
		});

	// Image metric (sharpness metric)
	connect(ui->comboBox_imageMetric, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, [this](int index) {
			this->parameters.sharpnessMetric = static_cast<ASCAN_SHARPNESS_METRIC>(index);
			emit paramsChanged(this->parameters);
		});

	// Metric threshold
	connect(ui->doubleSpinBox_metricThreshold, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, [this](double value) {
			this->parameters.metricThreshold = value;
			emit paramsChanged(this->parameters);
		});

	// d2 start and end values
	connect(ui->doubleSpinBox_d2Start, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, [this](double value) {
			this->parameters.d2start = value;
			emit paramsChanged(this->parameters);
		});
	connect(ui->doubleSpinBox_d2End, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, [this](double value) {
			this->parameters.d2end = value;
			emit paramsChanged(this->parameters);
		});

	// d3 start and end values
	connect(ui->doubleSpinBox_d3Start, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, [this](double value) {
			this->parameters.d3start = value;
			emit paramsChanged(this->parameters);
		});
	connect(ui->doubleSpinBox_d3End, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, [this](double value) {
			this->parameters.d3end = value;
			emit paramsChanged(this->parameters);
		});

	// Number of dispersion samples
	connect(ui->spinBox_numberOfDispersionSamples, QOverload<int>::of(&QSpinBox::valueChanged),
		this, [this](int value) {
			this->parameters.numberOfDispersionSamples = value;
			emit paramsChanged(this->parameters);
		});

	// Buttons
	connect(ui->pushButton_fetch, &QPushButton::clicked,this, &DispersionEstimatorForm::singleFetchRequested);
	connect(ui->toolButton_settings, &QToolButton::clicked, this, &DispersionEstimatorForm::toggleUIVisibility);
}

void DispersionEstimatorForm::setupPlot() {
	this->linePlot = this->ui->widget_linePlot;
	this->linePlot->setFirstCurveName("d2");
	this->linePlot->setSecondCurveName("d3");
	this->linePlot->setXAxisLabel("Dispersion Parameter");
	this->linePlot->setYAxisLabel("Metric Value");

	this->ascanPlot = this->ui->widget_linePlot_ascans;
	this->ascanPlot->setFirstCurveName("before");
	this->ascanPlot->setSecondCurveName("after");
	this->ascanPlot->setXAxisLabel("Sample");
	this->ascanPlot->setYAxisLabel("Intensity");
}

