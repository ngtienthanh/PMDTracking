#include "headtracking.hpp"

#include <math.h>
#include <pmdsdk2.h>
#include <QLayout>
#include <QComboBox>
#include <QDoubleSpinBox>

HeadTracking::HeadTracking (QWidget * parent):QWidget (parent)
{
  m_reservedPixels = 0;
  m_rows = 0;
  m_columns = 0;
  m_amplitudes = 0;
  m_cropBy = 4;
  m_coords = NULL;
  m_flags = NULL;

  m_headPosition[0] = 0.0f;
  m_headPosition[1] = 0.0f;
  m_headPosition[2] = 2.0f;

  m_tracker = new HeadTrackFilter ();

  m_perspecView = NULL;

  m_gray = NULL;
}

HeadTracking::~HeadTracking ()
{
  delete m_perspecView;
  delete m_tracker;

  if (m_gray)
    {
      cvReleaseImage (&m_gray);
    }

  delete[]m_amplitudes;
  delete[]m_coords;
  delete[]m_flags;
}

QWidget *HeadTracking::makeWidget (QWidget * parent)
{
  QWidget *mainWidget = new QWidget (parent);
  QGridLayout *layout = new QGridLayout (mainWidget);

  m_coordLabel = new QLabel ();
  m_coordLabel->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Maximum);
  layout->addWidget (m_coordLabel, 0, 0, 1, 2, Qt::AlignCenter);

  m_imageLabel = new QLabel ();
  m_imageLabel->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_imageLabel->setScaledContents (false);
  layout->addWidget (m_imageLabel, 1, 0, 1, 1, Qt::AlignCenter);

  QWidget *threedWidget = new QWidget ();
  threedWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  QGridLayout *layout2 = new QGridLayout (threedWidget);
  layout->addWidget (threedWidget, 1, 1, 1, 1);

  m_perspecView = new HeadPerspective (threedWidget, m_tracker);
  m_perspecView->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_perspecView->setFocusPolicy (Qt::StrongFocus);
  layout2->addWidget (m_perspecView);

  return mainWidget;
}

void HeadTracking::newSourceData (PMDDataDescription * dd, void *)
{
  m_pixelOrigin = dd->img.pixelOrigin;
  m_rows = dd->img.numRows;
  m_columns = dd->img.numColumns;

  // If format has changed reinitialize arrays
  if (m_rows * m_columns != m_reservedPixels)
    {
      m_reservedPixels = m_rows * m_columns;

      delete[]m_amplitudes;
      m_amplitudes = new float[m_reservedPixels];

      delete[]m_flags;
      m_flags = new unsigned[m_reservedPixels];

      memset (m_flags, 0, m_reservedPixels * sizeof (unsigned));

      delete[]m_coords;
      m_coords = new float[m_reservedPixels * 3];

      if (m_gray)
        {
          cvReleaseImage (&m_gray);
        }
      switch (m_pixelOrigin & 0xffff0000)
        {
          case PMD_DIRECTION_VERTICAL:
            {
              m_gray = cvCreateImage (cvSize (m_rows, m_columns), 8, 1);
              break;
            }
          default:
            {
              m_gray = cvCreateImage (cvSize (m_columns, m_rows), 8, 1);
              break;
            }
        }
    }
}

inline float clipZero (float v)
{
  if (v < 0)
    return 0;
  return v;
}

inline float clipAbove (float t, float v)
{
  if (v > t)
    return t;
  return v;
}

void HeadTracking::newAmplitudes (float *amps)
{
  unsigned max = 0;

  unsigned i, j;
  unsigned idx = 0;

  // Find the maximum for scaling
  for (i = 0; i < m_rows; ++i)
    {
      for (j = 0; j < m_columns; ++j)
        {
          idx = i * m_columns + j;
          if (max < amps[idx])
            {
              max = amps[idx];
            }
        }
    }

  // Flip and scale amplitudes
  idx = 0;
  unsigned w, h;
  unsigned x, y;
  for (i = 0; i < m_rows; ++i)
    {
      for (j = 0; j < m_columns; ++j, ++idx)
        {
          switch (m_pixelOrigin & 0xffff0000)
            {
              case PMD_DIRECTION_VERTICAL:
                x = i;
                y = j;
                w = m_rows;
                h = m_columns;
                break;
              default:
                x = j;
                y = i;
                w = m_columns;
                h = m_rows;
                break;
            }
          switch (m_pixelOrigin & 0x00000003)
            {
              case PMD_ORIGIN_TOP_RIGHT:
                x = w - 1 - x;
                break;
              case PMD_ORIGIN_TOP_LEFT:
                break;
              case PMD_ORIGIN_BOTTOM_RIGHT:
                x = w - 1 - x;
                y = h - 1 - y;
                break;
              case PMD_ORIGIN_BOTTOM_LEFT:
                y = h - 1 - y;
                break;
            }

          unsigned char dval =
            clipAbove (255,
                       128.0 * amps[idx] / max + 128.0 * log (1.0 + clipZero (amps[idx])) / log (1.0 + clipZero (max)));

          unsigned newidx = y * w + x;

          m_amplitudes[newidx] = amps[idx];

          m_gray->imageData[y * m_gray->widthStep + x] = (char) dval;
        }
    }
}

