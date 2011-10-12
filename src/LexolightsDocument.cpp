
/**
 * @file
 * LexolightsDocument class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ConvertUTF>
#include <QDir>
#include <QCoreApplication>
#include <QEvent>
#include <QFileInfo>
#include <QFileSystemWatcher>
#if defined(__WIN32__) || defined(_WIN32)
# include <windows.h>
# include <process.h>
# include <direct.h>
# include <io.h>
# include <errno.h>
#else
# include <sys/stat.h>
# include <sys/types.h>
#endif
#include "LexolightsDocument.h"
#include "Lexolights.h"
#include "gui/MainWindow.h"
#include "utils/Log.h"
#include "utils/minizip/unzip.h"
#include "utils/SetAnisotropicFilteringVisitor.h"
#include "utils/TextureUnitsUsageVisitor.h"
#include "utils/TextureUnitMoverVisitor.h"

using namespace osg;
using namespace osgDB;
using namespace std;

// forward declarations
static bool removeDirRecursively( const QString &dirName );
static bool removeDirRecursively( const std::wstring &dirName );



/**
 * Constructor.
 */
LexolightsDocument::LexolightsDocument()
   : _openOpThread( NULL ),
     _asyncSuccess( false )
{
   _watcher = new QFileSystemWatcher;
   connect( _watcher, SIGNAL( fileChanged( const QString& ) ),
            this, SLOT( fileChanged( const QString& ) ), Qt::QueuedConnection );
}


/**
 * Destructor.
 */
LexolightsDocument::~LexolightsDocument()
{
   close();
   delete _watcher;
}


void LexolightsDocument::fileChanged( const QString &fileName )
{
   Log::notice() << "LexolightsDocument: Reloading file '" << fileName
                 << "' as it has been modified on the disk." << endl;
   openFile( fileName );
   emit sceneChanged();
}


bool LexolightsDocument::openFile( const QString &fileName )
{
   return openFile( fileName, false );
}


bool LexolightsDocument::openFileAsync( const QString &fileName )
{
   return openFile( fileName, true, false );
}


bool LexolightsDocument::openFileAsync( const QString &fileName, bool openInMainWindow, bool resetViewSettings )
{
   return openFile( fileName, true, openInMainWindow, resetViewSettings );
}


bool LexolightsDocument::isOpenInProgress()
{
   return _openOpThread != NULL;
}


bool LexolightsDocument::waitForOpenCompleted()
{
   if( _openOpThread )
   {
      // wait for thread
      _openOpThread->wait();

      // perform completion in main thread
      asyncOpenCompleted();
   }

   // return success or failure
   return _asyncSuccess;
}


bool LexolightsDocument::openFile( const QString &fileName, bool background, bool openInMainWindow, bool resetViewSettings )
{
   // close previous document (if any)
   close();

   // set file name and watcher
   _fileName = fileName;
   if( !fileName.isEmpty() )
      _watcher->addPath( fileName );

   // prepare OpenOperation
   ref_ptr< OpenOperation > openOperation = new OpenOperation;
   openOperation->fileName = fileName;

   // set variables
   _asyncSuccess = false;
   _openInMainWindow = openInMainWindow;
   _resetViewSettings = resetViewSettings;

   // open
   if( background )
   {
      _openOpThread = new OpenOpThread( this, openOperation );
      _openOpThread->start();
      return true;
   }
   else
   {
      bool r = openOperation->run();
      _originalScene = openOperation->getOriginalScene();
      _pplScene = openOperation->getPPLScene();
      return r;
   }
}


void LexolightsDocument::close()
{
   // finish async open
   waitForOpenCompleted();

   // purge scene graph
   _originalScene = NULL;
   _pplScene = NULL;

   // empty file name and watcher
   if( !_fileName.isEmpty() )
      _watcher->removePath( _fileName );
   _fileName.clear();

   // remove temporary directory
   if( !_unzipDir.isEmpty() )
   {
      // remove model directory
      if( removeDirRecursively( _unzipDir ) ) {
         OSG_INFO << "Temporary directory '" << _unzipDir
                  << "' removed successfully." << std::endl;
      } else {
         OSG_WARN << "Error when removing temporary directory '"
                  << _unzipDir << "'." << std::endl;
      }

      // reset string
      _unzipDir.clear();
   }
}


