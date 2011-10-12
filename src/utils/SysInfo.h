
/**
 * @file
 * System info namespace header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef SYS_INFO_H
#define SYS_INFO_H

#include <QString>
#include <QSize>


namespace SysInfo {

   std::string getGLSLVersion();
   int getGLInteger( int name, int index = 0 );
   QString getGLIntegerStr( int name, int index = 0 );
   void getDisplayAttributes( int *width, int *height, int *bpp, int *freq );

   QString getOpenGLVersionInfo();
   QString getOpenGLExtensionsInfo();
   QString getOpenGLLimitsInfo();
   QString getGLSLLimitsInfo();
   QString getVideoMemoryInfo();
   QString getScreenInfo( const QSize& sizeIfLog, const QSize& fullSize );

   QString getLexolightVersion();
   QString getOsgRuntimeVersion();
   QString getOsgCompileVersion();
   QString getQtRuntimeVersion();
   QString getQtCompileVersion();
   QString getLibInfo();
   QString getGraphicsDriverInfo();
}


#endif /* SYS_INFO_H */
