
/**
 * @file
 * Lexolights application's main().
 *
 * @author PCJohn
 */

#include <QTextCodec>
#include <osgQt/GraphicsWindowQt>
//#include "CadworkViewer.h"
#include "utils/Log.h"

#include <dtABC/application.h>
#include <dtQt/qtguiwindowsystemwrapper.h>
#include <QtOpenGL/QGLWidget>
#include <dtQt/osggraphicswindowqt.h>
#include <dtCore/deltawin.h>
#include <Qt/qmainwindow.h>
#include <dtCore/scene.h>
#include <dtCore/system.h>
#include <dtQt/deltastepper.h>

#include "LexoanimQtApp.h"
#include "gui/LexoanimMainWindow.h"
#include "Lexoanim.h"


// redirect osg::notify to our log window
LOG_NOTIFY_REDIRECT_PROXY();

// set windowing system to Qt
USE_GRAPICSWINDOW_IMPLEMENTATION(Qt);

/**
 * Application's entry point.
 *
 * @param argc Number of parameters on the commandline.
 * @param argv Array that contains parameters passed from the commandline.
 *
 * @return QApplication::exec() result.
 */
int main(int argc, char* argv[])
{
   // Log application start
   osg::DisplaySettings *display = osg::DisplaySettings::instance();
   display->setMinimumNumStencilBits(8);
   display->setNumMultiSamples(8);

   Log::spawnTimeMsg( "START_TIME", "Application spawn completed in %1ms" );
   Log::startMsg();

   // set encoding of char* based strings used by the application
   QTextCodec::setCodecForCStrings( QTextCodec::codecForName( "UTF-8" ) );

   // Application object
   LexoanimQtApp lexoanimQtApp( argc, argv );

   // set viewer used by Qt main loop
   //osgQt::setViewer( Lexolights::viewer() );

   // realize window
   //Lexolights::realize();

   LexoanimMainWindow mainWin(NULL, NULL, true);

   dtQt::QtGuiWindowSystemWrapper::EnableQtGUIWrapper();
   dtCore::RefPtr<dtABC::Application> app = new LexoanimApp("neco.xml");
   app->Config();

   mainWin.setDeltaApp(app);


   // Run GUI
   // (note: on some platforms, there is no guarantee that exec will return;
   // Particularly, when logging off at Windows, application is terminated after closing
   // all top-level windows.)
   dtQt::OSGGraphicsWindowQt* osgGraphWindow = dynamic_cast<dtQt::OSGGraphicsWindowQt*>(app->GetWindow()->GetOsgViewerGraphicsWindow());
   osgGraphWindow->makeCurrent();
   osgGraphWindow->detectOpenGLCapabilities();
   osgGraphWindow->getState()->setGraphicsContext( NULL );
   osgGraphWindow->getState()->setGraphicsContext( osgGraphWindow );
   osgGraphWindow->getState()->init();
   osgGraphWindow->releaseContext();
   QWidget* glWidget = osgGraphWindow->GetQGLWidget();
   mainWin.setActiveCentralWidget(glWidget);

   //lexoanimQtApp.mainWindow()->setActiveCentralWidget(glWidget);
   glWidget->setGeometry( 0 , 0, glWidget->width(), glWidget->height());
   glWidget->setFocus();


   //mainWin.show();

   dtCore::System::GetInstance().Start();
   dtQt::DeltaStepper stepper;
   stepper.Start();

   //qapp.exec();
   lexoanimQtApp.exec();

   stepper.Stop();
   dtCore::System::GetInstance().Stop();

   QTextCodec::setCodecForCStrings( NULL );

   // return error code
   return 0;
}