bool LexolightsDocument::OpenOperation::openModel()
{
   // clear result variables (just for sure)
   _unzipDir = "";
   _originalScene = NULL;
   _pplScene = NULL;


   // load the model
   Timer time;
   QByteArray fna( _modelFileName.toUtf8() );
   const char *fn = fna.data();
   _originalScene = osgDB::readNodeFile( fn );
   double loadingTime = time.time_m();

   if( !_originalScene.valid() ) {

      // error message
      Log::fatal() << QString( "Model %1 loading failed (operation completed "
                               "in %2ms)." ).arg( _modelFileName )
                                            .arg( loadingTime, 0, 'f', 2 ) << Log::endm;
      return false;
   }

   // log the success and the load time
   Log::notice() << QString( "Model %1 loading completed successfully "
                             "in %2ms." ).arg( _modelFileName )
                                         .arg( loadingTime, 0, 'f', 2 ) << Log::endm;


   // build KdTree
   time.setStartTick();
   osg::ref_ptr< KdTreeBuilder > builder = new KdTreeBuilder;
   _originalScene->accept( *builder );
   builder = NULL;
   Log::info() << QString( "KdTree built in %1ms (model %2)." )
                          .arg( time.time_m() )
                          .arg( _modelFileName ) << Log::endm;


   // reset time
   time.setStartTick();


   // set anisotropy filtering for textures
   SetAnisotropicFilteringVisitor safv( 32.f );
   _originalScene->accept( safv );


   // detect texture units usage
   TextureUnitsUsageVisitor tuuv;
   _originalScene->accept( tuuv );
   OSG_INFO << "TextureUnitUsageVisitor results:" << endl;
   for( unsigned int i=0; i<tuuv._attributesFound.size(); i++ )
      OSG_INFO << "   Texture unit " << i << " attribute " << (tuuv._attributesFound[i] ? "found" : "not found") << endl;
   for( unsigned int i=0; i<tuuv._modeOn.size(); i++ )
      OSG_INFO << "   Texture unit " << i << " mode " << (tuuv._modeOn[i] ? "on" : "off") << endl;
   if( tuuv._attributesFound.size() == 0 && tuuv._modeOn.size() == 0 )
      OSG_INFO << "   No texture units used." << endl;

   // if texture unit 0 is empty and other units are used,
   // move content of the unit 1 to the unit 0
   if( tuuv._attributesFound.size() >= 2 )
   {
      if( tuuv._modeOn.size() <= 0 || tuuv._modeOn[0] == false )
      {
         OSG_NOTICE << "Model uses " << osg::maximum( tuuv._attributesFound.size(), tuuv._modeOn.size() )
                    << " texturing units while the unit 0 is not used.\n"
                       "    Moving texture unit 1 content to texture unit 0." << endl;

         // move content of the texturing unit 1 to the unit 0
         TextureUnitMoverVisitor tumv( 1, 0 );
         _originalScene->accept( tumv );
      }
      else
         OSG_INFO << "Model uses " << osg::maximum( tuuv._attributesFound.size(), tuuv._modeOn.size() )
                  << " texturing units and texturing unit 0\n"
                     "   seems to be used. No content move is performed." << endl;

   }
   else
      OSG_INFO << "Model uses " << osg::maximum( tuuv._attributesFound.size(), tuuv._modeOn.size() )
               << " texturing units." << endl;


   // report time
   Log::info() << QString( "AnisotropicFiltering setup and TextureUnit usage check performed in %1ms (model %2)." )
                          .arg( time.time_m() )
                          .arg( _modelFileName ) << Log::endm;


   // shader conversion
   if( Lexolights::options()->no_conversion )

      _pplScene = NULL;

   else {

      // shadow model
      PerPixelLighting::ShadowTechnique shadowTechnique = Lexolights::options()->shadowTechnique;
      if( Lexolights::options()->no_shadows )
         shadowTechnique = PerPixelLighting::NO_SHADOWS;

      // convert to per-pixel-lit scene
      PerPixelLighting ppl;
      ppl.convert( _originalScene, shadowTechnique );
      _pplScene = ppl.getScene();

   }

   // scene export (debugging)
   if( Lexolights::options()->exportScene ) {

      // save scene for debugging purposes
      osgDB::writeNodeFile( *_originalScene, "originalScene.osgt" );
      osgDB::writeNodeFile( *_pplScene, "pplScene.osgt" );

   }

   return true;
}


