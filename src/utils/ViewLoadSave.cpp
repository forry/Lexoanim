
/**
 * @file
 * View load and save functionality.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <QFile>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <osgGA/StandardManipulator>
#include "ViewLoadSave.h"

using namespace osg;


/**
 * Identifier of the ivv format version 2.
 */
#define IVV_2_IDENTIFIER "CADWORK_IVV_FORMAT_v2"

/**
 * Identifier of the ivv format version 3.
 */
#define IVV_3_IDENTIFIER "CADWORK_IVV_FORMAT_v3"

/**
 * Identifier of the ivv format version 4.
 */
#define IVV_4_IDENTIFIER "CADWORK_IVV_FORMAT_v4"



/**
 * Formats number to the given precision.
 *
 * @param f Float number.
 *
 * @return String that represents number.
 */
static QString formatNumber( const float f )
{
   return QString( "%1" ).arg( f, 0, 'f', 3 );
}


/**
 * Saves view configuration into the IVV file in format version 3.
 *
 * @param filename View filename
 * @param camera Camera whose view configuration will be saved
 * @param cameraManipulator Camera manipulator whose configuration will be saved
*/
int saveIVV( const QString& filename, const Camera& camera,
             const osgGA::StandardManipulator& cameraManipulator )
{
   if( filename.isEmpty() )
      return -1;

   // get camera information
   Vec3d eye, center, up;
   double fovy, tmp;
   float rot = 0.f;
   cameraManipulator.getTransformation( eye, center, up );
   if( !camera.getProjectionMatrixAsPerspective( fovy, tmp, tmp, tmp ) )
      return -1;

   // open file
   QFile f( filename );
   f.open( QIODevice::WriteOnly | QIODevice::Truncate );

   // write content
   f.write( QString( "%1\r\n\r\n" ).arg( IVV_3_IDENTIFIER ).toLocal8Bit() );

   f.write( QString( "Type: %1\r\n" ).arg( "PerspectiveCamera" ).toLocal8Bit() );
   f.write( QString( "Position: %1 %2 %3\r\n" ).arg( formatNumber( eye[0] ) )
                                               .arg( formatNumber( eye[1] ) )
                                               .arg( formatNumber( eye[2] ) ).toLocal8Bit() );
   f.write( QString( "LookAt: %1 %2 %3\r\n" ).arg( formatNumber( center[0] ) )
                                             .arg( formatNumber( center[1] ) )
                                             .arg( formatNumber( center[2] ) ).toLocal8Bit() );
   f.write( QString( "OpeningAngle: %1\r\n" ).arg( formatNumber( fovy ) ).toLocal8Bit() );
   f.write( QString( "Rotation: %1\r\n" ).arg( formatNumber( rot ) ).toLocal8Bit() );
   f.write( QString( "Info: %1\r\n" ).arg( "" ).toLocal8Bit() );

   // close file
   bool ok = ( f.error() == QFile::NoError );
   f.close();

   return ok ? 1 : -2;
}


/**
 * Loads the view data, updating camera and camera manipulator.
 *
 * @param filename View filename
 * @param camera Camera that will receive new view configuration
 * @param cameraManipulator Camera manipulator that will receive new view configuration
 */
