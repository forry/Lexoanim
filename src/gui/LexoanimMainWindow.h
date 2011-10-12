#ifndef LEXOANIM_MAIN_WINDOW
#define LEXOANIM_MAIN_WINDOW

#include "MainWindow.h"
#include "lexoanim.h"


namespace dtABC{
   class Application;
}

/**
 * Lexoanim Application's main window.
 */
class LexoanimMainWindow : public MainWindow
{
   Q_OBJECT

   public:

   // constructor and destructor
   LexoanimMainWindow( QWidget* parent = 0, Qt::WFlags flags = 0, bool build = true );
   virtual ~LexoanimMainWindow();

   // create GUI
   virtual void buildGUI( bool buildGLWidget = true );

   virtual void setDeltaApp(dtABC::Application *deltaApp);

   virtual dtABC::Application *getDeltaApp();

protected:
   //virtual void createOSGWindow();
   virtual void createActions();
   virtual void createMenu();
   virtual void createToolbars();
   virtual void createStatusBar();

   
   //util
   virtual LexoanimApp *GetLexoanimApp(){ return dynamic_cast<LexoanimApp *> (getDeltaApp()); }
   
   ///Delta3D application
   dtABC::Application *_deltaApp;

public slots:
   //void openDocument( LexolightsDocument *document, bool resetViewSettings );
   //void openModel( QString fileName = "" );
   virtual void reloadModel();
   virtual void loadModel( QString fileName, bool resetViewSettings );
   //void activeDocumentSceneChanged();
   virtual void defaultView();
   virtual void loadView();
   virtual void saveView();
   virtual void zoomAll();
   virtual void setBackgroundColor();
   virtual void selectCustomColor();
   //void backgroundImage( bool show );
   //void setCentralWidgetSize();
   virtual void setOrbitManipulator();
   virtual void setFirstPersonManipulator();
   //void setAxisVisible( bool visible );
   //void setPerPixelLighting( bool on );
   virtual void renderUsingPovray();
   //void setStereoscopicRendering( bool on );
   //void showAboutDlg();
   //void showLog( bool visible );
};

#endif //LEXOANIM_MAIN_WINDOW