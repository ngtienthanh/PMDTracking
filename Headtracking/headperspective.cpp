
/**
 *
 * Head perspective display implementation.
 *
 */
#include "./headperspective.hpp"

#include <GL/glu.h>

const static GLfloat cube_normals[6][3] = {
  {-1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0},
  {0.0, 0.0, 1.0},
  {0.0, 0.0, -1.0}
};
const static GLint cube_faces[6][4] = {
  {0, 1, 2, 3},
  {3, 2, 6, 7},
  {7, 6, 5, 4},
  {4, 5, 1, 0},
  {5, 6, 2, 1},
  {7, 4, 0, 3}
};

HeadPerspective::HeadPerspective (QWidget * parent, HeadTrackFilter * tracker):QGLWidget (parent)
{
  m_tracker = tracker;

  m_headPosition[0] = 0.0f;
  m_headPosition[1] = 0.0f;
  m_headPosition[2] = 2.0f;

  m_monitorWidth = 475;

  m_firstCoords = true;
  m_anaglyph = false;

  m_kalman = cvCreateKalman (6, 3, 0);

  m_measurement = cvCreateMat (3, 1, CV_32FC1);

  const float F[] = {
    1, 0, 0, 1, 0, 0,           // x + dx
    0, 1, 0, 0, 1, 0,           // y + dy
    0, 0, 1, 0, 0, 1,           // z + dz
    0, 0, 0, 1, 0, 0,           // dx = dx
    0, 0, 0, 0, 1, 0,           // dy = dy
    0, 0, 0, 0, 0, 1,           // dz = dz
  };
  memcpy (m_kalman->transition_matrix->data.fl, F, sizeof (F));

  cvZero (m_measurement);
}

HeadPerspective::~HeadPerspective ()
{
  cvReleaseKalman (&m_kalman);
  cvReleaseMat (&m_measurement);
}

void HeadPerspective::setHeadCoords (float *headPosition)
{
  if (m_firstCoords)
    {
      resetHead ();
      m_firstCoords = false;
    }

  // Update Kalman filter
  m_measurement->data.fl[0] = headPosition[0] * 1000.0f;        // x point
  m_measurement->data.fl[1] = headPosition[1] * 1000.0f;        // y point
  m_measurement->data.fl[2] = headPosition[2] * 1000.0f;        // z point

  const CvMat *prediction = cvKalmanPredict (m_kalman, 0);

  // adjust Kalman filter state
  cvKalmanCorrect (m_kalman, m_measurement);

  m_headPosition[0] = prediction->data.fl[0];
  m_headPosition[1] = prediction->data.fl[1];
  m_headPosition[2] = prediction->data.fl[2];

  updateGL ();

}

void HeadPerspective::initializeGL ()
{
  glDisable (GL_LIGHTING);
  glDisable (GL_CULL_FACE);
  glEnable (GL_DEPTH_TEST);
}

void HeadPerspective::setUserPerspective ()
{
  // Set user oriented perspective
  double aspect = 1.0f;
  if (this->height () > 0 && this->width () > 0)
    {
      aspect = (GLfloat) this->width () / (GLfloat) this->height ();
    }

  double nearPlane = 0.05f;
  double zRatio = nearPlane;
  if (m_headPosition[2] != 0.0f)
    {
      zRatio /= m_headPosition[2];
    }

  m_monitorHeight = m_monitorWidth / aspect;

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();

  glFrustum ((m_headPosition[0] - 0.5f * m_monitorWidth) * zRatio,
             (m_headPosition[0] + 0.5f * m_monitorWidth) * zRatio,
             (-m_headPosition[1] - 0.5f * m_monitorHeight) * zRatio,
             (-m_headPosition[1] + 0.5f * m_monitorHeight) * zRatio, nearPlane, 10000.0f);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (m_headPosition[0], -m_headPosition[1], -m_headPosition[2]);
}

