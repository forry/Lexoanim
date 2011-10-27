
/**
 * @file
 * MainWindow class header.
 *
 * @author PCJohn (Jan Pečiva)
 *         Tomáš Pafčo
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <QMainWindow>
#include <QString>
#include <osg/ref_ptr>
#include <osg/Vec4>

namespace osg {
   class Node;
   class Camera;
}
namespace osgViewer {
   class GraphicsWindow;
}
class QActionGroup;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QGLWidget;
class QToolButton;
class LexolightsDocument;
class CentralContainer;


/**
 * Application's main window.
 */
class MainWindow : public QMainWindow
{
   Q_OBJECT

   typedef QMainWindow inherited;
   friend class Lexolights;

public:

   // constructor and destructor
   MainWindow( QWidget* parent = 0, Qt::WFlags flags = 0, bool buildGUI = true );
   virtual ~MainWindow();

   // create GUI
   virtual void buildGUI( bool buildGLWidget = true );

   // QGLWidgets
   inline const QGLWidget* getGLWidget() const;
   inline QGLWidget* getGLWidget();
   inline QGLWidget* getGLStereoWidget();
   void initGLWidget();
   void initGLStereoWidget();
   typedef void ActivateWidgetFunc( QWidget *w, osg::ref_ptr< osgViewer::GraphicsWindow > &gw );
   virtual void setGLWidget( QGLWidget *widget, ActivateWidgetFunc *callback = NULL );
   virtual void setGLStereoWidget( QGLWidget *widget, ActivateWidgetFunc *callback = NULL );

   // get various widgets
   inline QLabel* getImageWidget();
   inline CentralContainer* getCentralContainer();

   // active central widget
   void setActiveCentralWidget( QWidget *widget );
   QWidget* getActiveCentralWidget();
   void switchToLastGLWidget();

protected:

   // widget holding all central widgets, while showing just the active one
   CentralContainer *_centralContainer;

   // GLWidgets, central widgets used as the rendering canvas
   QGLWidget *_glWidget;
   bool _ownsGLWidget;
   QGLWidget *_glStereoWidget;
   bool _ownsGLStereoWidget;

   // a widget for text or image (used, for example, by POV-Ray)
   QLabel *_imageWidget;

   // custom background color
   osg::Vec4 _customColor;

   // create GL widget
   virtual QGLWidget* createGLWidget( void (*&activateFunc)( QWidget *w, osg::ref_ptr< osgViewer::GraphicsWindow > &gw ),
                                      QGLWidget *shareWidget, bool stereo );
   QGLWidget* initGLWidgetInternal( QGLWidget *shareWidget, bool stereo );

   // Main window's menu bar
   QMenuBar* menuBar;
      QMenu* menuFile;  // File menu
      QMenu* menuView;  // View menu
         QMenu* menuBackground;     // Background color menu
         QMenu* menuWindowSize;     // Window size menu
      QMenu* menuHelp;   // Help menu

   // Main window's head toolbar
   QToolBar* toolHead;

   // Main window's bottom toolbar
   QToolBar* toolBottom;

   // toolbar widgets
   QDoubleSpinBox *povScale;
   QCheckBox *povFastAntialias;


   //
   // ACTIONS
   //

   // File actions
   QAction* actionOpenModel;  // open model action
   QAction* actionQuit;       // application terminate action

   // View actions
   QAction* actionReloadModel;      // reloads the model from file
   QAction* actionLoadView;         // load view from file
   QAction* actionSaveView;         // save view to file

   // Action group for background color selection
   QActionGroup* backgroundActionGroup;
      QAction* actionBlack;         // Action for selecting black
      QAction* actionDarkGrey;      // Action for selecting dark grey
      QAction* actionGrey;          // Action for selecting grey
      QAction* actionLightGrey;     // Action for selecting light grey
      QAction* actionWhite;         // Action for selecting white
      QAction* actionGriseous;      // Action for selecting griseous
      QAction* actionTan;           // Action for selecting tan
      QAction* actionCustomColor;   // Action for selecting custom color
      QAction* actionSelectCustomColor;   // Custom color selection
      QAction* actionBackgroundImage;     // Background image selection

   // Action group for window size selection
   QActionGroup* windowSizeActionGroup;
      QAction* actionWindowSize1920x1080;
      QAction* actionWindowSize1600x900;
      QAction* actionWindowSize1366x768;
      QAction* actionWindowSize1280x1024;
      QAction* actionWindowSize1280x720;
      QAction* actionWindowSize1024x768;
      QAction* actionWindowSize800x600;

   // Scene actions
   QAction* actionPPL;              // enable/disable per-pixel lighting (in GUI: shadow mode)
   QAction* actionPovrayRendering;  // Povray rendering
   QAction* actionStereo;           // enable/disable stereo (QUAD_BUFFER)

   // Help actions
   QAction* actionShowLog;          // show log
   QAction* actionAbout;            // show about dialog
   QAction* actionShowSceneInfo;    // show scene details
   QAction* actionShowSystemInfo;   // show system details

   // Top toolbar
   QAction* actionDefaultView;      // axonometric default view
   QAction* actionZoomAll;          // move back until whole model is on the screen

   // Bottom toolbar
   QActionGroup* manipulatorGroup;  // action group for manipulators
   QAction* actionOrbitManip;       // set orbit manipulator
   QAction* actionFirstPersonManip; // set first-person maniopulator
   QAction* actionAxisVisible;      // make axis visible or not


   // PRIVATE METHODS
   //
   // create GUI elements
   void createOSGWindow();
   void createActions();
   void createMenu();
   void createToolbars();
   void createStatusBar();

public slots:
   void openDocument( LexolightsDocument *document, bool resetViewSettings );
   void openModel( QString fileName = "" );
   virtual void reloadModel();
   virtual void loadModel( QString fileName, bool resetViewSettings );
   void activeDocumentSceneChanged();
   virtual void defaultView();
   virtual void loadView();
   virtual void saveView();
   virtual void zoomAll();
   virtual void setBackgroundColor();
   virtual void selectCustomColor();
   void backgroundImage( bool show );
   void setCentralWidgetSize();
   virtual void setOrbitManipulator();
   virtual void setFirstPersonManipulator();
   void setAxisVisible( bool visible );
   void setPerPixelLighting( bool on );
   virtual void renderUsingPovray();
   void setStereoscopicRendering( bool on );
   void showAboutDlg();
   void showSceneInfo();
   void showSystemInfo();
   void showLog( bool visible );

public:
   bool isLogShown() const;

protected:
   virtual void keyPressEvent( QKeyEvent *event );
   virtual void keyReleaseEvent( QKeyEvent *event );
   virtual void closeEvent( QCloseEvent *event );
   virtual void dragEnterEvent( QDragEnterEvent *event );
   virtual void dropEvent( QDropEvent *event );
};


//
//  inline functions
//

inline const QGLWidget* MainWindow::getGLWidget() const  { return _glWidget; }
inline QGLWidget* MainWindow::getGLWidget()  { return _glWidget; }
inline QGLWidget* MainWindow::getGLStereoWidget()  { return _glStereoWidget; }
inline QLabel* MainWindow::getImageWidget()  { return _imageWidget; }
inline CentralContainer* MainWindow::getCentralContainer()  { return _centralContainer; }


#endif /* MAIN_WINDOW_H */