bool LexolightsDocument::OpenOperation::openZip()
{
   osg::Timer time;
   _modelFileName = "";
   bool retValue = true;

   // create temp path
#if (defined(__WIN32__) || defined(_WIN32)) && !defined(__CYGWIN__)
   _unzipDir = QDir::tempPath() + "\\Lexolights-" + QString::number( _getpid() ) + '\\';
#else
   _unzipDir = QDir::tempPath() + "/Lexolights-" + QString::number( getpid() ) + '/';
#endif

   // create temp dir
   // (if old temp dir exists, remove it first and create a new one)
   QDir dir( _unzipDir );
   if( dir.exists() ) {
      Log::info() << "OpenZip: Removing old temporary directory: '" << _unzipDir << "'" << Log::endm;
      if( !removeDirRecursively( _unzipDir ) )
         Log::warn() << "OpenZip: Can not remove old temporary directory: '" << _unzipDir << "'" << Log::endm;
   }
   if( !dir.mkpath( "." ) ) {
      Log::fatal() << "OpenZip: Can not create temporary directory: '" << _unzipDir << "'" << Log::endm;
      return false;
   }


   //
   //  open zip and go to the first file
   //
   unz_file_info fileInfo;
   char *bufferski = NULL;
   unzFile handle;

   // open zip file
   handle = unzOpen( _zipFileName.toLocal8Bit().data() );
   if( !handle ) {
      Log::fatal() << "OpenZip: Can not open file: '" << _zipFileName << "'" << Log::endm;
      return false;
   }

   // go to the first file
   int e = unzGoToFirstFile( handle );
   if( e != UNZ_OK ) {
      Log::fatal() << "OpenZip: Can not go to first file inside zip file: '"
                   << _zipFileName << "'" << Log::endm;
      unzClose( handle );
      return false;
   }


   //
   //  go through the all files in the archive
   //
   do {

      // get filename size and alocate mem for it
      e = unzGetCurrentFileInfo( handle, &fileInfo, NULL, 0, NULL, 0, NULL, 0 );
      if( e != UNZ_OK ) {
         Log::fatal() << "OpenZip: Can not get current file info inside zip file: '"
                      << _zipFileName << "'" << Log::endm;
         unzClose( handle );
         return false;
      }

      uint fileNameBufSize = fileInfo.size_filename + 1;    // +1 because of terminating character '\0'
      char *fileNameBuf = new char[fileNameBufSize];

      // get the filename
      e = unzGetCurrentFileInfo( handle, &fileInfo, fileNameBuf, fileNameBufSize, NULL, 0, NULL, 0 );
      if( e != UNZ_OK ) {
         Log::fatal() << "OpenZip: Can not get current file name inside zip file: '"
                      << _zipFileName << "'" << Log::endm;
         unzClose( handle );
         delete[] fileNameBuf;
         return false;
      }

      // open file
      if( password.isEmpty() )
         e = unzOpenCurrentFile( handle );
      else
         e = unzOpenCurrentFilePassword( handle, password.toUtf8().data() );

      if( e != UNZ_OK ) {
         Log::fatal() << (password.isEmpty() ? "OpenZip: Can not open a file inside the zip file: '"
                                             : "OpenZip: Can not open a file inside the zip file with password: '" )
                      << _zipFileName << "'" << Log::endm;
         unzClose( handle );
         delete[] fileNameBuf;
         return false;
      }


      //
      // filename decoding and path conversion
      //
      QString unicodeFileName;
      if( fileInfo.flag & 2048 ) {
         // if bit 11 (dec value 2048) of general purpose flag is set, filenames are in UTF-8 encoding
         std::string nativeFileName = osgDB::convertFileNameToNativeStyle( std::string( fileNameBuf ) );
         unicodeFileName = QString::fromUtf8( nativeFileName.c_str() );
      }
      else {
         // else use local char set
         unicodeFileName = QString::fromLocal8Bit( fileNameBuf );
      }

      // log
      Log::info() << "OpenZip: Extracting '" << unicodeFileName << "'" << Log::endm;

      // alloc memory for read buffer
      bufferski = new char[fileInfo.uncompressed_size];
      if( !bufferski ) {
         Log::fatal() << "OpenZip: Can not allocate memory (" << fileInfo.uncompressed_size
                      << " bytes) to extract file '" << unicodeFileName
                      << "' inside zip file: '" << _zipFileName << "'" << Log::endm;
         unzCloseCurrentFile( handle );
         unzClose( handle );
         delete[] fileNameBuf;
         delete[] bufferski;
         return false;
      }

      // read to buffer
      while( !unzeof( handle ) ) {
         e = unzReadCurrentFile( handle, bufferski, fileInfo.uncompressed_size );
         if( e < 0 ) {
            Log::fatal() << "OpenZip: Error when extracting file '" << unicodeFileName
                         << "' from zip file: '" << _zipFileName << "'" << Log::endm;
            unzCloseCurrentFile( handle );
            unzClose( handle );
            delete[] fileNameBuf;
            delete[] bufferski;
            return false;
         }
      }

      // create directory structure
      if( unicodeFileName.contains( QDir::separator() ) ) {
         QString dir_path = unicodeFileName.section( QDir::separator(), 0, -2, QString::SectionSkipEmpty ); // without last section (without filename)
         if( !dir_path.isEmpty() )
            dir.mkpath( dir_path );
      }

      // open output file
      QString file_path = _unzipDir + QDir::separator() + unicodeFileName;
      QFile unzippedFile( file_path.replace( "\\", "/" ) );  // According to documentation: "QFile expects the file separator
                                             // to be '/' regardless of operating system. The use of other
                                             // separators (e.g., '\') is not supported."

      if( !unzippedFile.open( QIODevice::WriteOnly | QIODevice::Unbuffered ) ) {
         Log::fatal() << "OpenZip: Error while opening destination file '" << file_path
                        << "' extracted from zip file: '" << _zipFileName << "'" << Log::endm;
         unzCloseCurrentFile( handle );
         unzClose( handle );
         delete[] fileNameBuf;
         delete[] bufferski;
         return false;
      }

      // write decompressed data to output file
      qint64 writtenBytes = unzippedFile.write( bufferski, (qint64)fileInfo.uncompressed_size );
      if( writtenBytes < 0) {
         Log::fatal() << "OpenZip: Error while opening destination file '" << file_path
                        << "' extracted from zip file: '" << _zipFileName << "'" << Log::endm;
         unzCloseCurrentFile( handle );
         unzClose( handle );
         delete[] fileNameBuf;
         delete[] bufferski;
         return false;
      }


      // get file name to open
      std::string extension = osgDB::getFileExtension( unicodeFileName.toUtf8().data() );
      if( extension == "iv" || extension == "ivx" || extension == "ivl" )
         _modelFileName = unicodeFileName;


      // close output file
      unzippedFile.close();

      // close file in zip
      unzCloseCurrentFile( handle );

      // free read buffer
      delete[] bufferski;

      // free filename
      delete[] fileNameBuf;

      // go to next file in zip if there is one
      e = unzGoToNextFile( handle );
   } while( e != UNZ_END_OF_LIST_OF_FILE );


   // error check
   if( e != UNZ_END_OF_LIST_OF_FILE ) {
      Log::fatal() << "OpenZip: Can not move to the next file inside the zip file: '"
                   << fileName << "'" << Log::endm;
      unzClose( handle );
      return false;
   }

   // close zip file
   unzClose( handle );

   // log unzip success
   Log::info() << "OpenZip: File '" << fileName << "' unzipped to temporary folder '"
      << _unzipDir << "' in " << int( time.time_m() + .5 ) << "ms." << Log::endm;

   // open file
   if( !_modelFileName.isEmpty() )
   {
      _modelFileName = _unzipDir + _modelFileName;
      if( !openModel() ) {
         // error message provided by openModel()
         return false;
      }
      return true;
   }
   else
   {
      Log::fatal() << "OpenZip: No model file to open inside the zip file '" << fileName
                   << "'." << Log::endm;
      return false;
   }
}


