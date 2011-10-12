#include "LexoanimMainWindow.h"


#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>

#include <osgViewer/GraphicsWindow>
#include <osg/ref_ptr>

#include "LexoanimQtApp.h"
#include "LexolightsDocument.h"
#include "CentralContainer.h"
#include "gui/LogWindow.h"
#include "utils/Log.h"
#include "lexoanim.h"
#include "CameraHomer.h"
#include "CadworkMotionModelInterface.h"


#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/WriteFile>
#include <osgShadow/ShadowedScene>
#include <osgShadow/ShadowMap>
#include "MainWindow.h"
#include "OSGWidget.h"
#include "CentralContainer.h"
#include "Lexolights.h"
#include "LexolightsDocument.h"
#include "CadworkViewer.h"
#include "lighting/PerPixelLighting.h"
#include "lighting/ShadowVolume.h"
#include "gui/CadworkOrbitManipulator.h"
#include "gui/LogWindow.h"
#include "utils/Log.h"
#include "utils/ViewLoadSave.h"
#include "utils/WinRegistry.h"
#include "threading/ExternalApplicationWorker.h"
#include <dtCore/Scene.h>
#include <dtCore/transform.h>

#include <osgShadow/ShadowedScene>
#include "lighting/ShadowVolume.h"
#include "utils/FindLightVisitor.h"
#include <dtCore/object.h>
#include <dtCore/refptr.h>

using namespace std;

//class FindLightVisitor;

LexoanimMainWindow::LexoanimMainWindow( QWidget* parent, Qt::WFlags flags, bool build)
   : MainWindow(parent, flags, false)
{
   if(build)
      buildGUI(false);
}

/**
 * Destructor.
 */
LexoanimMainWindow::~LexoanimMainWindow()
{
}

/**
 * The method builds the GUI items of MainWindow.
 */
void LexoanimMainWindow::buildGUI( bool buildGLWidget )
{
   // protect against multiple calls
   if( this->centralWidget() != NULL )
      return;
   // main window settings
   this->setWindowTitle( "Lexolights" );
   this->setWindowIcon( QIcon( ":/images/Lexolights.png" ));
   setAcceptDrops( true );

   // create actions
   this->createActions();

   // create menus
   this->createMenu();

   // create toolbars
   this->createToolbars();

   // create statusbar
   this->createStatusBar();
   
   //disable unimplemented actions
   actionStereo->setDisabled(true);

   // create central holder
   // note: QStackedWidget is not used here as it is resized too slowly
   _centralContainer = new CentralContainer( this );
   this->setCentralWidget( _centralContainer );

   //if( buildGLWidget )
   //{
   //   // create OSG widget
   //   // (this is done inside setStereoscopicRendering()
   //   // for both - stereo and non-stereo OSG widgets - they are created on-demand)
   //   DisplaySettings *ds = Lexolights::viewer()->getDisplaySettings();
   //   if( ds == NULL )  ds = DisplaySettings::instance();
   //   bool stereo = ds->getStereo() &&
   //               ds->getStereoMode() == DisplaySettings::QUAD_BUFFER;
   //   setStereoscopicRendering( stereo );
   //}



   // initialize shadow volumes
   //osgShadow::ShadowVolume::setupCamera( Lexolights::viewer()->getCamera() );

   // allocate log window
   // (visibility will be determined later in this method)
   Log::showWindow( this, this, SLOT( showLog(bool) ) );

   menuBar->setEnabled( true );

   // restore window settings
   // (geometry is related to the MainWindow position
   //  while state restores position and visibility of child widgets (LogWindow,...))
   QSettings settings;
   bool geometryOk = restoreGeometry( settings.value( "geometry" ).toByteArray() );
   bool stateOk = restoreState( settings.value( "windowState" ).toByteArray() );

   // log visibility
#ifdef NDEBUG
   // do not show log initially in release
   showLog( false );
#else
   // show log window in debug
   // if visibility not set by restoreState
   if( !stateOk )
      showLog( true );
#endif

   // set initial window size to 800x600
   if( !geometryOk )
      this->resize( 800, 600 );

   // make window initially maximized in release
#ifdef NDEBUG
   if( !geometryOk )
      this->showMaximized();
#endif

   // update GUI
   bool wasBlocked = this->actionShowLog->blockSignals( true );
   this->actionShowLog->setChecked( Log::isVisible() );
   this->actionShowLog->blockSignals( wasBlocked );

   // propagate menu settings
//   setBackgroundColor();

   // show the window
   this->layout()->update(); // this avoids OSG rendering to appear in a small rectangle
                             // during the first frame (seen on Qt 4.7.2)
   this->show();

   // set focus
   /*if( _centralContainer->activeWidget() )
      _centralContainer->activeWidget()->setFocus();
   else
      Log::warn() << "MainWindow::buildGUI() warning: There is no active widget in central holder\n"
                     "   to set focus on it." << std::endl;*/
}

