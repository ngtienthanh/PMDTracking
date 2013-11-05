#include "mainwindow.hpp"

MainWindow::MainWindow ()
{
  m_pFlags = 0;
  m_pAmplitudes = 0;
  m_pCoordinates = 0;
  m_dataSize = 0;

  m_thread = 0;
  m_fpsCounter = 0;

  m_hnd = 0;

  m_pApp = new HeadTracking (this);

  QWidget *mainWidget = m_pApp->makeWidget (this);

  this->setWindowTitle ("Headtracking Example");

  setCentralWidget (mainWidget);

  m_thread = new AquisitionThread ();
  QObject::connect (m_thread, SIGNAL (hasNewFrame (PMDDataDescription *, void *)), this,
                    SLOT (newFrame (PMDDataDescription *, void *)));

  openCam ();
}

MainWindow::~MainWindow ()
{
  m_thread->quit ();
  while (!m_thread->isFinished ())
    {
    }

  pmdClose (m_hnd);
}

void MainWindow::newFrame (PMDDataDescription * dd, void *data)
{
  ++m_fpsCounter;

  if (m_fpsCounter == 10)
    {
      m_fpsCounter = 0;
      statusBar ()->showMessage (QString ("%1 fps").arg (10000.0 / m_lastFrame.elapsed ()));
      m_lastFrame.restart ();
    }

  int res;
  char err[128];

  if (m_dataSize != dd->img.numColumns * dd->img.numRows)
    {
      m_dataSize = dd->img.numColumns * dd->img.numRows;
      delete[]m_pFlags;
      delete[]m_pAmplitudes;
      delete[]m_pCoordinates;

      m_pFlags = new unsigned[m_dataSize];
      m_pAmplitudes = new float[m_dataSize];
      m_pCoordinates = new float[m_dataSize * 3];
    }

  m_pApp->newSourceData (dd, data);

  res = pmdCalcAmplitudes (m_hnd, m_pAmplitudes, dd->img.numColumns * dd->img.numRows * sizeof (float), *dd, data);
  if (res != PMD_OK)
    {
      pmdGetLastError (m_hnd, err, 128);
      fprintf (stderr, "Could not get amplitudes: %s\n", err);
      exit (1);
    }

  m_pApp->newAmplitudes (m_pAmplitudes);

  res = pmdCalc3DCoordinates (m_hnd, m_pCoordinates, dd->img.numColumns * dd->img.numRows * sizeof (float) * 3, *dd, data);
  if (res != PMD_OK)
    {
      pmdGetLastError (m_hnd, err, 128);
      fprintf (stderr, "Could not get coordinates: %s\n", err);
      exit (1);
    }

  m_pApp->new3DCoordinates (m_pCoordinates);

  res = pmdCalcFlags (m_hnd, m_pFlags, dd->img.numColumns * dd->img.numRows * sizeof (unsigned), *dd, data);
  if (res != PMD_OK)
    {
      pmdGetLastError (m_hnd, err, 128);
      fprintf (stderr, "Could not get flags: %s\n", err);
      exit (1);
    }

  m_pApp->newFlags (m_pFlags);

  m_pApp->finishedFrame ();

  m_thread->releaseData (dd, data);
}

void MainWindow::openCam ()
{
  int res;
  char err[128];

  res = pmdOpen (&m_hnd, "camboardnano.L32.pap", "", "camboardnanoproc.L32.ppp", "");
  if (res != PMD_OK)
    {
      pmdGetLastError (0, err, 128);
      fprintf (stderr, "Could not connect: %s\n", err);
      exit (1);
    }

  pmdSetIntegrationTime (m_hnd, 0, 500);

  m_thread->setHandle (m_hnd);

  m_thread->start ();
}

AquisitionThread::AquisitionThread ()
{
  m_framesInUse = 0;
  m_hnd = 0;
  m_timer = 0;
}

AquisitionThread::~AquisitionThread ()
{
  delete m_timer;
  m_hnd = 0;
}

void AquisitionThread::setHandle (PMDHandle hnd)
{
  m_hnd = hnd;
}

void AquisitionThread::run ()
{
  if (m_timer)
    {
      delete m_timer;
    }

  m_timer = new QTimer (this);
  connect (m_timer, SIGNAL (timeout ()), this, SLOT (aquire ()), Qt::DirectConnection);
  m_timer->setInterval (0);
  m_timer->start ();
  exec ();
}

void AquisitionThread::aquire ()
{
  if (m_framesInUse >= 3 || m_hnd <= 0)
    {
      return;
    }

  int res;
  char err[128];

  res = pmdUpdate (m_hnd);
  if (res != PMD_OK)
    {
      pmdGetLastError (m_hnd, err, 128);
      QMessageBox::critical (qApp->activeWindow (), "PMD Examples", QString ("PMD Update Error : ") + QString (err));
      printf ("Could not transfer data: %s\n", err);
      exit (1);
    }

  PMDDataDescription *dd = new PMDDataDescription ();

  res = pmdGetSourceDataDescription (m_hnd, dd);
  if (res != PMD_OK)
    {
      pmdGetLastError (m_hnd, err, 128);
      printf ("Could not get data description: %s\n", err);
      exit (1);
    }

  if (dd->subHeaderType != PMD_IMAGE_DATA)
    {
      printf ("Source data is not an image!\n");
      exit (1);
    }

  size_t rddsize;
  pmdGetSourceDataSize (m_hnd, &rddsize);

  unsigned char *pmdData = new unsigned char[rddsize];

  res = pmdGetSourceData (m_hnd, (void *) pmdData, rddsize);
  if (res != PMD_OK)
    {
      pmdGetLastError (m_hnd, err, 128);
      printf ("Could not transfer data: %s\n", err);
      exit (1);
    }

  ++m_framesInUse;

  emit hasNewFrame (dd, pmdData);
}

void AquisitionThread::releaseData (PMDDataDescription * dd, void *data)
{
  delete dd;
  delete[](char *) data;
  --m_framesInUse;
}
