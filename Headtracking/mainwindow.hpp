#ifndef MAINWINDOW_HPP_984735989
#define MAINWINDOW_HPP_984735989

#include <QtGui>
#include <pmdsdk2.h>

#include "headtracking.hpp"

class AquisitionThread:public QThread
{

  Q_OBJECT 

public:

  AquisitionThread ();
  ~AquisitionThread ();

  void run ();

  void setHandle (PMDHandle hnd);
  void releaseData (PMDDataDescription * dd, void *data);

public slots:

  void aquire ();

signals: 

  void hasNewFrame (PMDDataDescription * dd, void *data);

private:

  PMDHandle m_hnd;
  QTimer *m_timer;
  int m_framesInUse;
  QMutex m_mutex;
};

class MainWindow:public QMainWindow
{
  Q_OBJECT

public:

      /** Constructor */
  MainWindow ();

      /** Desctructor */
  ~MainWindow ();

public slots:

  void newFrame (PMDDataDescription * dd, void *data);

private:

  void openCam ();

  void startRecognition ();

  unsigned *m_pFlags;
  float *m_pAmplitudes;
  float *m_pCoordinates;
  unsigned m_dataSize;

  HeadTracking *m_pApp;

  PMDHandle m_hnd;

  AquisitionThread *m_thread;

  int m_fpsCounter;
  QTime m_lastFrame;
};

#endif // MAINWINDOW_HPP_984735989