//void LexoanimMainWindow::createOSGWindow()
//{
//   MainWindow::createOSGWindow();
//}

void LexoanimMainWindow::createActions()
{
   MainWindow::createActions();
}

void LexoanimMainWindow::createMenu()
{
   MainWindow::createMenu();
}

void LexoanimMainWindow::createToolbars()
{
   MainWindow::createToolbars();
}

void LexoanimMainWindow::createStatusBar()
{
   MainWindow::createStatusBar();
}

/**
 * The method loads the model from the file fileName.
 *
 * If parameter resetViwSettings is set to false,
 * camera position is set to its initial default view
 * and all the GUI settings are reset to default values as well (shadow mode,...).
 */
void LexoanimMainWindow::loadModel(QString fileName, bool resetViewSettings )
{
   // disable menu while loading
   this->menuBar->setDisabled( true );

   // redraw window to erase "open file" dialog from buffer
   if( _centralContainer->activeWidget() )
      _centralContainer->activeWidget()->repaint();

   // release previous scene from memory
   // to make it available for the new scene
   _activeDocument = NULL;
   //Lexolights::viewer()->setSceneData( NULL, false );
   ///Delta3D
   /** nefunguje spravne, pozdeji pridana scena se prida k te "odebrane" 
       Proto je nutne pozit jak dt3d RemoveAllDrawables(), tak i osg
       removeChildren(0, DeltaSceneGroup->getNumChildren()).
       Pokud bychom pouzili pouze osg, pak se nactena scena zobrazi s artefakty.
       Spatne shadovana (pixelove zrnita).
   */
   getDeltaApp()->GetScene()->RemoveAllDrawables();
   ref_ptr<osg::Group> DeltaSceneGroup = getDeltaApp()->GetScene()->GetSceneNode();
   DeltaSceneGroup->removeChildren(0, DeltaSceneGroup->getNumChildren());

   //~Delta3D
   bool ok = false;

   if( !fileName.isEmpty() ) {

      osg::ref_ptr< LexolightsDocument > newDocument = new LexolightsDocument;
      if( /*newDocument->openFile( fileName )*/ true)
      {
         // set new active document
         // and re-connect document's sceneChanged signal
         if( _activeDocument )
            disconnect( _activeDocument, SIGNAL( sceneChanged() ),
                        this, SLOT( activeDocumentSceneChanged() ) );
         _activeDocument = newDocument;
         if( _activeDocument )
            connect( _activeDocument, SIGNAL( sceneChanged() ),
                     this, SLOT( activeDocumentSceneChanged() ) );

         // re-enable PPL (Per-Pixel Lighting), if required
         // (if --no-conversion is specified, re-disable PPL)
         if( resetViewSettings ) {
            bool wasBlocked = actionPPL->blockSignals( true );
            actionPPL->setChecked( !Lexolights::options()->no_conversion );
            actionPPL->blockSignals( wasBlocked );
         }

         // set the new scene
         // and reset view if requested
         /*Lexolights::viewer()->setSceneData( actionPPL->isChecked() ?
                                                newDocument->getPPLScene() :
                                                newDocument->getOriginalScene(),
                                             resetViewSettings );*/
         ///Delta3D
         /*dtCore::RefPtr<dtCore::Object> sceneObject = new dtCore::Object("Model");
         sceneObject->LoadFile(fileName.toStdString());
         getDeltaApp()->AddDrawable(sceneObject);*/
         /*DeltaSceneGroup->addChild(actionPPL->isChecked() ?
                                                newDocument->getPPLScene() :
                                                newDocument->getOriginalScene());*/
         //DeltaSceneGroup->addChild(newDocument->getOriginalScene());
         DeltaSceneGroup = NULL;

         dtCore::RefPtr<dtCore::Object> obj = new dtCore::Object();
         obj->LoadFile(fileName.toStdString());

         //ref_ptr<osg::Node> sc = newDocument->getOriginalScene();
         ref_ptr<osg::Node> sc = obj->GetOSGNode();
         ref_ptr<osgShadow::ShadowedScene> shads = new osgShadow::ShadowedScene;
         ref_ptr<osgShadow::ShadowVolume> sv = new osgShadow::ShadowVolume();
         FindLightVisitor flv;
         sc->accept(flv);
         sv->setMethod( osgShadow::ShadowVolumeGeometryGenerator::ZFAIL );
         //sv->setMode(osgShadow::ShadowVolumeGeometryGenerator::CPU_SILHOUETTE);
         sv->setStencilImplementation( osgShadow::ShadowVolume::STENCIL_TWO_SIDED );
         sv->setShadowCastingFace( osgShadow::ShadowVolumeGeometryGenerator::BACK );
         sv->setUpdateStrategy( osgShadow::ShadowVolume::MANUAL_INVALIDATE );
         sv->setLight(flv.getLight());
         shads->setShadowTechnique( sv );
         shads->addChild(sc);
         //getDeltaApp()->GetScene()->SetSceneNode(DeltaSceneGroup);
         getDeltaApp()->GetScene()->SetSceneNode(shads);
         LexoanimApp *lexoAnimApp = dynamic_cast<LexoanimApp *> (getDeltaApp());
         if(lexoAnimApp)
         {
            /**
             * We should compute homing position for all motion models in lexoanimApp .
             * And maybe it would be better to have one shared Homer object, since the position
             * is always the same. So we could compute it only once.
             */
            dtCore::Camera *cam = lexoAnimApp->GetCamera();
            if(lexoAnimApp->GetFlyMotionModel()->GetAutoComputeHomePosition())
               lexoAnimApp->GetFlyMotionModel()->ComputeHomePosition(cam);

            if(lexoAnimApp->GetOrbitMotionModel()->GetAutoComputeHomePosition())
               lexoAnimApp->GetOrbitMotionModel()->ComputeHomePosition(cam);

            // Set position of camera to computed home position (for actualy used motion model)
            dtCore::CameraHomer  *comm = dynamic_cast<dtCore::CameraHomer  *> (lexoAnimApp->GetActualMotionModel());
            if(comm)
            {
               if(comm->GetAutoComputeHomePosition())
               {
                  comm->GoToHomePosition();
               }
            }
         }
         //~Delta3D
         //Sets focus back to rendering widget, so we can use keyboard without clicking into scene first
         if( _centralContainer->activeWidget() )
            _centralContainer->activeWidget()->setFocus();
         else
            Log::warn() << "MainWindow::buildGUI() warning: There is no active widget in central holder\n"
                     "   to set focus on it." << std::endl;
         ok = true;
      }
   }

   // reset camera position if requested and not already done
   if( resetViewSettings && !ok ){
      Lexolights::viewer()->setSceneData( NULL, true );
   }

   // hide message dialog and enable menu
#if 0 //FIXME: resurrect this
   this->message->hide();
#endif
   this->menuBar->setEnabled( true );

}