int loadIVV( QString &filename, Camera &camera,
             osgGA::StandardManipulator& cameraManipulator )
{
   // open file
   QFile file( filename );
   file.open( QIODevice::ReadOnly );

   // read all data
   QRegExp endLine( "\r*\n+" );
   QStringList data = QString( file.readAll() ).split( endLine );
   if( data.count() <= 0 )
      return -2; // empty file

   // file format version
   Qt::CaseSensitivity cs = Qt::CaseSensitive;
   int version = 0;
   if( data[0].compare( IVV_2_IDENTIFIER ) == 0 )
      version = 2;
   else if( data[0].compare( IVV_3_IDENTIFIER ) == 0 )
      version = 3;
   else if( data[0].compare( IVV_4_IDENTIFIER ) == 0 ) {
      version = 4;
      cs = Qt::CaseInsensitive;
   }

   // unsupported versions
   if( version == 0 ) {
      file.close();
      return -3;
   }

   // variables
   QString type;
   Vec3d eye, center;
   double fovy = 0.;
   float rot = 0.f;
   QString viewName;
   bool ec = false;

   // read data
   QStringList line;
   QRegExp splitter("\\s+");
   int cnt = 0;
   for (int i=0; i<data.count(); i++)
   {
      if (data[i].isEmpty()) continue; // skip empty lines
      line = data[i].split(splitter, QString::SkipEmptyParts);
      cnt = line.count();

      // parse version 2 elements
      if( line[0].compare( "Type:", cs ) == 0 && cnt >= 2 )
         type = line[1];
      else if( line[0].compare( "Position:", cs ) == 0 && cnt >= 4 )
         eye = Vec3d( line[1].toFloat(), line[2].toFloat(), line[3].toFloat() );
      else if( line[0].compare( "LookAt:", cs ) == 0 && cnt >= 4 )
         center = Vec3d( line[1].toFloat(), line[2].toFloat(), line[3].toFloat() );
      else if( line[0].compare( "OpeningAngle:", cs ) == 0 && cnt >= 2 )
         fovy = line[1].toFloat();
      else if( line[0].compare( "Rotation:", cs ) == 0 && cnt >= 2 )
         rot = line[1].toFloat();

      else if( version >= 3 ) {

         // parse version 3 elements
         if( line[0].compare( "Info:", cs ) == 0 && cnt >= 2)  viewName = line[1];

         else if( version >= 4 ) {

            // parse version 4 elements
            if( line[0].compare( "EC:", cs ) == 0 && cnt >= 2 )
               ec = (bool)line[1].toInt();

         }
      }

      line.clear();
   }

   // set FOV
   if( type.compare( "PerspectiveCamera" ) == 0 ) {

      double tmp, ratio, zNear, zFar;
      camera.getProjectionMatrixAsPerspective( tmp, ratio, zNear, zFar );
      camera.setProjectionMatrixAsPerspective( fovy, ratio, zNear, zFar );

   } else {

      // unsupported camera type
      return -4;

   }

   // FIXME: There is EC correction missing here.
   // It should be done whenever ec variable is true. PCJohn-2010-06-10

   // set manipulator transformation
   cameraManipulator.setTransformation( eye, center, Vec3d( 0.,0.,1. ) );

   // clean up
   file.close();
   return 1;
}

int saveIVV( const QString& filename, /*const osg::Matrix& modelView*/ osg::Vec3 eye, osg::Vec3 center, const double &fovy  )
{
   if( filename.isEmpty() )
      return -1;

   // get camera information
   Vec3d /*eye, center,*/ up(0.0,0.0,1.0);
   //double fovy, tmp;
   float rot = 0.f;
   //cameraManipulator.getTransformation( eye, center, up );
   //modelView.getLookAt(eye, center, up);
   /*if( !camera.getProjectionMatrixAsPerspective( fovy, tmp, tmp, tmp ) )
      return -1;*/

   // open file
   QFile f( filename );
   f.open( QIODevice::WriteOnly | QIODevice::Truncate );

   // write content
   f.write( QString( "%1\r\n\r\n" ).arg( IVV_3_IDENTIFIER ).toLocal8Bit() );
   
   //little hack for Delata3D coordinate system
   f.write( QString( "Type: %1\r\n" ).arg( "PerspectiveCamera" ).toLocal8Bit() );
   f.write( QString( "Position: %1 %2 %3\r\n" ).arg( formatNumber( eye[0] ) )//little hack for Delata3D coordinate system
                                               .arg( formatNumber( eye[1] ) )
                                               .arg( formatNumber( eye[2] ) ).toLocal8Bit() );//little hack for Delata3D coordinate system
   f.write( QString( "LookAt: %1 %2 %3\r\n" ).arg( formatNumber( center[0] ) )//little hack for Delata3D coordinate system
                                             .arg( formatNumber( center[1] ) )
                                             .arg( formatNumber( center[2] ) ).toLocal8Bit() );//little hack for Delata3D coordinate system
   f.write( QString( "OpeningAngle: %1\r\n" ).arg( formatNumber( fovy ) ).toLocal8Bit() );
   f.write( QString( "Rotation: %1\r\n" ).arg( formatNumber( rot ) ).toLocal8Bit() );
   f.write( QString( "Info: %1\r\n" ).arg( "" ).toLocal8Bit() );

   // close file
   bool ok = ( f.error() == QFile::NoError );
   f.close();

   return ok ? 1 : -2;
}

