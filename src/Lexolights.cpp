
/**
 * @file
 * Lexolights class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/Timer>
#include <osgDB/ReadFile>
#include <osgGA/FirstPersonManipulator>
#include <osgGA/OrbitManipulator>
#include <osgQt/GraphicsWindowQt>
#include <cassert>
#include <QAction>
#include <QDir>
#if defined(__WIN32__) || defined(_WIN32)
# include <shlobj.h>
#endif
#include "Lexolights.h"
#include "LexolightsDocument.h"
#include "CadworkViewer.h"
#include "gui/MainWindow.h"
#include "gui/CentralContainer.h"
#include "lighting/ShadowVolume.h"
#include "utils/Log.h"
#include "utils/CadworkReaderWriter.h"
#include "utils/WinRegistry.h"

using namespace std;
using namespace osg;


bool Lexolights::_initialized = false;
MainWindow* Lexolights::g_mainWindow = NULL;
ref_ptr< CadworkViewer > Lexolights::g_viewer;
ref_ptr< LexolightsDocument > Lexolights::g_activeDocument;
Options *Lexolights::g_options = NULL;


/** \fn Lexolights::activeDocument()
 *
 * Returns the active document of the application.
 * The one that is displayed by the main window. */

/** \fn Lexolights::viewer()
 *
 * Returns the osgViewer::viewer derived class
 * that controls the view and cameras in the MainWindow. */

/** \fn Lexolights::mainWindow()
 *
 * Returns main window of the application. */

/** \fn Lexolights::options()
 *
 * Returns main window of the application. */


/**
 * Constructor.
 */
Lexolights::Lexolights( int &argc, char* argv[], bool initialize )
   : QApplication( argc, argv )
{
   // set application and organization name
   // (these values are used by QSettings)
   setOrganizationName( "Cadwork Informatik" );
   setOrganizationDomain( "www.cadwork.com" );
   setApplicationName( "Lexolights" );

   // Options (cmd-line,...)
   if( !options() ) {
      g_options = new Options( argc, argv );
      if( options()->exitTime == Options::AFTER_PARSING_CMDLINE )
         std::exit( 99 );
   }

   // call init
   if( initialize )
      init();
}


void Lexolights::init()
{
   // protect against multiple initializations
   if( _initialized )
      return;
   _initialized = true;

   // time of initialization start
   osg::Timer time;

   // open model asynchronously on background thread
   LexolightsDocument *startUpModel = NULL;
   if( !options()->startUpModelName.isEmpty() &&
       options()->exitTime != Options::BEFORE_GUI_CREATION &&
       !options()->no_threads )
   {
      Log::notice() << "Opening model " << options()->startUpModelName << " in background thread." << Log::endm;
      startUpModel = new LexolightsDocument;
      startUpModel->openFileAsync( options()->startUpModelName, true, true );
   }

   // check whether all supported file extensions are associated
   if( !options()->removeFileAssociations )
      if( options()->recreateFileAssociations || !checkFileAssociations())
         registerFileAssociations();
      else
         Log::notice() << "File associations check: All associations ok. No update required." << Log::endm;

   // remove file associations
   if( options()->removeFileAssociations )
      unregisterFileAssociations();

   // exit based on cmd-line options
   if( options()->exitTime == Options::BEFORE_GUI_CREATION )
      std::exit( 0 );

   // create Viewer
   Log::info() << "GUI building started..." << std::endl;
   g_viewer = new CadworkViewer( *options()->argumentParser );

   // set Viewer's threading model
   if( g_viewer->getThreadingModel() == osgViewer::ViewerBase::AutomaticSelection )
   {
#if defined(__WIN32__) || defined(_WIN32)
      // seems that multithreading is stable on Windows
      g_viewer->setThreadingModel( osgViewer::ViewerBase::CullDrawThreadPerContext );
      //g_viewer->setThreadingModel( osgViewer::ViewerBase::DrawThreadPerContext );
#else
      // multithreading is not stable on Linux when using osgQt (seen on Ubuntu 11.04, Qt 4.7.2)
      // (this expects to be improved with Qt 4.8)
      g_viewer->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
#endif
   }

   // set Viewer's run scheme (defaults to ON_DEMAND)
   g_viewer->setRunFrameScheme( options()->continuousUpdate ? osgViewer::ViewerBase::CONTINUOUS : osgViewer::ViewerBase::ON_DEMAND );

   // report errors of command line
   if( options()->reportRemainingOptionsAsUnrecognized() )
       std::exit( 99 );

   // create the main app window
   g_mainWindow = new MainWindow( NULL, Qt::WFlags(), false );
   g_mainWindow->buildGUI();

   // register Cadwork Inventor aliases (ivx, ivl)
   CadworkReaderWriter::createAliases();

   // report GUI build time
   // (the time includes parsing of command line options and updating of file associations)
   Log::notice() << QString( "GUI building completed in %1ms."
                           ).arg( time.time_m(), 0, 'f', 2 ) << Log::endm;

   if( options()->no_threads )
   {
      // open model
      if( !options()->startUpModelName.isEmpty() )
         mainWindow()->openModel( options()->startUpModelName );
   }
   else
      // wait for file loading completion
      if( startUpModel )
         startUpModel->waitForOpenCompleted();
}