bool LexolightsDocument::OpenOperation::run()
{
   Log::info() << "LexolightsDocument::OpenOperation: Open operation started for file:\n"
                  "   " << fileName << Log::endm;
   Timer time;

   // load file
   _success = true;
   QByteArray fna( fileName.toUtf8() );
   const char *fn = fna.data();
   std::string extension = osgDB::getFileExtension( fn );
   if( extension == "ivz" || extension == "ivzl" || extension == "zip" )
   {
      // decompress zip and look for iv, ivx, or ivl file to open it
      _zipFileName = fileName;
      if( !openZip() ) {
         Log::fatal() << "Error when opening file '" << fileName << "'." << Log::endm;
         _success = false;
      }
   }
   else
   {
      // iv, ivx, ivl, and any extension supported by OSG
      _modelFileName = fileName;
      if( !openModel() ) {
         Log::fatal() << "Error when opening file '" << fileName << "'." << Log::endm;
         _success = false;
      }
   }

   // log message
   if( _success )
      Log::info() << "LexolightsDocument::OpenOperation: Model " << fileName << " loaded in " << time.time_m() << "ms." << Log::endm;
   else
      Log::info() << "LexolightsDocument::OpenOperation: Failed to load model " << fileName << " (operation took " << time.time_m() << "ms)." << Log::endm;

   return _success;
}