int loadIVV( QString &filename, /*osg::Matrix& modelView*/osg::Vec3 &eye, osg::Vec3 &center, double& fovy )
{
   // open file
   QFile file( filename );
   file.open( QIODevice::ReadOnly );

   // read all data
   QRegExp endLine( "\r*\n+" );
   QStringList data = QString( file.readAll() ).split( endLine );
   if( data.count() <= 0 )
      return -2; // empty file

   // file format version
   Qt::CaseSensitivity cs = Qt::CaseSensitive;
   int version = 0;
   if( data[0].compare( IVV_2_IDENTIFIER ) == 0 )
      version = 2;
   else if( data[0].compare( IVV_3_IDENTIFIER ) == 0 )
      version = 3;
   else if( data[0].compare( IVV_4_IDENTIFIER ) == 0 ) {
      version = 4;
      cs = Qt::CaseInsensitive;
   }

   // unsupported versions
   if( version == 0 ) {
      file.close();
      return -3;
   }

   // variables
   QString type;
   //Vec3d eye, center;
   //double fovy = 0.;
   float rot = 0.f;
   QString viewName;
   bool ec = false;

   // read data
   QStringList line;
   QRegExp splitter("\\s+");
   int cnt = 0;
   for (int i=0; i<data.count(); i++)
   {
      if (data[i].isEmpty()) continue; // skip empty lines
      line = data[i].split(splitter, QString::SkipEmptyParts);
      cnt = line.count();

      // parse version 2 elements
      if( line[0].compare( "Type:", cs ) == 0 && cnt >= 2 )
         type = line[1];
      else if( line[0].compare( "Position:", cs ) == 0 && cnt >= 4 )
         eye = Vec3d( line[1].toFloat(), line[2].toFloat(), line[3].toFloat() );
      else if( line[0].compare( "LookAt:", cs ) == 0 && cnt >= 4 )
         center = Vec3d( line[1].toFloat(), line[2].toFloat(), line[3].toFloat() );
      else if( line[0].compare( "OpeningAngle:", cs ) == 0 && cnt >= 2 )
         fovy = line[1].toFloat();
      else if( line[0].compare( "Rotation:", cs ) == 0 && cnt >= 2 )
         rot = line[1].toFloat();

      else if( version >= 3 ) {

         // parse version 3 elements
         if( line[0].compare( "Info:", cs ) == 0 && cnt >= 2)  viewName = line[1];

         else if( version >= 4 ) {

            // parse version 4 elements
            if( line[0].compare( "EC:", cs ) == 0 && cnt >= 2 )
               ec = (bool)line[1].toInt();

         }
      }

      line.clear();
   }

   // clean up
   file.close();
   // set FOV
   //if( type.compare( "PerspectiveCamera" ) == 0 ) {

   //   double tmp, ratio, zNear, zFar;
   //   camera.getProjectionMatrixAsPerspective( tmp, ratio, zNear, zFar );
   //   camera.setProjectionMatrixAsPerspective( fovy, ratio, zNear, zFar );

   //} else {

   //   // unsupported camera type
   //   return -4;

   //}

   // FIXME: There is EC correction missing here.
   // It should be done whenever ec variable is true. PCJohn-2010-06-10

   // set manipulator transformation
   //cameraManipulator.setTransformation( eye, center, Vec3d( 0.,0.,1. ) );
   //modelView.makeIdentity();
   //little hack for Delata3D coordinate system
   //eye.set(eye.x(), -eye.y(), eye.z());
   //center.set(-center.x(), -center.y(), -center.z());
   //modelView.makeLookAt(eye, center, Vec3d( 0.,0.,1. ));

   return 1;
}
