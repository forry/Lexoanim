
/**
 * @file
 * MainWindow class implementation.
 *
 * @author Tomáš Pafčo
 *         PCJohn (Jan Pečiva)
 */


#include <assert.h>
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
#include "gui/AboutDialog.h"
#include "gui/SceneInfoDialog.h"
#include "gui/SystemInfoDialog.h"
#include "threading/ExternalApplicationWorker.h"
#include "utils/Log.h"
#include "utils/ViewLoadSave.h"
#include "utils/WinRegistry.h"
#include "utils/BuildTime.h"


using namespace std;
using namespace osg;


/**
 * Constructor.
 *
 * @param parent Parent widget.
 * @param flags  Window flags (Qt::WindowFlags).
 */
MainWindow::MainWindow( QWidget *parent, Qt::WFlags flags, bool build ) :
      QMainWindow( parent, flags ),
      _glWidget( NULL ),
      _ownsGLWidget( false ),
      _glStereoWidget( NULL ),
      _ownsGLStereoWidget( false ),
      _customColor( Vec4( 0.0, 0.0, 0.0, 1.0 ) )
{
   // create GUI
   if( build )
      buildGUI();
}


/**
 * Destructor.
 */
MainWindow::~MainWindow()
{
}


/**
 * The method builds the GUI items of MainWindow.
 */
void MainWindow::buildGUI( bool buildGLWidget )
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

   // create central holder
   // note: QStackedWidget is not used here as it is resized too slowly
   _centralContainer = new CentralContainer( this );
   this->setCentralWidget( _centralContainer );

   if( buildGLWidget )
   {
      // create OSG widget
      // (this is done inside setStereoscopicRendering()
      // for both - stereo and non-stereo OSG widgets - they are created on-demand)
      DisplaySettings *ds = Lexolights::viewer()->getDisplaySettings();
      if( ds == NULL )  ds = DisplaySettings::instance();
      bool stereo = ds->getStereo() &&
                  ds->getStereoMode() == DisplaySettings::QUAD_BUFFER;
      setStereoscopicRendering( stereo );
   }

   // Image widget
#if 0 // no image widget used for POV-Ray now, external window is used instead
   class ImageWidget : public QLabel {
      typedef QLabel inherited;
   public:
      ImageWidget( QWidget *parent ) : QLabel( parent ) {}
      virtual void mousePressEvent( QMouseEvent *ev )
      {
         inherited::mousePressEvent( ev );
         Lexolights::mainWindow()->switchToLastOSGWidget();
      }
   };
   _imageWidget = new ImageWidget;
   _imageWidget->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
   _centralContainer->addWidget( _imageWidget );
#endif

   // initialize shadow volumes
   osgShadow::ShadowVolume::setupCamera( Lexolights::viewer()->getCamera() );

   // allocate log window
   // (visibility will be determined later in this method)
   Log::showWindow( this, this, SLOT( showLog(bool) ) );

#if 0
   // create "loading" message dialog
   this->message = new CMsgDialog(this);
#endif

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
   setBackgroundColor();

   // show the window
   this->layout()->update(); // this avoids OSG rendering to appear in a small rectangle
                             // during the first frame (seen on Qt 4.7.2)
   this->show();

   // set focus
   if( _centralContainer->activeWidget() )
      _centralContainer->activeWidget()->setFocus();
   else
      Log::warn() << "MainWindow::buildGUI() warning: There is no active widget in central holder\n"
                     "   to set focus on it." << std::endl;
}


/**
 * Creates all actions and signal-slot connections.
 */