void Lexolights::realize()
{
   viewer()->realize();

   // start POV-Ray rendering, if requested
   // note: this can be done even without realize, but probably only because
   //       Qt widgets are already shown
   if( options()->renderInPovray ) {
      QWidget *w = mainWindow()->getCentralContainer();
      viewer()->getCamera()->getGraphicsContext()->resized( w->x(), w->y(), w->width(), w->height() );
      viewer()->updateTraversal();
      mainWindow()->actionPovrayRendering->trigger();
   }
}


/**
 * Destructor.
 */
Lexolights::~Lexolights()
{
   delete g_mainWindow;
   g_mainWindow = NULL;
   g_viewer = NULL;
   g_activeDocument = NULL;
   delete g_options;
   g_options = NULL;
}


//
// Sources on UAC and application elevation:
//
// http://msdn.microsoft.com/en-us/library/bb530410.aspx
// http://support.microsoft.com/kb/981778
//
//
// Sources on file associations:
//
// http://msdn.microsoft.com/en-us/library/ee872121(v=VS.85).aspx
//

//
// Related registry keys (Win 7, Vista and XP):
// HKCR\[ext]
// HKCR\[AppProgID]
// HKCR\Applications\[executable]
// HKCR\SystemFileAssociations
// HKCU\Software\Classes\[ext]
// HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\[ext]
//
// seen on Windows 7 and Vista, not on XP:
// HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths
//

//
// Notes and experiences:
//
// HKEY_CLASSES_ROOT\.ext\OpenWithList & OpenWithProgIds take effect only when TWO and more applications
//    associate with the extension, otherwise OpenWith is not shown in context menu (seen on Windows 7
//    and XP)
//
// HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext\OpenWithProgids
//    tracks closely all the changes in HKEY_CLASSES_ROOT\.ext\OpenWithProgIds (is there a change callback?)
//    and adds values to this key (seen on Windows 7 and XP)
//    The "change callback" seems to be Windows Explorer that updates the registry on right-click
//    on the file (seen on Windows 7 and XP).
//
// The same "callback" seems to be implemented for
//    HKCR\Applications\[executable] that is created or updated on mouse-right click on a file
//    in Windows Explorer. (seen on Windows XP)
//
// Invalid entries are ignored (seen on Windows 7, Vista and XP)
//
// Explorer automatically reflects all the file association changes in the registry make by regedit
//    on Windows 7, Vista and XP.
//
// Checking "Always use the selected program to open this kind of file" checkbox makes Windows 7 to
//    create HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.iv\UserChoice
//    key that forces opening the file by particular program for current user. All other programs
//    are still available using "Open with".
//    On Windows XP, the same is achieved by creating a value Application in
//    HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.osg
//    and setting the value to the application name.
//
// Elevated process can not be started if it resides on non-local disk device. Seen on Windows Vista.
//
// Starting of elevated process using ShellExecuteEx tested on Windows 7/Vista/XP on admin account:
//    WinXP does not require elevation when using admin account.
//    Win7/Vista just ask user confirmation to launch the process with the elevated privileges.
//
// Starting elevated process using guest account:
//    WinXP gives a window to give user/password to start a process with different credentials.
//    Vista and 7 are expected to do the same.
//
// There may be an empty admin password issue (seen on WinXP):
//    If admin password is empty and guest account attempts to elevate
//    its process, an window appears asking about admin user name and password.
//    After giving the name and empty password, however, Windows complaints
//    that security policy does not allow to use admin account with empty password.
//    However, this is not our issue but Windows security policy issue.
//

