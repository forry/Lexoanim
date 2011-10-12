
/**
 * @file
 * CadworkReaderWriter class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include "CadworkReaderWriter.h"

using namespace osg;
using namespace osgDB;

REGISTER_OSGPLUGIN( Cadwork, CadworkReaderWriter )


/**
 * Constructor.
 */
CadworkReaderWriter::CadworkReaderWriter()
{
   supportsExtension( "ivx", "Open Inventor Ascii file format" );
   supportsExtension( "ivl", "Open Inventor models with light setup" );
}


/**
 * Destructor.
 */
CadworkReaderWriter::~CadworkReaderWriter()
{
}


void CadworkReaderWriter::createAliases()
{
   Registry::instance()->addFileExtensionAlias( "ivx", "iv" );
   Registry::instance()->addFileExtensionAlias( "ivl", "iv" );
}


ReaderWriter::ReadResult CadworkReaderWriter::readObject(
       const std::string &file, const Options *options ) const
{
   return readNode( file, options );
}


ReaderWriter::ReadResult CadworkReaderWriter::readNode(
       const std::string &file, const Options *options ) const
{
#if 0 // let's use istream readNode method instead of hacks
      // around string readNode method

   /**
    * ReaderWriterProtectedWorkaround class serves as a hack
    * for accessing ReaderWriter::supportsExtensions() method
    * because it is protected until 2.8.2 release and
    * 2.9.6 development release.
    */
   class ReaderWriterProtectedWorkaround : public osgDB::ReaderWriter
   {
   public:
      static void appendMyExtensions( ReaderWriter *rw ) {
         ReaderWriterProtectedWorkaround* workarounded = (ReaderWriterProtectedWorkaround*)rw;
         workarounded->supportsExtension( "ivx", "Open Inventor Ascii file format" );
      }
   };

   // get Inventor ReaderWriter
   ReaderWriter *rw = Registry::instance()->getReaderWriterForExtension( "iv" );
   if( !rw )
      return ReadResult::ReadResult( "Warning: Could not find "
            "Open Inventor plugin to handle model loading from "
            "iv and ivx files." );

   ReaderWriterProtectedWorkaround::appendMyExtensions( rw );
   return rw->readNode( file, options );

#else

   // verify the extension
   std::string ext = getLowerCaseFileExtension( file );
   if( !acceptsExtension( ext ) )
      return ReadResult::FILE_NOT_HANDLED;

   // find file
   std::string fileName = findDataFile( file, options );
   if( fileName.empty() )
      return ReadResult::FILE_NOT_FOUND;

   // append database path
   ref_ptr< Options > myOptions = options ? new Options( *options ) : new Options;
   myOptions->getDatabasePathList().push_front( getFilePath( fileName ));

   // load from ifstream
   osgDB::ifstream istream( fileName.c_str(), std::ios::in | std::ios::binary );
   return readNode( istream, myOptions );

#endif
}


ReaderWriter::ReadResult CadworkReaderWriter::readObject(
       std::istream &fin, const Options *options ) const
{
   return readNode( fin, options );
}


ReaderWriter::ReadResult CadworkReaderWriter::readNode(
       std::istream &fin, const Options *options ) const
{
   // get Inventor ReaderWriter
   ReaderWriter *rw = Registry::instance()->getReaderWriterForExtension( "iv" );
   if( !rw )
      return ReadResult( "Warning: Could not find "
            "Open Inventor plugin to handle loading of "
            "iv, ivx and ivl files." );

   // read the file by Inventor plugin
   return rw->readNode( fin, options );
}
