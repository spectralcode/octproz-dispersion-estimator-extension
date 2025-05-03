#ifndef DISPERSIONESTIMATORPARAMETERS_H
#define DISPERSIONESTIMATORPARAMETERS_H

#include <QString>
#include <QtGlobal>
#include <QMetaType>
#include <QRect>
#include <QByteArray>

#define DISPERSION_ESTIMATOR_SOURCE "image_source"
#define DISPERSION_ESTIMATOR_FRAME "frame_number"
#define DISPERSION_ESTIMATOR_SOURCE "image_source"
#define DISPERSION_ESTIMATOR_FRAME "frame_number"
#define DISPERSION_ESTIMATOR_BUFFER_SOURCE "buffer_source"
#define DISPERSION_ESTIMATOR_ROI "roi"
#define DISPERSION_ESTIMATOR_FRAME_NR "frame_nr"
#define DISPERSION_ESTIMATOR_BUFFER_NR "buffer_nr"
#define DISPERSION_ESTIMATOR_NUMBER_OF_CENTER_ASCANS "number_of_center_ascans"
#define DISPERSION_ESTIMATOR_USE_LINEAR_ASCANS "use_linear_ascans"
#define DISPERSION_ESTIMATOR_NUMBER_OF_ASCAN_SAMPLES_TO_IGNORE	"number_of_ascan_samples_to_ignore"
#define DISPERSION_ESTIMATOR_AUTO_CALC_D1 "auto_calculate_d1"
#define DISPERSION_ESTIMATOR_SHARPNESS_METRIC "sharpness_metric"
#define DISPERSION_ESTIMATOR_METRIC_THRESHOLD "metric_threshold"
#define DISPERSION_ESTIMATOR_D2_START "d2_start"
#define DISPERSION_ESTIMATOR_D2_END "d2_end"
#define DISPERSION_ESTIMATOR_D3_START "d3_start"
#define DISPERSION_ESTIMATOR_D3_END "d3_end"
#define DISPERSION_ESTIMATOR_NUMBER_OF_DISPERSION_SAMPLES "number_of_dispersion_samples"
#define DISPERSION_ESTIMATOR_WINDOW_STATE "dispersion_estimator_window_state"
#define DISPERSION_ESTIMATOR_GUI_TOGGLE "gui_visible"


enum BUFFER_SOURCE{
	RAW,
	PROCESSED
};

enum ASCAN_SHARPNESS_METRIC{
	SUM_ABOVE_THRESHOLD,
	SAMPLES_ABOVE_THRESHOLD,
	PEAK_VALUE,
	MEAN_SOBEL
};

struct DispersionEstimatorParameters {
	BUFFER_SOURCE bufferSource;
	QRect roi;
	int frameNr;
	int bufferNr;
	int numberOfCenterAscans;
	bool useLinearAscans;
	int numberOfAscanSamplesToIgnore;
	bool autoCalcD1;
	ASCAN_SHARPNESS_METRIC sharpnessMetric;
	qreal metricThreshold;
	qreal d2start;
	qreal d2end;
	qreal d3start;
	qreal d3end;
	int numberOfDispersionSamples;
	QByteArray windowState;
	bool guiVisible;
};
Q_DECLARE_METATYPE(DispersionEstimatorParameters)


#endif //DISPERSIONESTIMATORPARAMETERS_H


