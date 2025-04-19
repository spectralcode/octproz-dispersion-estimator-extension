QT += core gui widgets printsupport
QMAKE_PROJECT_DEPTH = 0

TARGET = dispersionestimatorextension
TEMPLATE = lib
CONFIG += plugin

#define path of OCTproZ_DevKit share directory, plugin/extension directory
SHAREDIR = $$shell_path($$PWD/../../octproz_share_dev)
PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins)

CONFIG(debug, debug|release) {
	PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins/debug)
}
CONFIG(release, debug|release) {
	PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins/release)
}

#Create PLUGINEXPORTDIR directory if it does not already exist
exists($$PLUGINEXPORTDIR){
	message("sharedir already existing")
}else{
	QMAKE_POST_LINK += $$quote(mkdir $${PLUGINEXPORTDIR} $$escape_expand(\\n\\t))
}


DEFINES += \
	DISPERSIONESTIMATION_LIBRARY \
	QT_DEPRECATED_WARNINGS #emit warnings if depracted Qt features are used

SOURCES += \
	src/dispersionestimator.cpp \
	src/dispersionestimatorform.cpp \
	src/dispersionestimationengine.cpp\
	src/thirdparty/qcustomplot/qcustomplot.cpp \
	src/lineplot.cpp \
	src/octprocessor/processor.tpp \
	src/octprocessor/processorcontroller.cpp\
	src/ascanmetriccalculator.cpp

HEADERS += \
	src/dispersionestimator.h \
	src/dispersionestimatorform.h \
	src/dispersionestimatorparameters.h \
	src/dispersionestimationengine.h\
	src/thirdparty/qcustomplot/qcustomplot.h \
	src/lineplot.h \
	src/octprocessor/processor.h \
	src/octprocessor/processorcontroller.h\
	src/ascanmetriccalculator.h

FORMS +=  \
	src/dispersionestimatorform.ui

INCLUDEPATH += \
	$$SHAREDIR \
	src \
	src/octprocessor \
	src/overlayitems \
	src/thirdparty \
	src/thirdparty/fftw \
	src/thirdparty/qcustomplot \

unix{
	LIBS += -lfftw3
}
win32{
	LIBS += -L$$PWD/src/thirdparty/fftw/ -llibfftw3-3
	DEPENDPATH += $$PWD/src/thirdparty/fftw
}


#set system specific output directory for extension
unix{
	OUTFILE = $$shell_path($$OUT_PWD/lib$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
}
win32{
	CONFIG(debug, debug|release) {
		OUTFILE = $$shell_path($$OUT_PWD/debug/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
	}
	CONFIG(release, debug|release) {
		OUTFILE = $$shell_path($$OUT_PWD/release/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
	}
}


#specifie OCTproZ_DevKit libraries to be linked to extension project
CONFIG(debug, debug|release) {
	unix{
		LIBS += $$shell_path($$SHAREDIR/debug/libOCTproZ_DevKit.a)
	}
	win32{
		LIBS += $$shell_path($$SHAREDIR/debug/OCTproZ_DevKit.lib)
	}
}
CONFIG(release, debug|release) {
	PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins/release)
	unix{
		LIBS += $$shell_path($$SHAREDIR/release/libOCTproZ_DevKit.a)
	}
	win32{
		LIBS += $$shell_path($$SHAREDIR/release/OCTproZ_DevKit.lib)
	}
}


##Copy extension to "PLUGINEXPORTDIR"
unix{
	QMAKE_POST_LINK += $$QMAKE_COPY $$quote($${OUTFILE}) $$quote($$PLUGINEXPORTDIR) $$escape_expand(\\n\\t)
}
win32{
	QMAKE_POST_LINK += $$QMAKE_COPY $$quote($${OUTFILE}) $$quote($$shell_path($$PLUGINEXPORTDIR/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})) $$escape_expand(\\n\\t)
}

##Add extension to clean directive. When running "make clean" plugin will be deleted
unix {
	QMAKE_CLEAN += $$shell_path($$PLUGINEXPORTDIR/lib$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
}
win32 {
	QMAKE_CLEAN += $$shell_path($$PLUGINEXPORTDIR/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
}