void HeadTracking::new3DCoordinates (float *coord)
{
  unsigned w, h;
  unsigned i, j;
  unsigned x, y;
  unsigned idx = 0;
  unsigned newidx;

  // Flip 3D coordinates
  for (i = 0; i < m_rows; ++i)
    {
      for (j = 0; j < m_columns; ++j, ++idx)
        {
          switch (m_pixelOrigin & 0xffff0000)
            {
              case PMD_DIRECTION_VERTICAL:
                x = i;
                y = j;
                w = m_rows;
                h = m_columns;
                break;
              default:
                x = j;
                y = i;
                w = m_columns;
                h = m_rows;
                break;
            }
          switch (m_pixelOrigin & 0x00000003)
            {
              case PMD_ORIGIN_TOP_RIGHT:
                x = w - 1 - x;
                break;
              case PMD_ORIGIN_TOP_LEFT:
                break;
              case PMD_ORIGIN_BOTTOM_RIGHT:
                x = w - 1 - x;
                y = h - 1 - y;
                break;
              case PMD_ORIGIN_BOTTOM_LEFT:
                y = h - 1 - y;
                break;
            }

          newidx = y * w + x;

          if ((m_pixelOrigin & 0xffff0000) == PMD_DIRECTION_VERTICAL)
            {
              m_coords[newidx * 3 + 0] = coord[idx * 3 + 1];
              m_coords[newidx * 3 + 1] = -coord[idx * 3 + 0];
            }
          else
            {
              m_coords[newidx * 3 + 0] = coord[idx * 3 + 0];
              m_coords[newidx * 3 + 1] = coord[idx * 3 + 1];
            }
          m_coords[newidx * 3 + 2] = coord[idx * 3 + 2];
        }
    }
}

void HeadTracking::newFlags (unsigned *flags)
{
  unsigned w, h;
  unsigned i, j;
  unsigned x, y;

  unsigned idx = 0;
  unsigned newidx;

  // Flip flags
  for (i = 0; i < m_rows; ++i)
    {
      for (j = 0; j < m_columns; ++j, ++idx)
        {
          switch (m_pixelOrigin & 0xffff0000)
            {
              case PMD_DIRECTION_VERTICAL:
                x = i;
                y = j;
                w = m_rows;
                h = m_columns;
                break;
              default:
                x = j;
                y = i;
                w = m_columns;
                h = m_rows;
                break;
            }
          switch (m_pixelOrigin & 0x00000003)
            {
              case PMD_ORIGIN_TOP_RIGHT:
                x = w - 1 - x;
                break;
              case PMD_ORIGIN_TOP_LEFT:
                break;
              case PMD_ORIGIN_BOTTOM_RIGHT:
                x = w - 1 - x;
                y = h - 1 - y;
                break;
              case PMD_ORIGIN_BOTTOM_LEFT:
                y = h - 1 - y;
                break;
            }

          newidx = y * w + x;

          m_flags[newidx] = flags[idx];
        }
    }
}

void HeadTracking::getCoords (int faceX, int faceY)
{
  double fSum[3] = { 0.0, 0.0, 0.0 };
  double fDivisor = 0.0;

  int window = 5;
  int x, y;

  int idx;

  // Retrieve the 3D coordinates of the face
  for (y = faceY - window; y <= faceY + window && y >= 0 && y < m_gray->height; ++y)
    {
      for (x = faceX - window; x <= faceX + window && x >= 0 && x < m_gray->width; ++x)
        {
          idx = (y * m_gray->width) + x;
          if ((m_flags[idx] & PMD_FLAG_INCONSISTENT) == 0x0 && m_coords[(idx * 3) + 2] > 0.0f)
            {
              fSum[0] += m_coords[(idx * 3) + 0] * m_amplitudes[idx];
              fSum[1] += m_coords[(idx * 3) + 1] * m_amplitudes[idx];
              fSum[2] += m_coords[(idx * 3) + 2] * m_amplitudes[idx];

              fDivisor += m_amplitudes[idx];
            }
        }
    }

  if (fDivisor > 0)
    {
      m_headPosition[0] = fSum[0] / fDivisor;
      m_headPosition[1] = fSum[1] / fDivisor;
      m_headPosition[2] = fSum[2] / fDivisor;
    }

}

void HeadTracking::finishedFrame ()
{
  int faceX, faceY;
  int nLeft = 0, nTop = 0, nWidth = 0, nHeight = 0;

  // Find the face
  int nRes = m_tracker->findFace (&m_gray, nLeft, nTop, nWidth, nHeight, faceX, faceY);
  if (nRes > 0)
    {
      getCoords (faceX, faceY);
    }

  m_coordLabel->setText ("X : " + QString::number (m_headPosition[0], 'f', 2) +
                         " Y : " + QString::number (m_headPosition[1], 'f', 2) +
                         " Z : " + QString::number (m_headPosition[2], 'f', 2));

  m_perspecView->setHeadCoords (m_headPosition);

  IplImage *rgbImage = cvCreateImage (cvGetSize (m_gray), 8, 4);

  cvCvtColor (m_gray, rgbImage, CV_GRAY2RGBA);
  int w = m_gray->width;
  int h = m_gray->height;
  m_image = QImage ((uchar *) rgbImage->imageData, w, h, QImage::Format_RGB32);
  if (nWidth && nHeight)
    {
      QPainter painter;
      painter.begin (&m_image);
      painter.setPen ((nRes == 1) ? Qt::red : Qt::green);
      painter.drawRect (QRect (nLeft, nTop, nWidth, nHeight));
      painter.end ();
    }
  m_imageLabel->setPixmap (QPixmap::fromImage (m_image));
  m_imageLabel->repaint ();

  cvReleaseImage (&rgbImage);
}
