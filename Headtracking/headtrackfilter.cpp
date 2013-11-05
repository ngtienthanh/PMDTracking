#include "headtrackfilter.hpp"

HeadTrackFilter::HeadTrackFilter ()
{
  m_depthTemplate = NULL;

  // Load the classifier
  // In this case we use the classifier shipped with OpenCV
  m_cascade = (CvHaarClassifierCascade *) cvLoad ("haarcascade_frontalface_alt.xml", 0, 0, 0);

  if (!m_cascade)
    {
      printf ("Can't load cascade!\n");
      exit (-1);
    }

  m_storage = cvCreateMemStorage (0);
}

HeadTrackFilter::~HeadTrackFilter ()
{
  cvReleaseMemStorage (&m_storage);
  cvReleaseHaarClassifierCascade (&m_cascade);

  if (m_depthTemplate)
    {
      cvReleaseImage (&m_depthTemplate);
    }
}

int HeadTrackFilter::findFace (IplImage ** pImage, int &nLeft, int &nTop,
                               int &nWidth, int &nHeight, int &faceX, int &faceY)
{
  faceX = 0;
  faceY = 0;

  // If no face was found before try to find one with Haar classifiers.
  // If we found a face before and have created a template, try to find the 
  // template with template matching.

  if (!m_depthTemplate)
    {
      cvClearMemStorage (m_storage);

      CvSeq *faces = cvHaarDetectObjects (*pImage, m_cascade, m_storage,
                                          1.2, 2,
                                          CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_SCALE_IMAGE,
                                          cvSize (0, 0));

      if (faces->total)
        {
          // Loop the number of faces found.
          for (int i = 0; i < (faces ? faces->total : 0); ++i)
            {
              // Create a new rectangle for drawing the face
              CvRect *r = (CvRect *) cvGetSeqElem (faces, i);

              faceX = r->x + (r->width / 2);
              faceY = r->y + (r->height / 2);

              nLeft = r->x;
              nTop = r->y;
              nWidth = r->width;
              nHeight = r->height;

              // Create new template
              m_depthTemplate = cvCreateImage (cvSize (r->width, r->height), 8, 1);
              cvSetImageROI (*pImage, cvRect (r->x, r->y, r->width, r->height));
              cvCopy (*pImage, m_depthTemplate, NULL);
              cvResetImageROI (*pImage);
              return 1;
            }
        }
    }
  else
    {
      IplImage *templateResult = cvCreateImage (cvSize ((*pImage)->width - m_depthTemplate->width + 1,
                                                        (*pImage)->height - m_depthTemplate->height + 1), 32, 1);

      cvMatchTemplate (*pImage, m_depthTemplate, templateResult, CV_TM_CCOEFF_NORMED);

      double min_val = 0, max_val = 0;
      CvPoint min_loc, max_loc;
      cvMinMaxLoc (templateResult, &min_val, &max_val, &min_loc, &max_loc);

      cvReleaseImage (&templateResult);

      if (max_val > 0.85)
        {
          faceX = max_loc.x;
          faceY = max_loc.y;

          nLeft = faceX;
          nTop = faceY;
          nWidth = m_depthTemplate->width;
          nHeight = m_depthTemplate->height;

          cvSetImageROI (*pImage, cvRect (nLeft, nTop, nWidth, nHeight));
          cvCopy (*pImage, m_depthTemplate, NULL);
          cvResetImageROI (*pImage);
          return 2;
        }
      else
        {
          cvReleaseImage (&m_depthTemplate);
          m_depthTemplate = NULL;
          return findFace (pImage, nLeft, nTop, nWidth, nHeight, faceX, faceY);
        }
    }

  return 0;
}

void HeadTrackFilter::resetHead ()
{
  if (m_depthTemplate)
    {
      cvReleaseImage (&m_depthTemplate);
    }

  m_depthTemplate = NULL;
}