void LexoanimMainWindow::setDeltaApp(dtABC::Application *deltaApp){
   _deltaApp = deltaApp;
}

dtABC::Application *LexoanimMainWindow::getDeltaApp(){
   return _deltaApp;
}


/*******************SLOTS**********************/


/**
 * The reloadModel slot reloads the model from the file.
 * This is useful, for example, to perform a model update
 * upon the change of the file on disk.
 *
 * The model to reload is given by the file name of the active document.
 */
void LexoanimMainWindow::reloadModel()
{
   if( _activeDocument )
      loadModel( _activeDocument->getFileName(), false );
}

/**
 * Set the orbit manipulator.
 *
 */
void LexoanimMainWindow::setOrbitManipulator()
{
   LexoanimApp *app = dynamic_cast<LexoanimApp *>(getDeltaApp());
   if(app){
      app->SetActualCameraMotionModel(app->GetOrbitMotionModel());
      actionOrbitManip->setChecked( true );
   }
}


/**
 * Set the 1st person manipulator.
 *
 */
void LexoanimMainWindow::setFirstPersonManipulator()
{
   LexoanimApp *app = dynamic_cast<LexoanimApp *>(getDeltaApp());
   if(app){
      app->GetFlyMotionModel()->ReleaseMouse();
      app->SetActualCameraMotionModel(app->GetFlyMotionModel());
      actionFirstPersonManip->setChecked( true );
   }
}

