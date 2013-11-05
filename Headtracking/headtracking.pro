TEMPLATE = app
INCLUDEPATH += /usr/local/pmd/include /usr/local/include/opencv /usr/local/include/opencv2
CONFIG += qt plugin debug_and_release 
QMAKE_LIBDIR += /usr/local/pmd/bin /usr/local/lib
LIBS += -lpmdaccess2 -lopencv_core -lopencv_objdetect -lopencv_highgui -lopencv_video -lopencv_imgproc -lGLU
DEPENDPATH += .

QT += opengl 

# Input
HEADERS += mainwindow.hpp headtracking.hpp headperspective.hpp headtrackfilter.hpp
SOURCES += main.cpp mainwindow.cpp headtracking.cpp headperspective.cpp headtrackfilter.cpp
TARGET   = headtracking
