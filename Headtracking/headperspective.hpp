
/**
 *
 * Head perspective display header.
 *
 */
#ifndef _HEADPERSPECTIVE_HPP_463738376754
#define _HEADPERSPECTIVE_HPP_463738376754

#include <QGLWidget>
#include <QApplication>
#include <QtGui>

#include <QTimer>

#include <opencv/highgui.h>
#include <opencv/cxcore.h>
#include <opencv/cv.h>

#include "headtrackfilter.hpp"

class HeadPerspective:public QGLWidget
{
  Q_OBJECT 

public:

  HeadPerspective (QWidget * parent = 0, HeadTrackFilter * app = 0);
  virtual ~ HeadPerspective ();

protected:

  void resizeGL (int width, int height);
  void paintGL ();
  void initializeGL ();

public:

  void setHeadCoords (float *headPosition);

  void resetHead ();
  void toggleAnaglyph ();
  void setScene (int scene);

protected:

  void setUserPerspective ();

  void drawTargets ();
  void drawTarget (float radius, float r, float g, float b);
  void drawCube (GLfloat size, GLenum type);

  void keyPressEvent (QKeyEvent * kEvent);

private:

  GLfloat m_headPosition[3];

  int m_width;
  int m_height;

  int m_monitorWidth;
  int m_monitorHeight;

  bool m_firstCoords;

  bool m_anaglyph;

  CvKalman *m_kalman;

  CvMat *m_measurement;

  HeadTrackFilter *m_tracker;
};

#endif // _HEADPERSPECTIVE_HPP_463738376754