/**
 * The slot sets the camera to its default view, e.g. to home possition
 * that is set properly and far enough to see all the objects of the scene.
 */
void LexoanimMainWindow::defaultView()
{
   LexoanimApp *app = GetLexoanimApp();
   if(app)
   {
      dtCore::Camera * cam = app->GetCamera();
      dtCore::CameraHomer *camHome = dynamic_cast<dtCore::CameraHomer *>(app->GetActualMotionModel());
      if(camHome)
      {
         camHome->ComputeHomePosition(cam);
         camHome->GoToHomePosition();
      }
   }
}

void LexoanimMainWindow::zoomAll()
{
   defaultView();
}

void LexoanimMainWindow::loadView()
{
   // get file name
   QString filename = QFileDialog::getOpenFileName(
         this,
         "Load View ...",          // caption
         QString::null,            // default dir
         "View (*.ivv)"            // filter
         );

   // handle empty filename
   if( filename.isEmpty() )
      return;
   osg::Matrix modelView;
   osg::Vec3 eye,center;
   double fovy;
   // read file
   int r = loadIVV( filename, /*modelView*/ eye, center, fovy );


   // init manipulator (this stops any camera animation, etc., but should not change its position)
   if( r > 0 ) {
      LexoanimApp *app = GetLexoanimApp();
      if(app)
      {
         dtCore::Camera *cam = app->GetCamera();
         double dummy,aspect,nearc,farc;
         app->GetCamera()->GetPerspectiveParams(dummy,aspect,nearc,farc);
         app->GetCamera()->SetPerspectiveParams(fovy,aspect,nearc,farc);
         CadworkMotionModelInterface *cadwmm = dynamic_cast<CadworkMotionModelInterface *> (app->GetActualMotionModel());
         if(cadwmm)
         {
            cadwmm->SetViewPosition( eye, center);
         }
         Log::notice() << "loadView: View settings successfully loaded from "
                       << filename << "." << Log::endm;
      }    
      else
         Log::fatal() << "loadView: Failed to load view settings from "
                      << filename << "." << Log::endm;
   }     
   else
      Log::fatal() << "loadView: Failed to load view settings from "
                   << filename << "." << Log::endm;
}