void MainWindow::createActions()
{
   // File->Open...
   this->actionOpenModel = new QAction(this);
   this->actionOpenModel->setText("&Open");
   this->actionOpenModel->setIcon(QIcon(":/images/open.xpm"));
   this->actionOpenModel->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
   connect(this->actionOpenModel, SIGNAL(triggered()), this, SLOT(openModel()));

   // File->Quit
   this->actionQuit = new QAction(this);
   this->actionQuit->setText("&Quit");
   connect(this->actionQuit, SIGNAL(triggered()), this, SLOT(close()));

   // View->Reload model from file
   this->actionReloadModel = new QAction(this);
   this->actionReloadModel->setText("Reload model");
   this->actionReloadModel->setIcon( QIcon( ":/images/reload.xpm" ) );
   this->actionReloadModel->setShortcut( Qt::Key_F5 );
   connect(this->actionReloadModel, SIGNAL(triggered()), this, SLOT(reloadModel()));

   // default view
   this->actionDefaultView = new QAction( this );
   this->actionDefaultView->setText( "Default view" );
   this->actionDefaultView->setIcon( QIcon( ":/images/default_view.png" ) );
   connect( this->actionDefaultView, SIGNAL( triggered() ), this, SLOT( defaultView() ) );

   // load view from XML file
   this->actionLoadView = new QAction(this);
   this->actionLoadView->setText("Load view");
   this->actionLoadView->setIcon(QIcon(":/images/load_view.png"));
   connect(this->actionLoadView, SIGNAL(triggered()), this, SLOT(loadView()));

   // save view to XML file
   this->actionSaveView = new QAction(this);
   this->actionSaveView->setText("Save view");
   this->actionSaveView->setIcon(QIcon(":/images/save_view.png"));
   connect(this->actionSaveView, SIGNAL(triggered()), this, SLOT(saveView()));

   // zoom all
   this->actionZoomAll = new QAction( this );
   this->actionZoomAll->setText( "Zoom all" );
   this->actionZoomAll->setIcon( QIcon( ":/images/zoom_all.xpm" ) );
   connect( this->actionZoomAll, SIGNAL( triggered() ), this, SLOT( zoomAll() ) );

   // View->Background group
   this->backgroundActionGroup = new QActionGroup(this);

      // View->Background->Black
      this->actionBlack = new QAction(this);
      this->actionBlack->setText("Black");
      this->actionBlack->setCheckable(true);
      this->actionBlack->setChecked(false);
      this->backgroundActionGroup->addAction(this->actionBlack);
      connect(this->actionBlack, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->Dark grey
      this->actionDarkGrey = new QAction(this);
      this->actionDarkGrey->setText("Dark grey");
      this->actionDarkGrey->setCheckable(true);
      this->actionDarkGrey->setChecked(false);
      this->backgroundActionGroup->addAction(this->actionDarkGrey);
      connect(this->actionDarkGrey, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->Grey
      this->actionGrey = new QAction(this);
      this->actionGrey->setText("Grey");
      this->actionGrey->setCheckable(true);
      this->actionGrey->setChecked(false);
      this->backgroundActionGroup->addAction(this->actionGrey);
      connect(this->actionGrey, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->Light grey
      this->actionLightGrey = new QAction(this);
      this->actionLightGrey->setText("Light grey");
      this->actionLightGrey->setCheckable(true);
      this->actionLightGrey->setChecked(false);
      this->backgroundActionGroup->addAction(this->actionLightGrey);
      connect(this->actionLightGrey, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->White
      this->actionWhite = new QAction(this);
      this->actionWhite->setText("White");
      this->actionWhite->setCheckable(true);
      this->actionWhite->setChecked(false);
      this->backgroundActionGroup->addAction(this->actionWhite);
      connect(this->actionWhite, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->Griseous
      this->actionGriseous = new QAction(this);
      this->actionGriseous->setText("Griseous");
      this->actionGriseous->setCheckable(true);
      this->actionGriseous->setChecked(true);
      this->backgroundActionGroup->addAction(this->actionGriseous);
      connect(this->actionGriseous, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->Tan
      this->actionTan = new QAction(this);
      this->actionTan->setText("Tan");
      this->actionTan->setCheckable(true);
      this->actionTan->setChecked(false);
      this->backgroundActionGroup->addAction(this->actionTan);
      connect(this->actionTan, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->Custom color
      this->actionCustomColor = new QAction(this);
      this->actionCustomColor->setText("Custom color");
      this->actionCustomColor->setCheckable(true);
      this->actionCustomColor->setChecked(false);
      this->backgroundActionGroup->addAction(this->actionCustomColor);
      connect(this->actionCustomColor, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

      // View->Background->Select custom color...
      this->actionSelectCustomColor = new QAction(this);
      this->actionSelectCustomColor->setText("Select custom color...");
      connect(this->actionSelectCustomColor, SIGNAL(triggered()), this, SLOT(selectCustomColor()));

      // View->Background->Background image...
      this->actionBackgroundImage = new QAction(this);
      this->actionBackgroundImage->setText("Background image...");
      this->actionBackgroundImage->setCheckable(TRUE);
      connect(this->actionBackgroundImage, SIGNAL(toggled(bool)), this, SLOT(backgroundImage(bool)));
      this->actionBackgroundImage->setDisabled(true);

   // View->Window size
   this->windowSizeActionGroup = new QActionGroup( this );

      // Full HD
      this->actionWindowSize1920x1080 = new QAction( this );
      this->actionWindowSize1920x1080->setText( "1920x1080" );
      this->windowSizeActionGroup->addAction( this->actionWindowSize1920x1080 );
      connect( this->actionWindowSize1920x1080, SIGNAL( triggered() ), this, SLOT( setCentralWidgetSize() ) );

      // 1600x900
      this->actionWindowSize1600x900 = new QAction( this );
      this->actionWindowSize1600x900->setText( "1600x900" );
      this->windowSizeActionGroup->addAction( this->actionWindowSize1600x900 );
      connect( this->actionWindowSize1600x900, SIGNAL( triggered() ), this, SLOT( setCentralWidgetSize() ) );

      // 1366x768
      this->actionWindowSize1366x768 = new QAction(this);
      this->actionWindowSize1366x768->setText( "1366x768" );
      this->windowSizeActionGroup->addAction(this->actionWindowSize1366x768);
      connect(this->actionWindowSize1366x768, SIGNAL(triggered()), this, SLOT( setCentralWidgetSize() ) );

      // SXGA
      this->actionWindowSize1280x1024 = new QAction(this);
      this->actionWindowSize1280x1024->setText( "1280x1024" );
      this->windowSizeActionGroup->addAction(this->actionWindowSize1280x1024);
      connect(this->actionWindowSize1280x1024, SIGNAL(triggered()), this, SLOT( setCentralWidgetSize() ) );

      // HD
      this->actionWindowSize1280x720 = new QAction(this);
      this->actionWindowSize1280x720->setText( "1280x720" );
      this->windowSizeActionGroup->addAction(this->actionWindowSize1280x720);
      connect(this->actionWindowSize1280x720, SIGNAL(triggered()), this, SLOT( setCentralWidgetSize() ) );

      // XGA
      this->actionWindowSize1024x768 = new QAction( this );
      this->actionWindowSize1024x768->setText( "1024x768" );
      this->windowSizeActionGroup->addAction( this->actionWindowSize1024x768 );
      connect( this->actionWindowSize1024x768, SIGNAL( triggered() ), this, SLOT( setCentralWidgetSize() ) );

      // SVGA
      this->actionWindowSize800x600 = new QAction( this );
      this->actionWindowSize800x600->setText( "800x600" );
      this->windowSizeActionGroup->addAction( this->actionWindowSize800x600 );
      connect( this->actionWindowSize800x600, SIGNAL( triggered() ), this, SLOT( setCentralWidgetSize() ) );

   // Scene->Shadow mode
   // (it is implemented as actionPPL (Per-Pixel Lighting), but it uses Shadow mode text
   // in GUI as it is more understandable by non-IT users)
   this->actionPPL = new QAction( this );
   this->actionPPL->setText( "Shadow mode" );
   this->actionPPL->setIcon( QIcon( ":/images/shadow.xpm" ) );
   this->actionPPL->setCheckable( true );
   this->actionPPL->setChecked( true );
   connect( this->actionPPL, SIGNAL( triggered(bool) ), this, SLOT( setPerPixelLighting(bool) ) );

   // Scene->Povray rendering
   actionPovrayRendering = new QAction( this );
   actionPovrayRendering->setText( "POV-Ray rendering" );
   this->actionPovrayRendering->setIcon( QIcon( ":/images/povray.png" ) );
   connect( actionPovrayRendering, SIGNAL( triggered() ), this, SLOT( renderUsingPovray() ) );

   // Scene->Stereoscopic view
   actionStereo = new QAction( this );
   actionStereo->setText( "Stereoscopic view" );
   actionStereo->setCheckable( true );
   connect( actionStereo, SIGNAL( triggered(bool) ), this, SLOT( setStereoscopicRendering(bool) ) );

   // Help->About
   this->actionAbout = new QAction( this );
   this->actionAbout->setText( "About" );
   this->actionAbout->setIcon( QIcon( ":/images/Lexolights.png" ) );
   connect( this->actionAbout, SIGNAL( triggered() ), this, SLOT( showAboutDlg() ) );

   // Help->Scene Info
   this->actionShowSceneInfo = new QAction( this );
   this->actionShowSceneInfo->setText( "Scene Info" );
   connect( this->actionShowSceneInfo, SIGNAL( triggered() ), this, SLOT( showSceneInfo() ) );

   // Help->System Info
   this->actionShowSystemInfo = new QAction( this );
   this->actionShowSystemInfo->setText( "System Info" );
   connect( this->actionShowSystemInfo, SIGNAL( triggered() ), this, SLOT( showSystemInfo() ) );

   // Help->Show Log
   this->actionShowLog = new QAction( this );
   this->actionShowLog->setText( "Show Log" );
   this->actionShowLog->setCheckable( true );
   this->actionShowLog->setChecked( false );
   connect( this->actionShowLog, SIGNAL( triggered(bool) ), this, SLOT( showLog(bool) ) );


   // TOOLBAR-ONLY ACTIONS

   // set orbit manipulator
   this->actionOrbitManip = new QAction( this );
   this->actionOrbitManip->setText( "Camera orbitting" );
   this->actionOrbitManip->setIcon( QIcon( ":/images/orbiting.xpm" ) );
   this->actionOrbitManip->setCheckable( true );
   this->actionOrbitManip->setChecked( true );
   connect( this->actionOrbitManip, SIGNAL( triggered() ), this, SLOT( setOrbitManipulator() ) );

   // set 1st person manipulator
   this->actionFirstPersonManip = new QAction( this );
   this->actionFirstPersonManip->setText( "First person look around" );
   this->actionFirstPersonManip->setIcon( QIcon( ":/images/first_person.xpm" ) );
   this->actionFirstPersonManip->setCheckable( true );
   this->actionFirstPersonManip->setChecked( false );
   connect( this->actionFirstPersonManip, SIGNAL( triggered() ), this, SLOT( setFirstPersonManipulator() ) );

   // manipulator group
   manipulatorGroup = new QActionGroup( this );
   manipulatorGroup->addAction( actionOrbitManip );
   manipulatorGroup->addAction( actionFirstPersonManip );
   actionOrbitManip->setChecked( true );

   // trun axis on/off
   this->actionAxisVisible = new QAction(this);
   this->actionAxisVisible->setText("Axis helper");
   this->actionAxisVisible->setIcon(QIcon(":/images/axis.png"));
   this->actionAxisVisible->setCheckable(true);
   this->actionAxisVisible->setChecked(true);
   connect(this->actionAxisVisible, SIGNAL(toggled(bool)), this, SLOT(setAxisVisible(bool)));
}


/**
 * Creates menu of MainWindow. It creates core menu objects
 * and fills the menu by actions.
 *
 * The method is used to build MainWindow menu during GUI creation process.
 */
void MainWindow::createMenu()
{
   // create menus and submenus
   this->menuBar = new QMenuBar( this );
      this->menuFile = new QMenu( this->menuBar );
      this->menuView = new QMenu( this->menuBar );
         this->menuBackground = new QMenu( this->menuView );
         this->menuWindowSize = new QMenu( this->menuView );
      this->menuHelp = new QMenu( this->menuBar );

   // set and initialize menu bar
   this->setMenuBar( this->menuBar );
   this->menuBar->addAction( this->menuFile->menuAction() );
   this->menuBar->addAction( this->menuView->menuAction() );
   this->menuBar->addAction( this->menuHelp->menuAction() );

   //
   // "File" menu
   //
   this->menuFile->setTitle( "&File" );
   this->menuFile->addAction( this->actionOpenModel );
   this->menuFile->addSeparator();
   this->menuFile->addAction( this->actionQuit );

   //
   // "View" menu
   //
   this->menuView->setTitle( "&View" );
   this->menuView->addAction( this->actionReloadModel );
   this->menuView->addAction( this->menuBackground->menuAction() );
   this->menuView->addAction( this->menuWindowSize->menuAction() );
   this->menuView->addSeparator();
   this->menuView->addAction( this->actionLoadView );
   this->menuView->addAction( this->actionSaveView );
   this->menuView->addAction( this->actionZoomAll );
   this->menuView->addSeparator();
   this->menuView->addAction( this->actionPovrayRendering );
   this->menuView->addAction( this->actionStereo );

      // "Background" submenu
      menuBackground->setTitle( "&Background" );
      this->menuBackground->addAction( this->actionBlack );
      this->menuBackground->addAction( this->actionDarkGrey );
      this->menuBackground->addAction( this->actionGrey );
      this->menuBackground->addAction( this->actionLightGrey );
      this->menuBackground->addAction( this->actionWhite );
      this->menuBackground->addAction( this->actionGriseous );
      this->menuBackground->addAction( this->actionTan );
      this->menuBackground->addAction( this->actionCustomColor );
      this->menuBackground->addSeparator();
      this->menuBackground->addAction( this->actionSelectCustomColor );
      this->menuBackground->addSeparator();
      this->menuBackground->addAction( this->actionBackgroundImage );

      // Window Size submenu
      menuWindowSize->setTitle( "&Window size" );
      this->menuWindowSize->addAction( this->actionWindowSize1920x1080 );
      this->menuWindowSize->addAction( this->actionWindowSize1600x900 );
      this->menuWindowSize->addAction( this->actionWindowSize1366x768 );
      this->menuWindowSize->addAction( this->actionWindowSize1280x1024 );
      this->menuWindowSize->addAction( this->actionWindowSize1280x720 );
      this->menuWindowSize->addAction( this->actionWindowSize1024x768 );
      this->menuWindowSize->addAction( this->actionWindowSize800x600 );

   // "Help" menu
   this->menuHelp->setTitle( "&Help" );
   this->menuHelp->addAction( this->actionAbout );
   this->menuHelp->addSeparator();
   this->menuHelp->addAction( this->actionShowSceneInfo );
   this->menuHelp->addAction( this->actionShowSystemInfo );
   this->menuHelp->addAction( this->actionShowLog );

}


/**
 * Creates toolbar widgets and attaches actions.
 */
void MainWindow::createToolbars()
{
   // top toolbar
   this->toolHead = new QToolBar( this );
   this->toolHead->setObjectName( "HeadToolbar" );
   this->toolHead->setMovable( false );
   this->toolHead->setOrientation( Qt::Horizontal );
   this->addToolBar( Qt::TopToolBarArea, this->toolHead );

   // bottom toolbar
   this->toolBottom = new QToolBar( this );
   this->toolBottom->setObjectName( "BottomToolbar" );
   this->toolBottom->setMovable( false );
   this->toolBottom->setOrientation( Qt::Horizontal );
   this->addToolBar( Qt::BottomToolBarArea, this->toolBottom );


   // top toolbar content
   this->toolHead->clear();
   this->toolHead->addAction( this->actionDefaultView );
   this->toolHead->addSeparator();
   this->toolHead->addAction( this->actionLoadView );
   this->toolHead->addAction( this->actionSaveView );
   this->toolHead->addAction( this->actionZoomAll );
   this->toolHead->addSeparator();
   this->toolHead->addAction( this->actionPovrayRendering );

   // POV-Ray settings staff in top toolbar
   this->toolHead->addWidget( new QLabel( "   Scale " ) );
   this->povScale = new QDoubleSpinBox( this );
   this->povScale->setMinimum( 0.00 );
   this->povScale->setMaximum( 100. );
   this->povScale->setSingleStep( 0.5 );
   this->povScale->setValue( 1. ); // FIXME: preserve the value among the sessions
   this->toolHead->addWidget( this->povScale );
   this->povFastAntialias = new QCheckBox;
   this->povFastAntialias->setVisible( false );
   this->povFastAntialias->setChecked( false ); // FIXME: preserve checked status among the sessions
   this->povFastAntialias->setText( " Fast antialiasing" );
   this->povFastAntialias->setLayoutDirection( Qt::RightToLeft );
   this->toolHead->addWidget( this->povFastAntialias );

   // bottom toolbar content
   this->toolBottom->clear();
   this->toolBottom->addAction( this->actionOrbitManip );
   this->toolBottom->addAction( this->actionFirstPersonManip );
}


/**
 * Creates statusbar widgets.
 */
void MainWindow::createStatusBar()
{

}


/**
 * The openModel slot opens model from the file stored on disk.
 *
 * @param fileName Name of file to be loaded. If empty, a dialog box is displayed.
 */
void MainWindow::openModel( QString fileName )
{

   if( fileName.isEmpty() )
   {
      // open dialog
      fileName = QFileDialog::getOpenFileName(
            NULL,          // parent widget
            NULL,          // default caption
            NULL,          // default dir
            "OpenInventor (*.iv *.ivx *.ivl *.ivz *.ivzl);;"
            "OpenSceneGraph (*.osgt *.osgb *.osg *.ive);;"
            "3D Studio MAX (*.3ds);;"
            "All Files (*.*)",
            0,             // selected filter (0 - default value)
            0 );           // options (0 - default value)

      // empty path is returned on cancel button
      if( fileName.isEmpty() )
         return;
   }

   loadModel( fileName, true );

}


/**
 * The reloadModel slot reloads the model from the file.
 * This is useful, for example, to perform a model update
 * upon the change of the file on disk.
 *
 * The model to reload is given by the file name of the active document.
 */
void MainWindow::reloadModel()
{
   if( Lexolights::activeDocument() )
      loadModel( Lexolights::activeDocument()->getFileName(), false );
}



/**
 * The method loads the model from the file fileName.
 *
 * If parameter resetViwSettings is set to false,
 * camera position is set to its initial default view
 * and all the GUI settings are reset to default values as well (shadow mode,...).
 */
void MainWindow::loadModel(QString fileName, bool resetViewSettings )
{

   // show "loading" message dialog
#if 0 //FIXME: resurrect this
   this->message->setMessage(QString("Loading...\nPlease wait."));
   this->message->show();
#endif

   // disable menu while loading
   this->menuBar->setDisabled( true );

   // redraw window to erase "open file" dialog from buffer
   if( _centralContainer->activeWidget() )
      _centralContainer->activeWidget()->repaint();

   // release previous scene from memory
   // to make it available for the new scene
   Lexolights::setActiveDocument( NULL );
   Lexolights::viewer()->setSceneData( NULL, false );
   bool ok = false;

   if( !fileName.isEmpty() ) {

      ref_ptr< LexolightsDocument > newDocument = new LexolightsDocument;
      if( newDocument->openFile( fileName ) )
      {
         openDocument( newDocument, resetViewSettings );
         ok = true;
      }
   }

   // reset camera position if requested and not already done
   if( resetViewSettings && !ok )
      Lexolights::viewer()->setSceneData( NULL, true );

   // hide message dialog and enable menu
#if 0 //FIXME: resurrect this
   this->message->hide();
#endif
   this->menuBar->setEnabled( true );

}


/**
 * The method opens the model given by document parameter.
 *
 * If parameter resetViwSettings is set to false,
 * camera position is set to its initial default view
 * and all the GUI settings are reset to default values as well (shadow mode,...).
 */
void MainWindow::openDocument( LexolightsDocument *document, bool resetViewSettings )
{
   if( document == Lexolights::activeDocument() )
      return;

   Log::notice() << "Opening document " << ( document ? document->getFileName() : "NULL" )
                 << " in MainWindow." << Log::endm;

   // set new active document
   // and re-connect document's sceneChanged signal
   if( Lexolights::activeDocument() )
      disconnect( Lexolights::activeDocument(), SIGNAL( sceneChanged() ),
                  this, SLOT( activeDocumentSceneChanged() ) );
   Lexolights::setActiveDocument( document );
   if( Lexolights::activeDocument() )
      connect( Lexolights::activeDocument(), SIGNAL( sceneChanged() ),
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
   if( Lexolights::activeDocument() )
      Lexolights::viewer()->setSceneData( actionPPL->isChecked() ?
                                             Lexolights::activeDocument()->getPPLScene() :
                                             Lexolights::activeDocument()->getOriginalScene(),
                                          resetViewSettings );
   else
      Lexolights::viewer()->setSceneData( NULL, resetViewSettings );
}


void MainWindow::activeDocumentSceneChanged()
{
   Lexolights::viewer()->setSceneData( actionPPL->isChecked() ?
                                          Lexolights::activeDocument()->getPPLScene() :
                                          Lexolights::activeDocument()->getOriginalScene(),
                                       false );
}


/**
 * The slot sets the camera to its default view, e.g. to home possition
 * that is set properly and far enough to see all the objects of the scene.
 */
void MainWindow::defaultView()
{
   Lexolights::viewer()->setSceneData( Lexolights::viewer()->getSceneData(), true );
}


/**
 * The slot loads view settings (e.g. camera setup) from a file.
 */
void MainWindow::loadView()
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

   // read file
   int r = loadIVV( filename, *Lexolights::viewer()->getCamera(),
                    dynamic_cast< osgGA::StandardManipulator& >( *Lexolights::viewer()->getCameraManipulator() ) );

   // init manipulator (this stops any camera animation, etc., but should not change its position)
   if( r > 0 ) {

      ref_ptr< osgGA::GUIEventAdapter > dummyEvent = Lexolights::viewer()->getEventQueue()->createEvent();
      osgViewer::GraphicsWindow *gw = dynamic_cast< osgViewer::GraphicsWindow* >( Lexolights::viewer()->getCamera()->getGraphicsContext() );
      if( gw )
         Lexolights::viewer()->getCameraManipulator()->init( *dummyEvent, *gw );

   }

   // log
   if( r > 0 )
      Log::notice() << "loadView: View settings successfully loaded from "
                    << filename << "." << Log::endm;
   else
      Log::fatal() << "loadView: Failed to load view settings from "
                   << filename << "." << Log::endm;
}


/**
 * The slot saves view settings (e.g. camera setup) to a file.
 */
void MainWindow::saveView()
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

   // write to file
   int r = saveIVV( filename, *Lexolights::viewer()->getCamera(),
                    dynamic_cast< osgGA::StandardManipulator& >( *Lexolights::viewer()->getCameraManipulator() ) );

   // log
   if( r > 0 )
      Log::notice() << "saveView: View settings saved to "
                    << filename << " successfully." << Log::endm;
   else
      Log::fatal() << "saveView: Can not save view settings to "
                   << filename << "." << Log::endm;
}


/**
 * The slot zooms to see the whole model from an optimal distance.
 */
void MainWindow::zoomAll()
{
   // call home on manipulator
   ref_ptr< osgGA::GUIEventAdapter > dummyEvent = Lexolights::viewer()->getEventQueue()->createEvent();
   osgViewer::GraphicsWindow *gw = dynamic_cast< osgViewer::GraphicsWindow* >( Lexolights::viewer()->getCamera()->getGraphicsContext() );
   if( gw )
      Lexolights::viewer()->getCameraManipulator()->home( *dummyEvent, *gw );
}


/**
 * The slot sets background color of the viewer according to the GUI menu choice.
 */
void MainWindow::setBackgroundColor()
{
   CadworkViewer *viewer = Lexolights::viewer();

   if( this->actionBlack->isChecked() )
      viewer->setBackgroundColor( Vec4( 0.0, 0.0, 0.0, 1.0 ) );

   if( this->actionDarkGrey->isChecked() )
      viewer->setBackgroundColor( Vec4( 0.25, 0.25, 0.25, 1.0 ) );

   if( this->actionGrey->isChecked() )
      viewer->setBackgroundColor( Vec4( 0.5, 0.5, 0.5, 1.0 ) );

   if( this->actionLightGrey->isChecked() )
      viewer->setBackgroundColor( Vec4( 0.75, 0.75, 0.75, 1.0 ) );

   if( this->actionWhite->isChecked() )
      viewer->setBackgroundColor( Vec4( 1.0, 1.0, 1.0, 1.0 ) );

   if( this->actionGriseous->isChecked() )
      viewer->setBackgroundColor( Vec4( 0.4, 0.4, 0.6, 1.0 ) );

   if( this->actionTan->isChecked() )
      viewer->setBackgroundColor( Vec4( 0.7578125, 0.7265625, 0.5859375, 1.0 ) );

   if( this->actionCustomColor->isChecked() )
      viewer->setBackgroundColor( _customColor );
}


/**
 * Opens color-dialog for to select custom background color.
 */
void MainWindow::selectCustomColor()
{
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
   Lexolights::viewer()->setBackgroundColor( _customColor );
}


/**
 * Select and show background image.
 *
 * @param show True when showing, false when hiding.
 */
void MainWindow::backgroundImage( bool show )
{

}


void MainWindow::setCentralWidgetSize()
{
   // get sending action
   QAction *a = dynamic_cast< QAction* >( QObject::sender() );
   if( !a )
      return;

   // parse text of the action (format width'x'height)
   QStringList s = a->text().split( 'x' );
   if( s.size() != 2 )
      return;

   // parse dimensions
   bool ok1, ok2;
   int width = s[0].toInt( &ok1 );
   int height = s[1].toInt( &ok2 );
   if( !ok1 || !ok2 )
      return;

   // resize CentralContainer and MainWindow
   QSize savedMinSize = _centralContainer->minimumSize();
   QSize savedMaxSize = _centralContainer->maximumSize();
   setWindowState( windowState() & ~Qt::WindowMaximized ); // un-maximize
   _centralContainer->setFixedSize( width, height );
   adjustSize(); // perform resize
   _centralContainer->setMinimumSize( savedMinSize );
   _centralContainer->setMaximumSize( savedMaxSize );
}


/**
 * Shows/hides log window.
 */
void MainWindow::showLog( bool visible )
{

   // handle LogWindow visibility
   // note: when minimizing application, visible param is false
   // and it turned Log visibility to off until the test of sender
   // was implemented. There is other way to handle it: to
   // implement our own "visibilityChanged" signal. PCJohn-2010-01-16
   if( sender() != Log::getWindow() || Log::getWindow() == NULL &&
       Log::isVisible() != visible )

      if( visible )
         Log::showWindow( this, this, SLOT(showLog(bool)) );
      else
         Log::hideWindow();

   // handle actionShowLog checked status
   if( sender() != actionShowLog &&
       actionShowLog->isChecked() != visible )
      actionShowLog->setChecked( visible );

}


/**
 * Convenience method returning log window visibility flag.
 */
bool MainWindow::isLogShown() const
{
   return Log::isVisible();
}


void MainWindow::showAboutDlg()
{
   AboutDialog *dlg = new AboutDialog;
   dlg->setAttribute( Qt::WA_DeleteOnClose );
   dlg->show();
}


void MainWindow::showSceneInfo()
{
   SceneInfoDialog *dlg = new SceneInfoDialog;
   dlg->setAttribute( Qt::WA_DeleteOnClose );
   dlg->show();
}


void MainWindow::showSystemInfo()
{
   SystemInfoDialog *dlg = new SystemInfoDialog;
   dlg->setAttribute( Qt::WA_DeleteOnClose );
   dlg->show();
}


/**
 * Set the orbit manipulator.
 *
 */
void MainWindow::setOrbitManipulator()
{
   Lexolights::viewer()->setOrbitManipulator();
   actionOrbitManip->setChecked( true );
}


/**
 * Set the 1st person manipulator.
 *
 */
void MainWindow::setFirstPersonManipulator()
{
   Lexolights::viewer()->setFirstPersonManipulator();
   actionFirstPersonManip->setChecked( true );
}


/**
 * Sets axis geometry visible or not.
 */
void MainWindow::setAxisVisible(bool visible)
{
#if 0 //FIXME: resurrect this
   osgWidget->getAxis()->setVisible(visible);
   osgWidget->update();
#endif
}


void MainWindow::keyPressEvent( QKeyEvent *event )
{
   if( event->key() == Qt::Key_Control )
      setFirstPersonManipulator();

   inherited::keyPressEvent( event );
}


void MainWindow::keyReleaseEvent( QKeyEvent *event )
{
   if( event->key() == Qt::Key_Control )
      setOrbitManipulator();

   inherited::keyPressEvent( event );
}


/**
 * The method handles the close event.
 * It calls close on all associated dialogs. If all of them accept the event,
 * it reports the event as accepted. As a result, all the dialogs and
 * MainWindow are closed.
 */
void MainWindow::closeEvent( QCloseEvent *event )
{
   QSettings settings;
   settings.setValue( "geometry", saveGeometry() );
   settings.setValue( "windowState", saveState() );
   inherited::closeEvent( event );
}


/** The function for handling drag and drop events.
 *  It is expected to take any drag enter events and drag drop events.
 *
 *  When passing drag enter events, dropped parameter should be set to false.
 *  The function accepts or rejects the event based on the test whether
 *  the dragged file is supported and can be opened by the application.
 *
 *  Drop events should be passed with dropped parameter set to true.
 *  If the dropped file is supported, the function calls the appropriate
 *  method to open the file. */
static void handleDragAndDrop( QDropEvent *event, bool dropped )
{
   // check supported types
   const QMimeData* mime = event->mimeData();
   if( mime->hasUrls() )
   {
      // check first URL
      QList<QUrl> urls = mime->urls();
      if( urls.size() > 0 )
      {
         // check scheme, e.g. protocol,
         // and get file name
         QString url;
         if( urls[0].scheme().compare( "ftp" ) == 0 )
         {
            url = urls[0].toString();
         }
         else
         {
            url = urls[0].toLocalFile();
         }

         // check file extension
         std::string ext = osgDB::getFileExtension( std::string( url.toLocal8Bit() ) );
         if( osgDB::Registry::instance()->getReaderWriterForExtension( ext ) != NULL )
         {
            // supported extension
            event->acceptProposedAction();
            if( dropped )
            {
               QCoreApplication::processEvents();
               Lexolights::mainWindow()->loadModel( url, true );
            }
         }
         else
            // unknown file extension
            event->ignore();
      }
      else
         // no URLs
         event->ignore();
   }
   else
      // unknown type of drop-event
      event->ignore();
}


/** Enter drag event. Accepts all model files supported by OpenSceneGraph. */
void MainWindow::dragEnterEvent( QDragEnterEvent* event )
{
   handleDragAndDrop( event, false );
}


/** Drop event. Opens the model dropped into the window.
 *  It accepts all model files supported by OpenSceneGraph. */
void MainWindow::dropEvent( QDropEvent *event )
{
   handleDragAndDrop( event, true );
}


void MainWindow::setActiveCentralWidget( QWidget *widget )
{
   _centralContainer->setActiveWidget( widget );
}


/** Returns active central widget.
 *  Central widgets are managed by CentralContainer class. Central holder
 *  makes just one widget active and visible while the rest is hidden. */
QWidget* MainWindow::getActiveCentralWidget()
{
   return _centralContainer->activeWidget();
}


void MainWindow::switchToLastGLWidget()
{
   _centralContainer->switchToLastGLWidget();
}


/**
 * Sets whether per-pixel lighting (shader based) is used for scene graph rendering.
 *
 * @param on Set to true for per-pixel lighting and false for standard rendering.
 */
void MainWindow::setPerPixelLighting( bool on )
{
   if( Lexolights::activeDocument() )
      Lexolights::viewer()->setSceneData( on ? Lexolights::activeDocument()->getPPLScene()
                                             : Lexolights::activeDocument()->getOriginalScene(),
                                          false );
}


void MainWindow::renderUsingPovray()
{
   // if no active document, do nothing
   if( !Lexolights::activeDocument() ) {
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
      ReturnHandler() : restoreGUI( true )
      {
         // update GUI
#if 0 // no image widget, use external window instead
         Lexolights::mainWindow()->getImageWidget()->setText( "POV-Ray rendering in progress..." );
         Lexolights::mainWindow()->setActiveCentralWidget( Lexolights::mainWindow()->getImageWidget() );
#endif
         Lexolights::mainWindow()->actionPovrayRendering->setEnabled( false );

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
            Lexolights::mainWindow()->switchToLastGLWidget();
            Lexolights::mainWindow()->actionPovrayRendering->setEnabled( true );
         }
      }
   } returnHandler;


   //
   //  Export POV file
   //

   // paths
   Timer time;
   string fileName( osgDB::getNameLessExtension( Lexolights::activeDocument()->getFileName().toLocal8Bit().data() ) );
   string filePath( osgDB::getFilePath( fileName ) );
   fileName += ".pov";
   string simpleFileName( osgDB::getSimpleFileName( fileName ) ); // no path, includes pov extension
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

   // Do we need to re-export the scene?
   QString povSceneFile = QDir( pathDir.filePath( "scene.inc" ) ).canonicalPath();
   bool reexport = true;
   if( !povSceneFile.isEmpty() )
   {
      QSettings settings;
      settings.beginGroup( "PovrayExport" );
      QString appBuildTimeStamp = settings.value( "ApplicationBuildTimeStamp" ).toString();
      if( appBuildTimeStamp == QString( buildDate ) + " " + buildTime )
      {
         QString lastInputScene = settings.value( "LastInputScene" ).toString();
         QString lastOutputScene = settings.value( "LastOutputScene" ).toString();
         if( !lastInputScene.isEmpty() && !lastOutputScene.isEmpty() &&
            Lexolights::activeDocument()->getCanonicalName() == lastInputScene &&
            povSceneFile == lastOutputScene )
         {
            FileTimeStamp::getRecord( "povray input scene", Lexolights::activeDocument()->getCanonicalName().toStdString() ) =
                  FileTimeStamp( settings.value( "LastInputFileTimeStamp" ).toString().toStdString(), Lexolights::activeDocument()->getCanonicalName().toStdString() );
            FileTimeStamp::getRecord( "povray output scene", povSceneFile.toStdString() ) =
                  FileTimeStamp( settings.value( "LastOutputFileTimeStamp" ).toString().toStdString(), povSceneFile.toStdString() );
         }
         settings.endGroup();

         FileTimeStamp expectedTS = FileTimeStamp::getRecord( "povray input scene", Lexolights::activeDocument()->getCanonicalName().toStdString() );
         FileTimeStamp currentTS = Lexolights::activeDocument()->getSceneTimeStamp();
         reexport = expectedTS != currentTS;
         Log::info() << "Checking Lexolights scene timestamp (file: " << Lexolights::activeDocument()->getCanonicalName() << "):\n"
                        "   expected timestamp: " << expectedTS.getTimeStampAsString() << "\n"
                        "   current timestamp: " << currentTS.getTimeStampAsString() << Log::endm;
         if( reexport == false )
         {
            expectedTS = FileTimeStamp::getRecord( "povray output scene", povSceneFile.toStdString() );
            currentTS = FileTimeStamp( povSceneFile.toStdString() );
            reexport = expectedTS != currentTS;
            Log::info() << "Checking POV scene file timestamp (file: " << povSceneFile << "):\n"
                           "   expected timestamp: " << expectedTS.getTimeStampAsString() << "\n"
                           "   current timestamp: " << currentTS.getTimeStampAsString() << Log::endm;
         }
      }
   }

   // log messages
   Log::info() << "Exporting scene to POV-Ray file (" << fileName << ")"
                  " while placing scene to separate file (scene.inc)..." << Log::endm;
   if( reexport )
      Log::info() << "Going to update scene file (scene.inc)." << Log::endm;
   else
      Log::info() << "Skipping update of scene file (scene.inc), as it is not required." << Log::endm;

   // pov create camera
   ref_ptr< Camera > camera = dynamic_cast< Camera* >(
         Lexolights::viewer()->getCamera()->clone( CopyOp::SHALLOW_COPY ) );
   camera->removeChild( 0, camera->getNumChildren() );
   camera->addChild( Lexolights::activeDocument()->getOriginalScene() );

   // create options
   ref_ptr< osgDB::Options > options = new osgDB::Options( "CopyFiles SceneFileName=scene.inc" +
                                                           ( reexport ? string() : " OnlyCameraFile" ) );
   string modelDir = osgDB::getFilePath( Lexolights::activeDocument()->getFileName().toLocal8Bit().data() );
   if( !modelDir.empty() )
      options->getDatabasePathList().push_back( modelDir );
   else
      Log::warn() << "Render using POV-Ray warning: Can not get model directory." << endl;

   // write pov
   bool r = osgDB::writeNodeFile( *camera, fileName, options );
   camera = NULL;
   double dt = time.time_m();
   if( r )  Log::notice() << QString( "POV-Ray file %1 successfully written in %2ms" )
                                     .arg( fileName.c_str() ).arg( dt, 0, 'f', 2 ) << Log::endm;
   else {
      Log::fatal() << "Can not export POV-Ray file " << fileName << Log::endm;
      return;
   }

   // update time stamps
   FileTimeStamp povSceneFileTimeStamp( povSceneFile.toStdString() );
   FileTimeStamp::getRecord( "povray input scene",  Lexolights::activeDocument()->getCanonicalName().toStdString() ) = Lexolights::activeDocument()->getSceneTimeStamp();
   FileTimeStamp::getRecord( "povray output scene", povSceneFile.toStdString() ) = povSceneFileTimeStamp;
   QSettings settings;
   settings.beginGroup( "PovrayExport" );
   settings.setValue( "LastInputScene", Lexolights::activeDocument()->getCanonicalName() );
   settings.setValue( "LastInputFileTimeStamp", Lexolights::activeDocument()->getSceneTimeStamp().getTimeStampAsString().c_str() );
   settings.setValue( "LastOutputScene", povSceneFile );
   settings.setValue( "LastOutputFileTimeStamp", povSceneFileTimeStamp.getTimeStampAsString().c_str() );
   settings.setValue( "ApplicationBuildTimeStamp", QString( buildDate ) + " " + buildTime );
   settings.endGroup();


   //
   //  Write ini file
   //
   stringstream iniStream;
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
                    const QString& workingDirectory, const QString& renderedImageFile )
         : ExternalApplicationWorker( program, arguments, workingDirectory, true, true ),
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
         Lexolights::mainWindow()->actionPovrayRendering->setEnabled( true );

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
            Lexolights::mainWindow()->switchToLastGLWidget();
         }
      }
   protected:
      QString _renderedImageFile;
   };
#if defined(__WIN32__) || defined(_WIN32)

   // prepare argument list (fileName only as width, height, output format and antialiasing went to povray.ini file)
   // note: With /EXIT option, there must be no other POV-Ray instance running to successfully start rendering.
   //       If using /EXIT, /NORESTORE is useful to avoid loading of recently opened files (this is just waste of time),
   //       but it causes POV-Ray to forget recently opened files and when the user starts POV-Ray manually next time,
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
   ( new PovrayWorker( executable, params, QString( filePath.c_str() ), ( filePath + "/" + osgDB::getNameLessExtension( simpleFileName ) + ".png" ).c_str() ) )->start();

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


static void activateOSGWidget( QWidget *w, ref_ptr< osgViewer::GraphicsWindow > &gw )
{
   // stop threads
   osgViewer::ViewerBase::ThreadingModel tm = Lexolights::viewer()->getThreadingModel();
   Lexolights::viewer()->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );

   // setup QUAD_BUFFER stereo in viewer's DisplaySettings
   DisplaySettings *ds = Lexolights::viewer()->getDisplaySettings();
   if( ds == NULL )
   {
      ds = new DisplaySettings( *DisplaySettings::instance() );
      Lexolights::viewer()->setDisplaySettings( ds );
   }
   if( dynamic_cast< QGLWidget* >( w ) && 
       dynamic_cast< QGLWidget* >( w )->format().stereo() )
   {
      ds->setStereo( true );
      ds->setStereoMode( DisplaySettings::QUAD_BUFFER );
   }
   else
      ds->setStereo( false );

   if( gw.get() == NULL )
   {
      // setup viewer and GraphicsWindow
      ref_ptr< osg::Referenced > windowData = new osgQt::GraphicsWindowQt::WindowData( dynamic_cast< osgQt::GLWidget* >( w ) );
      Lexolights::viewer()->setUpViewInWindow( windowData, 0, 0, w->width(), w->height(),
                                               false, true, NULL, NULL );
      gw = dynamic_cast< osgViewer::GraphicsWindow* >( Lexolights::viewer()->getCamera()->getGraphicsContext() );
   }
   else
   {
      Camera *camera = Lexolights::viewer()->getCamera();
      camera->setGraphicsContext( gw );

      int x, y, width, height;
      gw->getWindowRectangle( x, y, width, height );
      gw->getEventQueue()->getCurrentEventState()->setWindowRectangle( x, y, width, height );

      double fovy, aspectRatio, zNear, zFar;
      camera->getProjectionMatrixAsPerspective( fovy, aspectRatio, zNear, zFar );

      double newAspectRatio = double( width ) / double( height );
      double aspectRatioChange = newAspectRatio / aspectRatio;
      if (aspectRatioChange != 1.0)
      {
         camera->getProjectionMatrix() *= osg::Matrix::scale( 1.0 / aspectRatioChange, 1.0, 1.0 );
      }

      camera->setViewport( new osg::Viewport( x, y, width, height ) );
   }

   // make sure that State has correct display settings
   gw->getState()->setDisplaySettings( ds );

   // set active central widget
   Lexolights::mainWindow()->getCentralContainer()->internalSetActiveWidget( w );

   // realize
   if( !gw->isRealized() )
      gw->realize();

   // restart threads
   Lexolights::viewer()->setThreadingModel( tm );
}


QGLWidget* MainWindow::createGLWidget( void (*&activateFunc)( QWidget *w, ref_ptr< osgViewer::GraphicsWindow > &gw ),
                                       QGLWidget *shareWidget, bool stereo )
{
   // create QGLWidget
   // (put info messages to the log as creation of OpenGL window may cause SIGSEGV
   // when OpenGL is not installed properly (seen on Ubuntu 10.10 and Catalyst 10.11))
   Log::info() << "Creating OpenGL window (stereo: " << (stereo ? "yes" : "no") << ")..." << std::endl;
   QGLWidget *w = new OSGWidget( QGLFormat( stereo ? QGL::StereoBuffers | QGL::SampleBuffers : QGL::SampleBuffers ),
                                 _centralContainer, shareWidget );
   activateFunc = &activateOSGWidget;
   return w;
}


QGLWidget* MainWindow::initGLWidgetInternal( QGLWidget *shareWidget, bool stereo )
{
   // stop threading during GUI changes
   struct PreserveThreading {

      osgViewer::ViewerBase::ThreadingModel tm;

      PreserveThreading()
      {
         // stop threads
         tm = Lexolights::viewer()->getThreadingModel();
         Lexolights::viewer()->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
      }

      ~PreserveThreading()
      {
         // restart threads
         Lexolights::viewer()->setThreadingModel( tm );
      }

   } preserveThreading;


   // create widget
   void (*activateFunc)( QWidget *w, ref_ptr< osgViewer::GraphicsWindow > &gw );
   QGLWidget *w = createGLWidget( activateFunc, shareWidget, stereo );

   // test for sharing
   if( shareWidget && !w->isSharing() )
   {
      Log::warn() << "OpenGL error: OpenGL window does not share OpenGL data." << endl;
   }

   // test for stereo
   if( stereo )
   {
      if( w->format().stereo() )
      {
         Log::notice() << "STEREO format initialized for OpenGL window." << std::endl;
         Log::info() << "OpenGL window created successfully." << std::endl;
      }
      else
      {
         Log::warn() << "STEREO format did not received for OpenGL window." << std::endl;
         Log::warn() << "OpenGL window with STEREO capabilities was not created." << endl;

         // delete widget
         delete w;
         w = NULL;
         setStereoscopicRendering( false );
      }
   }
   else
   {
      Log::info() << "OpenGL window created successfully." << std::endl;
   }

   // append widget to central holder
   if( w )
      _centralContainer->addWidget( w, activateFunc );

   return w;
}


void MainWindow::initGLWidget()
{
   // test whether OSG widget was already created (if not, create one)
   if( _glWidget == NULL )
   {
      _glWidget = initGLWidgetInternal( _glStereoWidget, false );
      _ownsGLWidget = true;
   }
}


void MainWindow::initGLStereoWidget()
{
   // test whether stereo window was already created
   if( _glStereoWidget == NULL )
   {
      _glStereoWidget = initGLWidgetInternal( _glWidget, true );
      _ownsGLStereoWidget = true;
   }
}


static void internalSetWidget( QGLWidget *&_widget, QGLWidget *w, bool &_ownsWidget,
                               MainWindow::ActivateWidgetFunc *callback, CentralContainer *_centralContainer )
{
   // ignore setting the same widget
   if( _widget == w )
      return;

   // delete old widget
   if( _widget )
   {
      _centralContainer->removeWidget( _widget );
      if( _ownsWidget )
         delete _widget;
   }

   // set widget
   _widget = w;
   _ownsWidget = false;

   // append widget to central holder
   if( w )
      _centralContainer->addWidget( _widget, callback );
}


void MainWindow::setGLWidget( QGLWidget *widget, MainWindow::ActivateWidgetFunc *callback )
{
   internalSetWidget( _glWidget, widget, _ownsGLWidget, callback, _centralContainer );
}


void MainWindow::setGLStereoWidget( QGLWidget *widget, MainWindow::ActivateWidgetFunc *callback )
{
   internalSetWidget( _glStereoWidget, widget, _ownsGLStereoWidget, callback, _centralContainer );
}


/**
 * Activates/disables stereoscopic rendering.
 * The method takes care about the creation of OSG Widgets if they do not exist yet.
 */
void MainWindow::setStereoscopicRendering( bool on )
{
   // update GUI
   if( actionStereo->isChecked() != on )
   {
      bool wasBlocked = actionStereo->blockSignals( true );
      actionStereo->setChecked( on );
      actionStereo->blockSignals( wasBlocked );
   }


   if( on == false )
   {
      //
      //  Rule: show OSG widget (non-stereo)
      //
      //  When setting stereoscopic rendering off,
      //  any widget may be active inside central widget.
      //  Whatever widget is active, ignore it and show
      //  OSG widget (non-stereo) inside central widget.
      //

      // init widget
      initGLWidget();

      // set active widget
      if( _glWidget )
         setActiveCentralWidget( _glWidget );
   }
   else
   {
      //
      //  Rules:
      //    1: show stereo widget
      //    2: if 1 failed, e.g. no stereo support, show non-stereo widget
      //
      //  When setting stereoscopic rendering on,
      //  any widget may be active inside central widget.
      //  Whatever widget is active, ignore it and show
      //  stereo widget inside central widget.
      //

      // init widget
      initGLStereoWidget();

      // set active widget
      if( _glStereoWidget )
         setActiveCentralWidget( _glStereoWidget );
   }
}
