
/**
 * @file
 * OSGWidget class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 *         Tomáš Pafčo
 */


#include <osgViewer/ViewerBase>
#include <QMouseEvent>
#include <cassert>
#include "OSGWidget.h"
#include "Lexolights.h"
#include "CadworkViewer.h"
#include "MainWindow.h"
#include "lighting/ShadowVolume.h"
#include "gui/LogWindow.h"
#include "utils/Log.h"
#include "utils/SysInfo.h"
#include "threading/MainThreadRoutine.h"

using namespace std;


/**
 * Constructor.
 *
 * @param parent Parent widget.
 * @param flags  Window flags (Qt::WindowFlags).
 */
OSGWidget::OSGWidget( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
   : inherited( parent, shareWidget, f, true )
{
   commonConstructor();
}


OSGWidget::OSGWidget( QGLContext* context, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
   : inherited( context, parent, shareWidget, f, true )
{
   commonConstructor();
}


OSGWidget::OSGWidget( const QGLFormat& format, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
   : inherited( format, parent, shareWidget, f, true )
{
   commonConstructor();
}


void OSGWidget::commonConstructor()
{
   // members initialization
   defaultCursor = Qt::CrossCursor;
   movCursor = Qt::SizeAllCursor;
   zrotCursor = QPixmap( ":/images/arc_rotate.png" );
   rotCursor = QPixmap( ":/images/birot.png" );
   hrotCursor = QPixmap( ":/images/rotate_h.png" );
   vrotCursor = QPixmap( ":/images/rotate_v.png" );
   lookCursor = QPixmap( ":/images/look_cur.png" );

   // initialize view
   setCursor( defaultCursor );
}


void OSGWidget::initializeGL()
{
   // call parent
   inherited::initializeGL();

   // set focus on ourselves
   this->setFocus( Qt::OtherFocusReason );

   // verify that we have stencil buffer
   QGLFormat f = format();
   if( !f.stencil() || f.stencilBufferSize() < 1 )
      Log::warn() << "No stencil buffer available for the rendering window." << Log::endm;
}


/**
 * Destructor.
 */
OSGWidget::~OSGWidget()
{
}


/**
 * Mouse-press event handler.
 */
void OSGWidget::mousePressEvent( QMouseEvent *e )
{
   inherited::mousePressEvent( e );

   // set appropriate cursor
   if( Lexolights::viewer()->isOrbitManipulatorActive() )
   {
      if( e->button() == Qt::LeftButton ||
          e->button() == Qt::RightButton )
      {
         setCursor( rotCursor );
      }
      else if( e->button() == Qt::MidButton )
      {
         setCursor( movCursor );
      }
   }
   else if( Lexolights::viewer()->isFirstPersonManipulatorActive() )
   {
      if( e->button() == Qt::LeftButton ||
          e->button() == Qt::RightButton )
      {
         setCursor( lookCursor );
      }
      else if( e->button() == Qt::MidButton )
      {
         setCursor( lookCursor );
      }
   }
}


/**
 * Mouse-release event handler. Changes the cursor.
 */
void OSGWidget::mouseReleaseEvent( QMouseEvent *e )
{
   inherited::mouseReleaseEvent( e );

   // restore default cursor
   setCursor( defaultCursor );
}


/**
 * Mouse-move event handler.
 */
void OSGWidget::mouseMoveEvent( QMouseEvent *e )
{
   inherited::mouseMoveEvent( e );
}


/**
 * Mouse-wheel event handler.
 */
void OSGWidget::wheelEvent( QWheelEvent *e )
{
   inherited::wheelEvent( e );
}


void OSGWidget::keyPressEvent( QKeyEvent *event )
{
   inherited::keyPressEvent( event );
}


void OSGWidget::keyReleaseEvent( QKeyEvent *event )
{
   inherited::keyReleaseEvent( event );
}


#if defined(__WIN32__) || defined(_WIN32)

static void callWithContext( QGLWidget *glWidget, QString (*f)() )
{
   osgViewer::ViewerBase::ThreadingModel tm = Lexolights::viewer()->getThreadingModel();
   Lexolights::viewer()->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );

   glWidget->makeCurrent();
   Log::always() << f() << Log::endm;
   glWidget->doneCurrent();

   Lexolights::viewer()->setThreadingModel( tm );
}

#else

static void callFunction( void* data )
{
   typedef QString (*Callback)(void);
   Callback f = (Callback)data;
   Log::always() << (*f)() << Log::endm;
}

#endif


void OSGWidget::printOpenGLVersion()
{
/* We can switch to SingleThreaded model at Win32, activate the context, perform
 * the OpenGL calls/queries and set threading model back (tested on Win7 and Qt 4.6.3).
 *
 * However, this does not work on Linux (seen on Kubuntu 10.10, Qt 4.7.0).
 * Stopping and restarting of threads does not work correctly.
 * Seems that the problem is related to Qt. Valgrind reports this:
 *
 * Invalid read of size 8
 * ==8327==    at 0x979499F: _XReply (in /usr/lib/libX11.so.6.3.0)
 * ==8327==    by 0x9789CFC: XTranslateCoordinates (in /usr/lib/libX11.so.6.3.0)
 * ==8327==    by 0x72BA63D: QWidget::mapFromGlobal(QPoint const&) const (qwidget_x11.cpp:1327)
 * ==8327==    by 0x72BA6A1: QWidget::mapFromGlobal(QPoint const&) const (qwidget_x11.cpp:1320)
 * ==8327==    by 0x72BA6A1: QWidget::mapFromGlobal(QPoint const&) const (qwidget_x11.cpp:1320)
 * ==8327==    by 0x721D834: QApplicationPrivate::pickMouseReceiver(QWidget*, QPoint const&, QPoint&, QEvent::Type, QFlags<Qt::MouseButton>, QWidget*, QWidget*) (qapplication.cpp:2985)
 * ==8327==    by 0x72A22C1: QETWidget::translateMouseEvent(_XEvent const*) (qapplication_x11.cpp:4393)
 * ==8327==    by 0x72A0C5B: QApplication::x11ProcessEvent(_XEvent*) (qapplication_x11.cpp:3536)
 * ==8327==    by 0x72CD0E1: x11EventSourceDispatch(_GSource*, int (*)(void*), void*) (qguieventdispatcher_glib.cpp:146)
 * ==8327==    by 0xAFFC341: g_main_context_dispatch (in /lib/libglib-2.0.so.0.2600.1)
 *
 * and fails with assertion: ../../src/xcb_io.c:249: process_responses: Assertion `(((long) (dpy->last_request_read) - (long) (dpy->request)) <= 0)' failed.
 * Aborted
 *
 * Seems like OpenGL Qt implementation on X11 has problems with using multiple threads.
 */
#if defined(__WIN32__) || defined(_WIN32)
   callWithContext( this, &SysInfo::getOpenGLVersionInfo );
#else
   Lexolights::viewer()->appendOneTimeOpenGLCallback( &callFunction, (void*)&SysInfo::getOpenGLVersionInfo );
#endif
}


void OSGWidget::printOpenGLExtensions()
{
#if defined(__WIN32__) || defined(_WIN32)
   callWithContext( this, &SysInfo::getOpenGLExtensionsInfo );
#else
   Lexolights::viewer()->appendOneTimeOpenGLCallback( &callFunction, (void*)&SysInfo::getOpenGLExtensionsInfo );
#endif
}


void OSGWidget::printOpenGLLimits()
{
#if defined(__WIN32__) || defined(_WIN32)
   callWithContext( this, &SysInfo::getOpenGLLimitsInfo );
#else
   Lexolights::viewer()->appendOneTimeOpenGLCallback( &callFunction, (void*)&SysInfo::getOpenGLLimitsInfo );
#endif
}


void OSGWidget::printGLSLLimits()
{
#if defined(__WIN32__) || defined(_WIN32)
   callWithContext( this, &SysInfo::getGLSLLimitsInfo );
#else
   Lexolights::viewer()->appendOneTimeOpenGLCallback( &callFunction, (void*)&SysInfo::getGLSLLimitsInfo );
#endif
}


void OSGWidget::printGraphicsDriverInfo()
{
#if defined(__WIN32__) || defined(_WIN32)
   callWithContext( this, &SysInfo::getGraphicsDriverInfo );
#else
   Lexolights::viewer()->appendOneTimeOpenGLCallback( &callFunction, (void*)&SysInfo::getGraphicsDriverInfo );
#endif
}


void OSGWidget::printVideoMemoryInfo()
{
#if defined(__WIN32__) || defined(_WIN32)
   callWithContext( this, &SysInfo::getVideoMemoryInfo );
#else
   Lexolights::viewer()->appendOneTimeOpenGLCallback( &callFunction, (void*)&SysInfo::getVideoMemoryInfo );
#endif
}


static struct ScreenLogInfo {
   OSGWidget *osgWindow;
   bool wasLogShown;
   bool keepAtBottom;
   QSize sizeIfLog;
   QSize fullSize;
   int frameNum;
   ScreenLogInfo() : frameNum(0) {}
} screenLogInfo;

static void printScreenInfoDrawCallback( void *data );


// OSGWidget::printScreenInfo() is called from main thread
void OSGWidget::printScreenInfo()
{
   // avoid multiple simultaneous tasks
   if( screenLogInfo.frameNum != 0 )
      return;

   screenLogInfo.frameNum++;

   // record window info
   screenLogInfo.osgWindow = this;
   screenLogInfo.wasLogShown = Lexolights::mainWindow()->isLogShown();

   // is scrollbar at the bottom?
   screenLogInfo.keepAtBottom = Log::getWindow()->isScrolledDown();

   // record widget size
   if( screenLogInfo.wasLogShown )  screenLogInfo.sizeIfLog = screenLogInfo.osgWindow->size();
   else  screenLogInfo.fullSize = screenLogInfo.osgWindow->size();

   // invert log visibility
   Lexolights::mainWindow()->showLog( !screenLogInfo.wasLogShown );

   // process all events, especially log visibility change
   QCoreApplication::processEvents();

   // record widget size
   if( screenLogInfo.wasLogShown )  screenLogInfo.fullSize = screenLogInfo.osgWindow->size();
   else  screenLogInfo.sizeIfLog = screenLogInfo.osgWindow->size();

   // schedule callback
   Lexolights::viewer()->appendOneTimeOpenGLCallback( &printScreenInfoDrawCallback );
}


// printScreenInfoDrawCallback() is called from rendering thread
static void printScreenInfoDrawCallback( void *data )
{
   Log::always() << SysInfo::getScreenInfo( screenLogInfo.sizeIfLog, screenLogInfo.fullSize ) << Log::endm;

   class MyRoutine : public MainThreadRoutine {
      virtual void exec()
      {
         // restore original log window visibility
         Lexolights::mainWindow()->showLog( screenLogInfo.wasLogShown );

         // process all events, especially log visibility change
         QCoreApplication::processEvents();

         // scroll to bottom
         if( screenLogInfo.keepAtBottom )  Log::getWindow()->makeScrolledDown();

         // reset frameNum to allow further screen info requests
         screenLogInfo.frameNum = 0;
      }
   };
   (new MyRoutine)->post();
}