#if defined(__WIN32__) || defined(_WIN32)

static QString osgAssociationID( "OpenSceneGraph.Scene" );
static QString ivAssociationID( "Inventor.Document" );
static QString getOpenCommand()
{
   QString exePath = QDir::toNativeSeparators( QApplication::applicationFilePath() );
   return "\"" + exePath + "\" \"%1\"";
}
static QString getIconPath()
{
   QString exePath = QDir::toNativeSeparators( QApplication::applicationFilePath() );
   return exePath + ",0";
}


// check whether extension is registered
static bool checkAssociation( const QString& ext, const QString& progID )
{
   // prepare strings
   QString openCmd( getOpenCommand() );

   // extension
   if( WinRegistry::getString( HKEY_CLASSES_ROOT, ext, NULL ) != progID )
      return false;
   if( !WinRegistry::exists( HKEY_CLASSES_ROOT, ext + "\\OpenWithProgIds", progID ) )
      return false;
   if( !WinRegistry::exists( HKEY_CLASSES_ROOT, ext + "\\OpenWithList\\lexolights.exe", NULL ) )
      return false;

   // executable
   if( WinRegistry::getQString( HKEY_CLASSES_ROOT, "Applications\\lexolights.exe\\shell\\Open",
       NULL ) != QString( "Lexolights" ) )
      return false;
   if( WinRegistry::getQString( HKEY_CLASSES_ROOT, "Applications\\lexolights.exe\\shell\\Open\\command",
       NULL ).compare( openCmd, Qt::CaseInsensitive ) != 0 )
      return false;

   // ProgID
   if( WinRegistry::getString( HKEY_CLASSES_ROOT, progID, NULL ) != QString( "Lexolights" ) )
      return false;
   if( WinRegistry::getString( HKEY_CLASSES_ROOT, progID + "\\shell\\Open", NULL) != QString( "Lexolights" ) )
      return false;
   if( WinRegistry::getString( HKEY_CLASSES_ROOT, progID + "\\shell\\Open\\command",
                               NULL).compare( openCmd, Qt::CaseInsensitive ) != 0 )
      return false;
   if( WinRegistry::getString( HKEY_CLASSES_ROOT, progID + "\\DefaultIcon",
                               NULL).compare( getIconPath(), Qt::CaseInsensitive ) != 0 )
      return false;

   // everything registered => return true
   return true;
}


static void setAssociation( const QString& ext, const QString& progID )
{
   // prepare strings
   QString openCmd( getOpenCommand() );
   QString iconPath( getIconPath() );

   // extension
   WinRegistry::setString( HKEY_CLASSES_ROOT, ext, NULL, progID );
   WinRegistry::setString( HKEY_CLASSES_ROOT, ext + "\\OpenWithProgIds", progID, "" );
   WinRegistry::setString( HKEY_CLASSES_ROOT, ext + "\\OpenWithList\\lexolights.exe", NULL, "" );

   // executable
   WinRegistry::setString( HKEY_CLASSES_ROOT, "Applications\\lexolights.exe\\shell\\Open", NULL, "Lexolights" );
   WinRegistry::setString( HKEY_CLASSES_ROOT, "Applications\\lexolights.exe\\shell\\Open\\command", NULL, openCmd );

   // ProgID
   WinRegistry::setString( HKEY_CLASSES_ROOT, progID, NULL, "Lexolights" );
   WinRegistry::setString( HKEY_CLASSES_ROOT, progID + "\\shell\\Open", NULL, "Lexolights" );
   WinRegistry::setString( HKEY_CLASSES_ROOT, progID + "\\shell\\Open\\command", NULL, openCmd );
   WinRegistry::setString( HKEY_CLASSES_ROOT, progID + "\\DefaultIcon", NULL, iconPath );
}