void HeadPerspective::paintGL ()
{
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set the user-centered perspective
  setUserPerspective ();

  if (m_anaglyph)
    {
      // Draw left eye view
      glClear (GL_DEPTH_BUFFER_BIT);

      glColorMask (GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);

      glPushMatrix ();
      gluLookAt (-0.025, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

      drawTargets ();

      glPopMatrix ();

      // Draw right eye view
      glClear (GL_DEPTH_BUFFER_BIT);

      glColorMask (GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);

      glPushMatrix ();
      gluLookAt (0.025, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

      drawTargets ();

      glPopMatrix ();
      glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
  else
    {
      drawTargets ();
    }
}

void HeadPerspective::resizeGL (int width, int height)
{
  if (!width || !height)
    {
      return;
    }

  m_width = width;
  m_height = height;

  glViewport (0, 0, (GLint) m_width, (GLint) m_height);

  glDisable (GL_LIGHTING);
  glDisable (GL_CULL_FACE);
  glEnable (GL_DEPTH_TEST);
}

void HeadPerspective::drawTargets ()
{
  glDisable (GL_TEXTURE_2D);

  // Ceiling
  glPushMatrix ();
  glBegin (GL_QUADS);
  glColor3f (0.3f, 0.3f, 0.3f);
  glVertex3i (-m_monitorWidth / 2, m_monitorHeight / 2, 0);
  glVertex3i (m_monitorWidth / 2, m_monitorHeight / 2, 0);
  glVertex3i (m_monitorWidth / 2, m_monitorHeight / 2, -m_monitorWidth);
  glVertex3i (-m_monitorWidth / 2, m_monitorHeight / 2, -m_monitorWidth);
  glEnd ();
  glPopMatrix ();

  // Floor
  glPushMatrix ();
  glBegin (GL_QUADS);
  glColor3f (0.3f, 0.3f, 0.3f);
  glVertex3i (-m_monitorWidth / 2, -m_monitorHeight / 2, 0);
  glVertex3i (m_monitorWidth / 2, -m_monitorHeight / 2, 0);
  glVertex3i (m_monitorWidth / 2, -m_monitorHeight / 2, -m_monitorWidth);
  glVertex3i (-m_monitorWidth / 2, -m_monitorHeight / 2, -m_monitorWidth);
  glEnd ();
  glPopMatrix ();

  // Left
  glPushMatrix ();
  glBegin (GL_QUADS);
  glColor3f (0.4f, 0.4f, 0.4f);
  glVertex3i (-m_monitorWidth / 2, m_monitorHeight / 2, 0);
  glVertex3i (-m_monitorWidth / 2, -m_monitorHeight / 2, 0);
  glVertex3i (-m_monitorWidth / 2, -m_monitorHeight / 2, -m_monitorWidth);
  glVertex3i (-m_monitorWidth / 2, m_monitorHeight / 2, -m_monitorWidth);
  glEnd ();
  glPopMatrix ();

  // Right
  glPushMatrix ();
  glBegin (GL_QUADS);
  glColor3f (0.4f, 0.4f, 0.4f);
  glVertex3i (m_monitorWidth / 2, m_monitorHeight / 2, 0);
  glVertex3i (m_monitorWidth / 2, -m_monitorHeight / 2, 0);
  glVertex3i (m_monitorWidth / 2, -m_monitorHeight / 2, -m_monitorWidth);
  glVertex3i (m_monitorWidth / 2, m_monitorHeight / 2, -m_monitorWidth);
  glEnd ();
  glPopMatrix ();

  // Back
  glPushMatrix ();
  glBegin (GL_QUADS);
  glColor3f (0.2f, 0.2f, 0.2f);
  glVertex3i (-m_monitorWidth / 2, m_monitorHeight / 2, -m_monitorWidth);
  glVertex3i (m_monitorWidth / 2, m_monitorHeight / 2, -m_monitorWidth);
  glVertex3i (m_monitorWidth / 2, -m_monitorHeight / 2, -m_monitorWidth);
  glVertex3i (-m_monitorWidth / 2, -m_monitorHeight / 2, -m_monitorWidth);
  glEnd ();
  glPopMatrix ();

  float target_offset = 80;
  float target_radius = 20;
  float depth_increment = m_monitorWidth / 3;

  // Center
  glPushMatrix ();
  glTranslated (0.0, 0.0, 0.0);
  drawTarget (target_radius, 0.0f, 0.0f, 0.0f);
  glPopMatrix ();

  // Left
  glPushMatrix ();
  glTranslated (-target_offset, 0, depth_increment);
  drawTarget (target_radius, 0.0f, 0.0f, 0.0f);
  glPopMatrix ();

  // Top
  glPushMatrix ();
  glTranslated (0, target_offset, -depth_increment);
  drawTarget (target_radius, 0.0f, 0.0f, 0.0f);
  glPopMatrix ();

  // Right
  glPushMatrix ();
  glTranslated (target_offset, 0, -2 * depth_increment);
  drawTarget (target_radius, 0.0f, 0.0f, 0.0f);
  glPopMatrix ();

  // Down
  glPushMatrix ();
  glTranslated (0, -target_offset, 2 * depth_increment);
  drawTarget (target_radius, 0.0f, 0.0f, 0.0f);
  glPopMatrix ();
}

void HeadPerspective::drawTarget (float radius, float r, float g, float b)
{
  glPushMatrix ();
  glColor3f (1.0f, 1.0f, 1.0f);
  float length = 10000.0f;
  glTranslated (0, 0, -(length / 2 + 15));
  glScaled (radius / 10.0f, radius / 10.0f, length);
  drawCube (1, GL_QUADS);
  glPopMatrix ();

  glPushMatrix ();
  float radius_increment = radius / 4;

  {
    glColor3f (r, g, b);
    glPushMatrix ();
    GLUquadric *quadric = gluNewQuadric ();
    gluDisk (quadric, 0, radius_increment, 36, 36);
    gluDeleteQuadric (quadric);
    glPopMatrix ();
  }

  {
    glColor3f (1.0f, 1.0f, 1.0f);
    glPushMatrix ();
    GLUquadric *quadric = gluNewQuadric ();
    gluDisk (quadric, radius_increment, 2.0 * radius_increment, 36, 36);
    gluDeleteQuadric (quadric);
    glPopMatrix ();
  }

  {
    glColor3f (r, g, b);
    glPushMatrix ();
    GLUquadric *quadric = gluNewQuadric ();
    gluDisk (quadric, 2.0 * radius_increment, 3.0 * radius_increment, 36, 36);
    gluDeleteQuadric (quadric);
    glPopMatrix ();
  }

  {
    glColor3f (1.0f, 1.0f, 1.0f);
    glPushMatrix ();
    GLUquadric *quadric = gluNewQuadric ();
    gluDisk (quadric, 3.0 * radius_increment, 4.0 * radius_increment, 36, 36);
    gluDeleteQuadric (quadric);
    glPopMatrix ();
  }

  glPopMatrix ();
}

void HeadPerspective::drawCube (GLfloat size, GLenum type)
{
  GLfloat v[8][3];
  GLint i;

  v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
  v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
  v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
  v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
  v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
  v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

  for (i = 5; i >= 0; i--)
    {
      glBegin (type);
      glNormal3fv (&cube_normals[i][0]);
      glVertex3fv (&v[cube_faces[i][0]][0]);
      glVertex3fv (&v[cube_faces[i][1]][0]);
      glVertex3fv (&v[cube_faces[i][2]][0]);
      glVertex3fv (&v[cube_faces[i][3]][0]);
      glEnd ();
    }
}

void HeadPerspective::resetHead ()
{
  // Initialize Kalman filter
  cvSetIdentity (m_kalman->measurement_matrix, cvRealScalar (1));
  cvSetIdentity (m_kalman->process_noise_cov, cvRealScalar (1e-3));
  cvSetIdentity (m_kalman->measurement_noise_cov, cvRealScalar (2e+2));
  cvSetIdentity (m_kalman->error_cov_post, cvRealScalar (1));
}

void HeadPerspective::toggleAnaglyph ()
{
  m_anaglyph = !m_anaglyph;
}

void HeadPerspective::keyPressEvent (QKeyEvent * kEvent)
{
  if (kEvent->key () == Qt::Key_R)
    {
      resetHead ();
      m_tracker->resetHead ();
    }
  else if (kEvent->key () == Qt::Key_A)
    {
      toggleAnaglyph ();
    }
}
