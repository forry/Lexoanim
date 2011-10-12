#include "LexoanimLogWindow.h"
#include "lexoanim.h"
#include "LexoanimMainWindow.h"
#include "utils/SysInfo.h"

#include <QComboBox>
#include <osg/Camera>
#include <osg/Vec3>
#include <osg/io_utils>

LexoanimLogWindow::LexoanimLogWindow( QWidget *parent, Qt::WFlags flags )
   :LogWindow(parent,flags)
{

}

/**
 * Destructor.
 */
LexoanimLogWindow::~LexoanimLogWindow()
{
}

/** On Show button */
void LexoanimLogWindow::printSomethingCB()
{
   LexoanimMainWindow *mw = dynamic_cast<LexoanimMainWindow *> (parent());
   if(mw)
   {
      LexoanimApp *app = dynamic_cast<LexoanimApp *> (mw->getDeltaApp());
      if(app)
      {
         /*OSGWidget *osgWidget = dynamic_cast< OSGWidget* >( Lexolights::mainWindow()->getGLWidget() );
         if( !osgWidget ) {
            Log::warn() << "LogWindow warning: can not print info as OpenGL widget is not osg OSGWidget type." << std::endl;
            return;
         }*/

         switch( chooseSomething->currentIndex() ) {
            case 0: printOpenGLVersion();
                    break;
            case 1: printOpenGLExtensions();
                    break;
            case 2: printOpenGLLimits();
                    break;
            case 3: printGLSLLimits();
                    break;
            case 4: printGraphicsDriverInfo();
                    break;
            case 5: printVideoMemoryInfo();
                    break;
            case 6: //osgWidget->printScreenInfo();
                    break;
            case 7: Log::always() << SysInfo::getLibInfo() << Log::endm;
                    break;
            case 8: {
                       osg::Camera *camera = app->GetCamera()->GetOSGCamera();
                       if( !camera )
                          Log::always() << "Camera is NULL." << Log::endm;
                       else {
                          osg::Vec3 eye, center, up;
                          double fovy, tmp, zNear, zFar;
                          camera->getViewMatrixAsLookAt( eye, center, up );
                             camera->getProjectionMatrixAsPerspective( fovy, tmp, zNear, zFar );
                          Log::always() << "Camera view data:\n"
                                           "   Position:  " << eye << "\n"
                                           "   Direction: " << (center-eye) << "\n"
                                           "   Up vector: " << up << "\n"
                                           "   FOV (in vertical direction): " << fovy << "\n"
                                           "   zNear,zFar: " << zNear << "," << zFar << Log::endm;
                       }
                    }
                    break;
         };
      }
   }

   
}

void LexoanimLogWindow::printOpenGLVersion()
{
   Log::always()<<SysInfo::getOpenGLVersionInfo()<<Log::endm;
}

void LexoanimLogWindow::printOpenGLExtensions()
{
   Log::always()<<SysInfo::getOpenGLExtensionsInfo()<<Log::endm;
}

void LexoanimLogWindow::printOpenGLLimits()
{
   Log::always()<<SysInfo::getOpenGLLimitsInfo()<<Log::endm;
}

void LexoanimLogWindow::printGLSLLimits()
{
   Log::always()<<SysInfo::getGLSLLimitsInfo()<<Log::endm;
}

void LexoanimLogWindow::printGraphicsDriverInfo()
{
   Log::always()<<SysInfo::getGraphicsDriverInfo()<<Log::endm;
}

void LexoanimLogWindow::printVideoMemoryInfo()
{
   Log::always()<<SysInfo::getVideoMemoryInfo()<<Log::endm;
}