#ifndef HEADTRACK_HPP_9087598984
#define HEADTRACK_HPP_9087598984

#include <QtGui>
#include <QLabel>
#include <QPainter>

#include <opencv/highgui.h>
#include <opencv/cxcore.h>
#include <opencv/cv.h>

#include <pmdsdk2.h>

#include "headperspective.hpp"
#include "headtrackfilter.hpp"

using namespace cv;

/** Simple image visualisation.
 * Displays amplitudes, distances or intensities as an image.
 */
class HeadTracking:QWidget
{

  Q_OBJECT 

public:

      /** Contructor
       * \param parent Parent QObject
       */
  HeadTracking (QWidget * parent);

      /** Destructor */
  ~HeadTracking ();

      /** from LightVisApp */
  QWidget *makeWidget (QWidget * parent);

  void newSourceData (PMDDataDescription * dd, void *data);
  void newAmplitudes (float *);
  void new3DCoordinates (float *);
  void newFlags (unsigned *);
  void finishedFrame ();

private:

  void getCoords (int faceX, int faceY);

private:

      /** Number of rows in the current image */
  unsigned m_rows;

      /** Number of columns in the current image */
  unsigned m_columns;

      /** Origin of the image */
  unsigned m_pixelOrigin;

      /** Number of pixels currently allocated in m_pixels */
  unsigned m_reservedPixels;

      /** Qt image for the display widget */
  QImage m_image;

      /** Label to display the image in */
  QLabel *m_imageLabel;

  QLabel *m_coordLabel;

      /** Amplitudes array */
  float *m_amplitudes;

      /** Coordinates array */
  float *m_coords;

      /** Flags array */
  unsigned *m_flags;

      /** Crop pixels from all four sides */
  unsigned m_cropBy;

      /** Number of instantiations of the class. Not a reference counter. */
  static unsigned s_instances;

  IplImage *m_gray;

  float m_headPosition[3];

  HeadPerspective *m_perspecView;
  HeadTrackFilter *m_tracker;
};

#endif // HEADTRACK_HPP_9087598984