static void removeAssociation( const QString& ext, const QString& progID )
{
   // check whether open command contains lexolights.exe
   QString programName = QFileInfo( QApplication::applicationFilePath() ).baseName();
   if( WinRegistry::getString( HKEY_CLASSES_ROOT, progID + "\\shell\\Open\\command",
                               NULL).contains( programName, Qt::CaseInsensitive ) ) {

      // ProgID\shell\Open
      WinRegistry::removeKey( HKEY_CLASSES_ROOT, progID + "\\shell\\Open", true );
      WinRegistry::removeKey( HKEY_CLASSES_ROOT, progID + "\\shell", false );
   }

   // check whether DefaultIcon points to the icon inside lexolights.exe
   if( WinRegistry::getString( HKEY_CLASSES_ROOT, progID + "\\DefaultIcon",
                               NULL).contains( programName, Qt::CaseInsensitive ) ) {

      // ProgID\\DefaultIcon
      WinRegistry::removeKey( HKEY_CLASSES_ROOT, progID + "\\DefaultIcon", true );
   }

   // ProgID
   if( WinRegistry::getQString( HKEY_CLASSES_ROOT, progID, "" )
       .compare( "Lexolights", Qt::CaseInsensitive ) == 0 )
      WinRegistry::removeValue( HKEY_CLASSES_ROOT, progID, NULL );
   bool progIdRemoved = WinRegistry::removeKey( HKEY_CLASSES_ROOT, progID, false );

   // executable
   WinRegistry::removeKey( HKEY_CLASSES_ROOT, "Applications\\lexolights.exe", true );

   // extension
   WinRegistry::removeKey( HKEY_CLASSES_ROOT, ext + "\\OpenWithList\\lexolights.exe", true );
   WinRegistry::removeKey( HKEY_CLASSES_ROOT, ext + "\\OpenWithList", false );
   WinRegistry::removeValue( HKEY_CLASSES_ROOT, ext + "\\OpenWithProgIds", progID );
   WinRegistry::removeKey( HKEY_CLASSES_ROOT, ext + "\\OpenWithProgIds", false );
   if( WinRegistry::getQString( HKEY_CLASSES_ROOT, ext, "" ) == progID )
      if( progIdRemoved ) {
         WinRegistry::removeValue( HKEY_CLASSES_ROOT, ext, NULL );
         WinRegistry::removeKey( HKEY_CLASSES_ROOT, ext, false );
      }
}


/** The method launches Lexolights with elevated privileges.
    It returns 0 on success. On failure, it returns error code given by GetLastError().*/
int Lexolights::startElevated( const QString& params )
{
   QString verb( "runas" );
   QString exePath = QDir::toNativeSeparators( applicationFilePath() );

   SHELLEXECUTEINFOW sei = { sizeof(sei) };
   sei.lpVerb = (LPCWSTR)verb.utf16();
   sei.lpFile = (LPCWSTR)exePath.utf16();
   sei.lpParameters = (LPCWSTR)params.utf16();
   sei.hwnd = NULL;
   sei.nShow = SW_SHOWDEFAULT;

   if( ShellExecuteExW( &sei ) )
      return 0;
   else
      return GetLastError();
}

#endif


/**
 * Check whether registry file associations are properly registered.
 */
bool Lexolights::checkFileAssociations()
{
#if defined(__WIN32__) || defined(_WIN32)

   // check whether .osg extension is registered
   if( !checkAssociation( ".osg", osgAssociationID ) ) {
      WinRegistry::getError();
      return false;
   }

   // check whether .ivl extension is registered
   if( !checkAssociation( ".ivl", osgAssociationID ) ) {
      WinRegistry::getError();
      return false;
   }

   // FIXME: include .iv and .ivx extensions as well, possibly by OpenWith

#endif

   return true;
}