void LexoanimMainWindow::saveView()
{
   // get file name
   QString filename = QFileDialog::getSaveFileName(
         this,
         "Save View ...",        // caption
         QString::null,          // default dir
         "View (*.ivv)"          // filter
         );

   // handle empty filename
   if( filename.isEmpty() )
      return;

   // append extension if missing
   while( filename.endsWith( " " ) )  filename.truncate( filename.length()-1 );
   if( !filename.contains( "." ) )
      filename.append( ".ivv" );

   osg::Matrix modelView;
   double fovy, dummy;

         osg::Vec3 focal, eye;
   LexoanimApp *app = GetLexoanimApp();
   if(app)
   {
      app->GetCamera()->GetPerspectiveParams(fovy,dummy,dummy,dummy);
      dtCore::Transform trans;
      app->GetActualMotionModel()->GetTarget()->GetTransform(trans);
      trans.Get(modelView);
      dtCore::CadworkOrbitMotionModel * cmm = dynamic_cast<dtCore::CadworkOrbitMotionModel *> (app->GetActualMotionModel());
      trans.GetTranslation(eye);
      if(cmm)
      {
         focal = cmm->GetFocalPoint();
      }
   }
   else
   {
      Log::fatal() << "saveView: Can not get Delta3D Lexoanim Application object "
                   << filename << "." << Log::endm;
      return;
   }

   // write to file
   int r = saveIVV( filename, /*modelView*/eye,focal, fovy);

   // log
   if( r > 0 )
      Log::notice() << "saveView: View settings saved to "
                    << filename << " successfully." << Log::endm;
   else
      Log::fatal() << "saveView: Can not save view settings to "
                   << filename << "." << Log::endm;
}

void LexoanimMainWindow::setBackgroundColor()
{
   dtCore::Camera *camera = getDeltaApp()->GetCamera();

   if( this->actionBlack->isChecked() )
      camera->SetClearColor(  osg::Vec4( 0.0, 0.0, 0.0, 1.0 ) );

   if( this->actionDarkGrey->isChecked() )
      camera->SetClearColor(  osg::Vec4( 0.25, 0.25, 0.25, 1.0 ) );

   if( this->actionGrey->isChecked() )
      camera->SetClearColor(  osg::Vec4( 0.5, 0.5, 0.5, 1.0 ) );

   if( this->actionLightGrey->isChecked() )
      camera->SetClearColor(  osg::Vec4( 0.75, 0.75, 0.75, 1.0 ) );

   if( this->actionWhite->isChecked() )
      camera->SetClearColor(  osg::Vec4( 1.0, 1.0, 1.0, 1.0 ) );

   if( this->actionGriseous->isChecked() )
      camera->SetClearColor(  osg::Vec4( 0.4, 0.4, 0.6, 1.0 ) );

   if( this->actionTan->isChecked() )
      camera->SetClearColor(  osg::Vec4( 0.7578125, 0.7265625, 0.5859375, 1.0 ) );

   if( this->actionCustomColor->isChecked() )
      camera->SetClearColor(  _customColor );
}

/**
 * Opens color-dialog for to select custom background color.
 */
void LexoanimMainWindow::selectCustomColor()
{
   dtCore::Camera *camera = getDeltaApp()->GetCamera();
   // get color from dialog
   QColor c = QColorDialog::getColor( QColor( (int)( _customColor.r() * 255.0f ),
                                              (int)( _customColor.g() * 255.0f ),
                                              (int)( _customColor.b() * 255.0f ),
                                              (int)( _customColor.a() * 255.0f ) ),
                                      NULL );
   // store selected color
   if( c.isValid() )
      _customColor.set( c.red() / 255.0f,
                        c.green() / 255.0f,
                        c.blue() / 255.0f,
                        1.0 );

   // set background color
   this->actionCustomColor->setChecked( true );
   camera->SetClearColor( _customColor );
}