LexolightsDocument::OpenOpThread::OpenOpThread( LexolightsDocument *parent, OpenOperation *openOp )
   : inherited( parent ),
     _openOp( openOp )
{
}


static int asyncOpenCompletedEventId = QEvent::registerEventType();


void LexolightsDocument::OpenOpThread::run()
{
   Timer t;
   Log::info() << "OpenOpThread: Open task started." << Log::endm;
   _openOp->run();
   Log::info() << "OpenOpThread: Open task finished." << Log::endm;
   Log::notice() << "Background loading thread for file " << _openOp->fileName << " finished in " << t.time_m() << "ms." << Log::endm;

   QEvent *event = new QEvent( (QEvent::Type)asyncOpenCompletedEventId );
   QCoreApplication::postEvent( this, event );
}


void LexolightsDocument::OpenOpThread::customEvent( QEvent *event )
{
   if( event->type() == asyncOpenCompletedEventId )
      dynamic_cast< LexolightsDocument* >( parent() )->asyncOpenCompleted();
}


void LexolightsDocument::asyncOpenCompleted()
{
   OpenOperation *openOp = _openOpThread->getOpenOperation();

   Log::notice() << "Background loading of file " << openOp->fileName << " performs final processing and synchronizing in the main thread." << std::endl;

   // copy data from OpenOpThread
   _asyncSuccess = openOp->getSuccess();
   _originalScene = openOp->getOriginalScene();
   _pplScene = openOp->getPPLScene();
   _unzipDir = openOp->getUnzipDir();

   // delete OpenOpThread
   //_openOpThread->deleteLater();
   delete _openOpThread;
   _openOpThread = NULL;

   // open in MainWindow if requested
   if( _openInMainWindow && _asyncSuccess )
      Lexolights::mainWindow()->openDocument( this, _resetViewSettings );
}


