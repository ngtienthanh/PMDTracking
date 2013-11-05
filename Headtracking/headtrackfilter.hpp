#ifndef HEADTRACKFILTER_HPP_8932408979102
#define HEADTRACKFILTER_HPP_8932408979102

#include <opencv/highgui.h>
#include <opencv/cxcore.h>
#include <opencv/cv.h>

using namespace cv;

class HeadTrackFilter
{

public:

  // / the constructor
  HeadTrackFilter ();

  // / the destructor
  ~HeadTrackFilter ();

  int findFace (IplImage ** pImage, int &nLeft, int &nTop, int &nWidth, int &nHeight, int &faceX, int &faceY);

  void resetHead ();

private:

  CvHaarClassifierCascade * m_cascade;
  CvMemStorage *m_storage;

  IplImage *m_depthTemplate;
};

#endif // HEADTRACKFILTER_HPP_8932408979102