void LexoanimMainWindow::renderUsingPovray()
{
   // if no active document, do nothing
   if( !_activeDocument ) {
      Log::notice() << "No active document. Can not export to POV-Ray." << endl;
      return;
   }

   //
   //  Detect whether POV-Ray is installed.
   //
   //  This is implemented on Windows by looking inside registry for POV-Ray path.
   //
#if defined(__WIN32__) || defined(_WIN32)

   // find POV-Ray executable path in registry
   QString exePath = WinRegistry::getString( HKEY_CURRENT_USER,
                                             "Software\\POV-Ray\\CurrentVersion\\Windows",
                                             "Home" ).c_str();
   if( !exePath.isEmpty() )
      Log::info() << "Found POV-Ray path in registry: " << exePath << Log::endm;
   else {
      Log::notice() << "Can not find POV-Ray path in windows registry.\n"
                       "Its path is expected to be in the following key:\n"
                       "HKEY_CURRENT_USER\\Software\\POV-Ray\\CurrentVersion\\Windows "
                       "in Home value." << Log::endm;
      Log::fatal() << "POV-Ray not installed." << Log::endm;
      return;
   }
   if( exePath[exePath.length()-1] != '\\' )
      exePath += '\\';

#endif


   //
   //
   struct ReturnHandler {
      bool restoreGUI;
      LexoanimMainWindow * mainWindow;
      ReturnHandler(LexoanimMainWindow * mw) : restoreGUI( true ), mainWindow(mw)
      {
         // update GUI
#if 0 // no image widget, use external window instead
         Lexolights::mainWindow()->getImageWidget()->setText( "POV-Ray rendering in progress..." );
         Lexolights::mainWindow()->setActiveCentralWidget( Lexolights::mainWindow()->getImageWidget() );
#endif
         //Lexolights::mainWindow()->actionPovrayRendering->setEnabled( false );
         mainWindow->actionPovrayRendering->setEnabled( false );

         // process all messages and events
         QCoreApplication::sendPostedEvents();
         QApplication::processEvents();
      }
      ~ReturnHandler()
      {
         // restore GUI
         // (If the POV-Ray thread is not started because of, for example, fail of scene-to-povray export,
         // GUI would not be restored as there is no POV-Ray thread that would restore
         // GUI when it finishes its work. Thus, following code is used.)
         if( restoreGUI )
         {
            // restore GUI
            mainWindow->switchToLastGLWidget();
            mainWindow->actionPovrayRendering->setEnabled( true );
         }
      }
   } returnHandler(this);


   //
   //  Export POV file
   //

   // paths
   osg::Timer time;
   std::string fileName( osgDB::getNameLessExtension( _activeDocument->getFileName().toLocal8Bit().data() ) );
   std::string filePath( osgDB::getFilePath( fileName ) );
   fileName += ".pov";
   std::string simpleFileName( osgDB::getSimpleFileName( fileName ) ); // no path, includes pov extension
   QDir pathDir( QString( filePath.c_str() ) );
   pathDir.mkdir( "povray" );
   if( pathDir.cd( "povray" ) ) {
      // update paths
      fileName = pathDir.filePath( QString( simpleFileName.c_str() ) ).toLocal8Bit().data();
      filePath = osgDB::getFilePath( fileName );
   } else {
      // restore pathDir
      pathDir = QString( filePath.c_str() );
   }

   // export scene
   Log::info() << "Exporting scene to POV-Ray file (" << fileName << ")..." << Log::endm;
   osg::ref_ptr< osg::Camera > camera = dynamic_cast< osg::Camera* >(
      getDeltaApp()->GetCamera()->GetOSGCamera()->clone( osg::CopyOp::SHALLOW_COPY ) );


   camera->removeChild( 0, camera->getNumChildren() );
   camera->addChild( _activeDocument->getOriginalScene() );
   osg::ref_ptr< osgDB::Options > options = new osgDB::Options( "CopyFiles" );
   std::string modelDir = osgDB::getFilePath( _activeDocument->getFileName().toLocal8Bit().data() );
   if( !modelDir.empty() )
      options->getDatabasePathList().push_back( modelDir );
   else
      Log::warn() << "Render using POV-Ray warning: Can not get model directory." << endl;
   bool r = osgDB::writeNodeFile( *camera, fileName, options );

   //bool r = osgDB::writeNodeFile( *(getDeltaApp()->GetScene()->GetSceneNode()), fileName, options );
   camera = NULL;
   double dt = time.time_m();
   if( r )  Log::notice() << QString( "POV-Ray file %1 successfully written in %2ms" )
                                     .arg( fileName.c_str() ).arg( dt, 0, 'f', 2 ) << Log::endm;
   else {
      Log::fatal() << "Can not export POV-Ray file " << fileName << Log::endm;
      return;
   }


   //
   //  Write ini file
   //
   std::stringstream iniStream;
   float scale = povScale->value() == 0. ? 0.01 : povScale->value();
   iniStream << "Width = " << _centralContainer->width() * scale << endl;
   iniStream << "Height = " << _centralContainer->height() * scale << endl;
   iniStream << "Display = Off" << endl;
   iniStream << "Verbose = On" << endl;
   iniStream << "Output_to_File = true" << endl;
   iniStream << "Output_File_Type = N8       ; PNG 8 bits per color (range 5..16)" << endl;
   iniStream << endl;

   // tracing options are desctibed, for example, at
   // http://www.povray.org/documentation/view/3.6.2/223/
   //
   // performance notes:
   // Küche model (39'000 triangles, 1086x573) renders about 7:45 without antialiasing,
   // about 11:30 with +AM2 threshold=0.09 depth=3
   iniStream << "Quality = 11                ; range 0..11, 8 reflections, 9..11 compute media and radiosity" << endl;
   iniStream << "Antialias = on" << endl;
   iniStream << "Sampling_Method = 1         ; supersampling method: 1-non-adaptive, 2-adaptive" << endl;
   iniStream << "Antialias_Threshold = " << ( povFastAntialias->isChecked() ?
                   "0.3   ; 0.3 = 3 x 0.1, e.g. 10% for each of RGB component (allowed range is 0..3)" :
                   "0.09  ; 0.09 = 3 x 0.03, e.g. 3% for each of RGB component (allowed range is 0..3)" ) << endl;
   iniStream << "Antialias_Depth = 3         ; for method 1: 1 means 1 (1x1), 2 means 4 (2x2), 3 means 9 (3x3),..." << endl;
   iniStream << "                            ; for method 2: 0 means 4 (=2x2) supersamples for method 2, 1 means 4 to 9 samples (2x2..3x3), 2 means 4..25 (2x2..5x5), 3 means 4..81 (up to 9x9)" << endl;
   iniStream << "Jitter = on                 ; adds sampling noise that reduces aliasing" << endl;
   iniStream << "Jitter_Amount = 1.0         ; recommended range 0..1" << endl;

   // Following line is commented out as we do not want to pause.
   // Rather we want to show the result in Lexolights window as soon as povray terminates.
   // iniStream << "+P ; pause on Linux after the rendering (e.g. Window will not disappear)" << endl;
   QFile ini( pathDir.filePath( "povray.ini" ) );
   if( ini.open( QIODevice::WriteOnly ) )
   {
      ini.write( iniStream.str().c_str() );
      ini.close();
   } else
      Log::warn() << "Failed to write povray.ini." << Log::endm;


   //
   //  Prepare arguments and start POV-Ray
   //
   class PovrayWorker : public ExternalApplicationWorker {
   public:
      PovrayWorker( const QString& program, const QStringList& arguments,
                    const QString& workingDirectory, const QString& renderedImageFile, LexoanimMainWindow * mw )
         : ExternalApplicationWorker( program, arguments, workingDirectory, true, true ), mainWindow(mw),
           _renderedImageFile( renderedImageFile )
      {
         // remove old image
         if( QFile::exists( _renderedImageFile ) )
         {
            bool r = QFile::remove( _renderedImageFile );
            if( r )
               Log::info() << "PovRayWorker: Successfully removed recently rendered file." << endl;
            else
               Log::warn() << "PovRayWorker: Failed to remove recently rendered file." << endl;
         }
      }
      virtual void done()
      {
         mainWindow->actionPovrayRendering->setEnabled( true );

         if( _exitStatus == QProcess::NormalExit && _exitCode == 0 )
         {
            QPixmap p( _renderedImageFile );
            if( p.isNull() )
            {
               if( QFile::exists( _renderedImageFile ) )
                  Log::warn() << "PovrayWorker: failed to read generated image ("
                              << _renderedImageFile << ")." << endl;
               else
                  Log::warn() << "POV-Ray failed to render the image." << endl;
            }

#if 0 // no image widget, use external window instead
            Lexolights::mainWindow()->getImageWidget()->setPixmap( p );
#else
            QMainWindow *povWindow = new QMainWindow;
            povWindow->setWindowTitle( "POV-Ray" );
            QLabel *image = new QLabel( povWindow );
            image->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
            povWindow->setCentralWidget( image );
            if( !p.isNull() )
            {
               image->setPixmap( p );
               povWindow->setFixedSize( p.size() );
            }
            else
            {
               image->setText( "POV-Ray failed to generate the image." );
            }
            povWindow->show();
#endif
         }

         if( _exitStatus == QProcess::CrashExit )
         {
            Log::fatal() << "POV-Ray process failed to start or crashed.\n"
                            "POV-Ray may not be installed or is not configured properly." << Log::endm;
            mainWindow->switchToLastGLWidget();
         }
      }
   protected:
      QString _renderedImageFile;
      LexoanimMainWindow * mainWindow;
   };
#if defined(__WIN32__) || defined(_WIN32)

   // prepare argument list (fileName only as width, height, output format and antialiasing went to povray.ini file)
   // note: With /EXIT option, there must be no other POV-Ray instance running to successfully start rendering.
   //       If using /EXIT, /NORESTORE is useful to avoid loading of recently opened files (this is just waste of time),
   //       but it causes POV-Ray to forget on recently opened files and when the user starts POV-Ray manually,
   //       he gets just empty session.
   //       /RENDER is nice as it allows staring rendering even if another POV-Ray instance is running,
   //       while it automatically reads povray.ini. However, it is not possible to automatically close
   //       the POV-Ray after the rendering and starting while another instance is running will produce
   //       a message about "Keep Single Instance Feature".
   QStringList params;
   params << "/EXIT" << simpleFileName.c_str() << "povray.ini";

   // start povray
   QString executable( exePath + "bin\\pvengine64.exe" );
   if( !QFile::exists( executable ) )
      executable = exePath + "bin\\pvengine.exe";
   ( new PovrayWorker( executable, params, QString( filePath.c_str() ), ( filePath + "/" + osgDB::getNameLessExtension( simpleFileName ) + ".png" ).c_str() , this) )->start();

   // fix name for log
   if( executable.contains( ' ' ) )
      executable = '\"' + executable + '\"';

#else

   // prepare argument list (fileName only as width, height, output format, antialiasing and wait after rendering went to povray.ini file)
   QStringList params;
   params << simpleFileName.c_str();

   // start povray
   QString executable( "povray" );
   ( new PovrayWorker( executable, params, QString( filePath.c_str() ), ( filePath + "/" + osgDB::getNameLessExtension( simpleFileName ) + ".png" ).c_str() ) )->start();

#endif

   // do not restore GUI if everything went ok and we reached this point
   returnHandler.restoreGUI = false;


   //
   //  Log message
   //
   Log::notice() << "POV-Ray process start scheduled. Used command:\n   " << executable;
   for( int i=0, c=params.size(); i<c; i++ )
      if( params[i].contains( ' ' ) )
         Log::notice() << " \"" << params[i] << "\" ";
      else
         Log::notice() << " " << params[i] << " ";
   Log::notice() << "\n  while setting current directory to: " << filePath << Log::endm;
}