/**
 * Create *.osg and *.iv file associations.
 * This may require spawning slave process with elevated privileges on Windows platform.
 */
void Lexolights::registerFileAssociations()
{
#if defined(__WIN32__) || defined(_WIN32)

   // clear previous errors
   if( WinRegistry::getError() != 0 )
      Log::warn() << "Pending error in WinRegistry." << Log::endm;


   // Lexolights .osg file association
   setAssociation( ".osg", osgAssociationID );

   // Lexolights .iv file association
   setAssociation( ".ivl", osgAssociationID );

   // notify about file associations change
   SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

   // problem during association update?
   int e = WinRegistry::getError();
   if( e != 0) {
      if( e != ERROR_ACCESS_DENIED )

         // unknown error
         Log::warn() << "Updating file associations failed.\n"
                     << "Registry error code: " << e << Log::endm;

      else {

         // insufficient access rights
         if( options()->elevatedProcess )

            // there was already attempt to elevate proces
            // => make error message only
            Log::fatal() << "Failed to update file associations.\n"
                         << "Reason: Can not get sufficient access rights." << Log::endm;

         else {

            Log::info() << "Updating registry requires administative privileges.\n"
                           "Starting elevated process..." << Log::endm;

            // launch itself as administrator
            int r = startElevated( "--install-elevated" );

            if( r == 0 )

               // elevated process successfully started
               Log::notice() << "Updating file associations: Elevated process successfully started." << Log::endm;

            else {

               if( r == ERROR_CANCELLED )
               {
                  // the user refused to allow privileges elevation
                  Log::fatal() << "Can not update file extension associations.\n"
                               << "Reason: Can not start child process with administrative privileges." << Log::endm;
               }
               else
               {
                  // unknown error
                  Log::fatal() << "Can not update file extension associations.\n"
                               << "Error code: " << r << Log::endm;
               }
            }
         }
      }
   }
#endif
}


/**
 * Remove *.osg and *.iv file associations.
 * The operation may require to spawn slave process with elevated privileges on Windows platform.
 */
void Lexolights::unregisterFileAssociations()
{
#if defined(__WIN32__) || defined(_WIN32)

   // clear previous errors
   if( WinRegistry::getError() != 0 )
      Log::warn() << "Pending error in WinRegistry." << Log::endm;

   // .iv
   removeAssociation( ".ivl", osgAssociationID );

   // .osg
   removeAssociation( ".osg", osgAssociationID );

   // notify about file associations change
   SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

   // problem during association update?
   int e = WinRegistry::getError();
   if( e == 0)

      // registry reports no error, so all remove operations are expected to be completed
      Log::notice() << "File associations removed successfully." << Log::endm;

   else
   {
      if( e != ERROR_ACCESS_DENIED )

         // unknown error
         Log::warn() << "Removing file associations failed.\n"
                        << "Registry error code: " << e << Log::endm;

      else {

         // insufficient access rights
         if( options()->elevatedProcess )

            // there was already attempt to elevate proces
            // => make error message only
            Log::fatal() << "Failed to remove file associations.\n"
                         << "Reason: Can not get sufficient access rights." << Log::endm;

         else {

            Log::info() << "Updating registry requires administative privileges.\n"
                           "Starting elevated process..." << Log::endm;

            // launch itself as administrator
            int r = startElevated( "--uninstall-elevated" );

            if( r == 0 )

               // elevated process successfully started
               Log::notice() << "Removing file associations: Elevated process successfully started. " << Log::endm;

            else {

               if( r == ERROR_CANCELLED )
               {
                  // the user refused to allow privileges elevation
                  Log::fatal() << "Failed to remove file extension associations.\n"
                               << "Reason: Can not start child process with administrative privileges." << Log::endm;
               }
               else
               {
                  // unknown error
                  Log::fatal() << "Failed to remove file extension associations.\n"
                               << "Error code: " << r << Log::endm;
               }
            }
         }
      }
   }
#endif
}