/** The function deletes everything inside the directory.
 *  It silently expects that fi parameter points to the directory
 *  and that the directory exists.*/
static void removeEverythingInDir( const QFileInfo &fi )
{
   QDir dir( fi.filePath() );
   QFileInfoList list = dir.entryInfoList();
   for( QFileInfoList::iterator it = list.begin();
        it != list.end(); it++ )
   {
      if( it->isDir() ) {
         if( it->fileName() == "." || it->fileName() == ".." )
            continue;
         removeEverythingInDir( *it );
         dir.rmdir( it->fileName() );
      }
      else
         dir.remove( it->fileName() );
   }
}


// return false if not success
static bool removeDirRecursively( const QString &dirName )
{
   // create valid file info
   QFileInfo fi( dirName );
   if( !fi.exists() )
      return false;

   // remove trailing (back)slash at the end of the path
   // (trailing (back)slash is indicated by empty fileName
   // and path containing everything)
   if( fi.fileName().isEmpty() )
   {
      QString path = fi.path();
      if( path.endsWith( '\\' ) || path.endsWith( '/' ) )
         path.chop( 1 );
      fi.setFile( path );
   }

   // remove dir
   if( fi.isDir() )
   {
      // remove content
      removeEverythingInDir( fi );

      // remove dir
      QDir parentDir( fi.path() );
      return parentDir.rmdir( fi.fileName() );
   }

   // if it is not a dir, remove it in a way that files are removed
   QDir parentDir( fi.path() );
   return parentDir.remove( fi.fileName() );
}


// return false if not success
static bool removeDirRecursively( const std::wstring &dirName )
{
#if defined(__WIN32__) || defined(_WIN32)
   // make sure about terminating slash
   std::wstring dirWithSlash( dirName );
   int l = dirWithSlash.length();
   if( l>0 && dirWithSlash[l-1] != L'\\')
      dirWithSlash += L'\\';

   // initialize iteration through the directory entities
   _wfinddata_t findData;
   intptr_t h = _wfindfirst( (dirWithSlash + L"*.*").c_str(), &findData );
   if( h == -1L )
   {
      OSG_WARN << "Can not iterate through entities of directory '"
               << dirWithSlash.c_str() << "'." << std::endl;
   }
   else
   {
      // loop through the directory entities
      while( true )
      {
         if( (findData.attrib & _A_SUBDIR) == 0 )
         {
            if( _wunlink( (dirWithSlash + findData.name).c_str() ) == -1 ) {
               OSG_WARN << "Can not remove temporary file '"
                        << (dirWithSlash + findData.name).c_str() << "'." << std::endl;
            }
         }
         else
         {
            if( wcscmp( findData.name, L"." ) != 0 &&
                wcscmp( findData.name, L".." ) != 0 )
            {
               removeDirRecursively( QString::fromStdWString( dirWithSlash + findData.name ) );
            }
         }

         // find the next entity
         if( _wfindnext( h, &findData ) == -1L ) {

            // no more items
            if( errno == ENOENT )
               break;

            // failure
            OSG_WARN << "Error when iterating items of directory '"
                     << dirWithSlash.c_str() << "'." << std::endl;
            break;
         }
      }

      // clean up after iteration
      if( _findclose( h ) == -1L ) {
         OSG_WARN << "Error when finishing iteration of directory '"
                  << dirName.c_str() << "'." << std::endl;
      }
   }

   // remove directory
   if( _wrmdir( dirName.c_str() ) == 0 )
      return true;

   // report error
   OSG_WARN << "Error when removing temporary directory '"
            << dirName.c_str() << "'." << std::endl;
#endif
   return false;
}
