
/**
 * @file
 * System info namespace implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include <QString>
#if defined(__WIN32__) || defined(_WIN32)
# include <windows.h>
#else
# include <X11/Xlib.h>
# include <X11/extensions/xf86vmode.h>
# include <GL/glx.h>
# define WINAPI
#endif
#include <GL/gl.h>
#include <cassert>
#include <osg/Version>
#include "SysInfo.h"
#include "utils/Log.h"
#include "utils/WinRegistry.h"

#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

#ifndef GL_MAX_TEXTURE_COORDS
#define GL_MAX_TEXTURE_COORDS 0x8871
#endif

#ifndef GL_MAX_TEXTURE_UNITS
#define GL_MAX_TEXTURE_UNITS 0x84E2
#endif

#ifndef GL_MAX_3D_TEXTURE_SIZE
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#endif

#ifndef GL_MAX_CUBE_MAP_TEXTURE_SIZE
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#endif

#ifndef GL_MAX_VERTEX_ATTRIBS
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#endif

#ifndef GL_MAX_VERTEX_UNIFORM_COMPONENTS
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS 0x8B4A
#endif

#ifndef GL_MAX_VARYING_FLOATS
#define GL_MAX_VARYING_FLOATS 0x8B4B
#endif

#ifndef GL_MAX_FRAGMENT_UNIFORM_COMPONENTS
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#endif

#ifndef GL_MAX_DRAW_BUFFERS
#define GL_MAX_DRAW_BUFFERS 0x8824
#endif

#ifndef GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#endif

#ifndef GL_MAX_TEXTURE_IMAGE_UNITS
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#endif

#ifndef GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#endif

#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#endif

#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#endif

#ifndef GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#endif

#ifndef GL_TEXTURE_FREE_MEMORY_ATI
#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC
#endif

#ifndef WGL_GPU_RAM_AMD
#define WGL_GPU_RAM_AMD 0x21A3
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif



static bool supportsExtension( const char *extension )
{

   // get extension string
   const char *extensions = (const char*)glGetString( GL_EXTENSIONS );
   if( !extensions )
      return false;

   // search in extension string
   int len = strlen( extension );
   const char *s = extensions;
   do {

      // search for the occurence
      s = strstr( s, extension );

      if( s == NULL ) {

         // end of extension string reached
         return false;

      }

      // process the occurence
      const char *endch = s + len;
      if( (*endch == ' ' || *endch == '\n' || *endch == 0 ) &&
          (s == extensions || *(s-1) == ' ' ||*(s-1) == '\n') ) {

         // extension found
         return true;

      }

      // move behind the current occurence
      s = endch;

   } while (true);

}


std::string SysInfo::getGLSLVersion()
{
   bool glslSupported = supportsExtension( "GL_ARB_shading_language_100" );

   if( !glslSupported )
      return "not supported";
   else {
      GLenum e = glGetError();
      assert(e==GL_NO_ERROR && "OpenGL is in error state.");
      const char *s = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
      e = glGetError();
      if (e == GL_INVALID_ENUM)
         return "1.00";
      else
         return s;
   }
}


int SysInfo::getGLInteger( int name, int index )
{
   // make sure that there is no OpenGL error
   GLenum e = glGetError();
   assert( e == GL_NO_ERROR && "OpenGL is in error state." );

   // get integer
   GLint value[4];
   assert( index >= 0 && index < 4 && "Index out of bounds." );
   glGetIntegerv( name, value );

   // make sure that GL error was not raised
   e = glGetError();
   if( e == GL_INVALID_ENUM )
      return -1;
   else
      if( e == GL_NO_ERROR )
         return value[index];
      else
         return -2;
}


QString SysInfo::getGLIntegerStr( int name, int index )
{
   int r = getGLInteger( name, index );
   if( r >= 0 )
      return QString::number( r );
   else
      if( r == -1 )
         return QString( "not supported" );
      else
         return QString( "error reading value" );
}


void SysInfo::getDisplayAttributes( int *width, int *height,
                                   int *bpp, int *freq )
{
#if defined(__WIN32__) || defined(_WIN32)

   DEVMODE dm;
   dm.dmSize = sizeof( dm );
   dm.dmDriverExtra = 0;
   if( !EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ))
      assert( 0 && "EnumDisplaySettings() failed." );

   if( width )  *width  = dm.dmPelsWidth;
   if( height ) *height = dm.dmPelsHeight;
   if( bpp )    *bpp    = dm.dmBitsPerPel;
   if( freq )   *freq   = dm.dmDisplayFrequency;

#else

   Display *display = XOpenDisplay( 0 );
   int screenNumber = DefaultScreen( display );

# if 0

   // using Xlib functions (did not know how to get frequency)
   Screen *screen = XScreenOfDisplay( display, screenNumber );
   if( width )  *width  = XWidthOfScreen( screen );
   if( height ) *height = XHeightOfScreen( screen );
   if( bpp )    *bpp    = XDefaultDepthOfScreen( screen );
   if( freq )   *freq   = 0;

# else

   // using XFree86-VidModeExtension
   XF86VidModeModeLine ml;
   int dotClock;
   XF86VidModeGetModeLine( display, screenNumber, &dotClock, &ml );
   Screen *screen = XScreenOfDisplay( display, screenNumber );
   if( ml.privsize != 0 )  XFree( ml.c_private );

   if( width )  *width  = ml.hdisplay;
   if( height ) *height = ml.vdisplay;
   if( bpp )    *bpp    = XDefaultDepthOfScreen( screen );
   if( freq )   *freq   = ml.htotal && ml.vtotal ?
                           dotClock*1000 / ( ml.htotal*ml.vtotal ) : 0;
# endif

   XCloseDisplay( display );

#endif
}


QString SysInfo::getOpenGLVersionInfo()
{
   return QString(
         "OpenGL version:\n"
         "Vendor: %1\n"
         "Renderer: %2\n"
         "Version: %3\n"
         "GLSL version: %4"
         )
         .arg( (const char*)glGetString( GL_VENDOR ) )
         .arg( (const char*)glGetString( GL_RENDERER ) )
         .arg( (const char*)glGetString( GL_VERSION ) )
         .arg( getGLSLVersion().c_str() );
}


QString SysInfo::getOpenGLExtensionsInfo()
{
   return QString(
         "OpenGL extensions:\n"
         "(Renderer: %1, Version: %2)\n"
         "%3"
         )
         .arg( (const char*)glGetString( GL_RENDERER ) )
         .arg( (const char*)glGetString( GL_VERSION ) )
         .arg( (const char*)glGetString( GL_EXTENSIONS ) );
}


QString SysInfo::getOpenGLLimitsInfo()
{
   return QString(
      "OpenGL version: %1\n"
      "MAX_LIGHTS: %2\n"
      "MAX_CLIP_PLANES: %3\n"
      "MAX_TEXTURE_MAX_ANISOTROPY: %4\n"
      "MAX_TEXTURE_COORDS: %5\n"
      "MAX_TEXTURE_UNITS: %6\n"
      "MAX_TEXTURE_SIZE: %7\n"
      "MAX_3D_TEXTURE_SIZE: %8\n"
      "MAX_CUBE_MAP_TEXTURE_SIZE: %9\n"
      "MAX_VIEWPORT_DIMS: %10x%11"
      )
      .arg( (const char*)glGetString( GL_VERSION ) )
      .arg( getGLIntegerStr( GL_MAX_LIGHTS ) )
      .arg( getGLIntegerStr( GL_MAX_CLIP_PLANES ) )
      .arg( getGLIntegerStr( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT ) )
      .arg( getGLIntegerStr( GL_MAX_TEXTURE_COORDS ) )
      .arg( getGLIntegerStr( GL_MAX_TEXTURE_UNITS ) )
      .arg( getGLIntegerStr( GL_MAX_TEXTURE_SIZE ) )
      .arg( getGLIntegerStr( GL_MAX_3D_TEXTURE_SIZE ) )
      .arg( getGLIntegerStr( GL_MAX_CUBE_MAP_TEXTURE_SIZE ) )
      .arg( getGLIntegerStr( GL_MAX_VIEWPORT_DIMS, 0 ) )
      .arg( getGLIntegerStr( GL_MAX_VIEWPORT_DIMS, 1 ) );
}


QString SysInfo::getGLSLLimitsInfo()
{
   return QString(
      "GLSL version: %1\n"
      "Vertex shader limits:\n"
      "MAX_VERTEX_ATTRIBS: %2\n"
      "MAX_VERTEX_UNIFORM_COMPONENTS: %3\n"
      "Fragment shader limits:\n"
      "MAX_VARYING_FLOATS: %4\n"
      "MAX_FRAGMENT_UNIFORM_COMPONENTS: %5\n"
      "MAX_DRAW_BUFFERS: %6\n"
      "Texturing limits:\n"
      "MAX_TEXTURE_COORDS: %7\n"
      "MAX_TEXTURE_UNITS: %8\n"
      "MAX_VERTEX_TEXTURE_IMAGE_UNITS: %9\n"
      "MAX_TEXTURE_IMAGE_UNITS: %10\n"
      "MAX_COMBINED_TEXTURE_IMAGE_UNITS: %11"
      )
      // GLSL version
      .arg( getGLSLVersion().c_str() )
      // Vertex shader
      .arg( getGLIntegerStr( GL_MAX_VERTEX_ATTRIBS ) )
      .arg( getGLIntegerStr( GL_MAX_VERTEX_UNIFORM_COMPONENTS ) )
      // Fragment shader
      .arg( getGLIntegerStr( GL_MAX_VARYING_FLOATS ) )
      .arg( getGLIntegerStr( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS ) )
      .arg( getGLIntegerStr( GL_MAX_DRAW_BUFFERS ) )
      // Texturing limits
      .arg( getGLIntegerStr( GL_MAX_TEXTURE_COORDS ) )
      .arg( getGLIntegerStr( GL_MAX_TEXTURE_UNITS ) )
      .arg( getGLIntegerStr( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS ) )
      .arg( getGLIntegerStr( GL_MAX_TEXTURE_IMAGE_UNITS ) )
      .arg( getGLIntegerStr( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS ) );
}


QString SysInfo::getScreenInfo( const QSize& sizeIfLog, const QSize& fullSize )
{
   int w = 0;
   int h = 0;
   int bpp = 0;
   int freq = 0;
   getDisplayAttributes( &w, &h, &bpp, &freq );

   return QString(
         "Screen info:\n"
         "Mode line (w x h : bpp @ freq): %1x%2:%3@%4\n"
         "Rendering area size: %5x%6 (without log window: %7x%8)\n"
         "Color bits (RGBA): %9,%10,%11,%12\n"
         "Depth and stencil bits: %13,%14\n"
         "Accumulation bits: %15,%16,%17,%18"
         )
         .arg( w )
         .arg( h )
         .arg( bpp )
         .arg( freq )
         .arg( sizeIfLog.width() )
         .arg( sizeIfLog.height() )
         .arg( fullSize.width() )
         .arg( fullSize.height() )
         .arg( getGLIntegerStr( GL_RED_BITS ) )
         .arg( getGLIntegerStr( GL_GREEN_BITS ) )
         .arg( getGLIntegerStr( GL_BLUE_BITS ) )
         .arg( getGLIntegerStr( GL_ALPHA_BITS ) )
         .arg( getGLIntegerStr( GL_DEPTH_BITS ) )
         .arg( getGLIntegerStr( GL_STENCIL_BITS ) )
         .arg( getGLIntegerStr( GL_ACCUM_RED_BITS ) )
         .arg( getGLIntegerStr( GL_ACCUM_GREEN_BITS ) )
         .arg( getGLIntegerStr( GL_ACCUM_BLUE_BITS ) )
         .arg( getGLIntegerStr( GL_ACCUM_ALPHA_BITS ) );
}


QString SysInfo::getLexolightVersion()
{
   return QString( LEXOLIGHTS_VERSION_MAJOR
                   "." LEXOLIGHTS_VERSION_MINOR );
}


QString SysInfo::getOsgRuntimeVersion()
{
   return QString( osgGetVersion() ) +
          "  (SOVERSION: " + osgGetSOVersion() + ")";
}


QString SysInfo::getOsgCompileVersion()
{
   return QString::number( OPENSCENEGRAPH_MAJOR_VERSION ) + "." +
          QString::number( OPENSCENEGRAPH_MINOR_VERSION ) + "." +
          QString::number( OPENSCENEGRAPH_PATCH_VERSION ) + "  (SOVERSION: " +
          QString::number( OPENSCENEGRAPH_SOVERSION ) + ")";
}


QString SysInfo::getQtRuntimeVersion()
{
   return QString( qVersion() );
}


QString SysInfo::getQtCompileVersion()
{
   return QString( QT_VERSION_STR );
}


QString SysInfo::getLibInfo()
{
   return QString( "Lexolight version: " )   + getLexolightVersion() +
          QString( "\nOSG runtime version: " ) + getOsgRuntimeVersion() +
          QString( "\nOSG compile version: " ) + getOsgCompileVersion() +
          QString( "\nQt runtime version:  " ) + getQtRuntimeVersion() +
          QString( "\nQt compile version:  " ) + getQtCompileVersion();
}


QString SysInfo::getVideoMemoryInfo()
{
   // GL_NVX_gpu_memory_info
   // according to http://www.geeks3d.com/20100120/nvidia-r196-21-opengl-opencl-and-cuda-details-geforce-9600-gt/
   // this extension is available since NVIDIA display driver release R196.21
   QString s = QString( "Renderer: %1" ).arg( (const char*)glGetString( GL_RENDERER ) );

   s += "\nGL_NVX_gpu_memory_info: ";
   if( supportsExtension( "GL_NVX_gpu_memory_info" ) )
      s += QString( "total: %1MB, dedicated: %2MB, available: %3MB" )
                   .arg( (getGLInteger( GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX )   + 512) / 1024 )
                   .arg( (getGLInteger( GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX )         + 512) / 1024 )
                   .arg( (getGLInteger( GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX ) + 512) / 1024 );
   else
      s += QString( "not supported" );

   // WGL_AMD_gpu_association
   // extension seems to be introduced by Catalyst 9.6
   // worth to note is WGL_AMDX_gpu_association extension that has very similar name
   // but it is still undocumented until today (2010-06-18)
   s += "\nWGL_AMD_gpu_association: ";
   if( supportsExtension( "WGL_AMD_gpu_association" ) ) {

      typedef GLuint (WINAPI * PFNWGLGETGPUIDSAMDPROC)( GLuint maxCount, GLuint *ids );
      typedef GLint (WINAPI * PFNWGLGETGPUINFOAMDPROC)( GLuint id, int property, GLenum dataType,
                                                        GLuint size, void *data );
      PFNWGLGETGPUIDSAMDPROC  my_glGetGPUIDsAMD;
      PFNWGLGETGPUINFOAMDPROC my_glGetGPUInfoAMD;
#if defined(__WIN32__) || defined(_WIN32)
      my_glGetGPUIDsAMD  = PFNWGLGETGPUIDSAMDPROC( wglGetProcAddress( "wglGetGPUIDsAMD" ) );
      my_glGetGPUInfoAMD = PFNWGLGETGPUINFOAMDPROC( wglGetProcAddress( "wglGetGPUInfoAMD" ) );
#else
      my_glGetGPUIDsAMD  = PFNWGLGETGPUIDSAMDPROC( glXGetProcAddress( (GLubyte*)"glXGetGPUIDsAMD" ) );
      my_glGetGPUInfoAMD = PFNWGLGETGPUINFOAMDPROC( glXGetProcAddress( (GLubyte*)"glXGetGPUInfoAMD" ) );
#endif

      GLuint *ids = NULL;
      GLuint n = my_glGetGPUIDsAMD( 0, NULL );
      if( n == 0 )  goto failure;

      ids = new GLuint[n];
      n = my_glGetGPUIDsAMD( n, ids );
      if( n == 0 )  goto failure;

      for( unsigned int i=0; i<n; i++ ) {
         size_t totalMem;
         my_glGetGPUInfoAMD( ids[i], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(size_t), &totalMem);
         s += QString( " %1" ).arg( totalMem );
      }
   goto end;
   failure:
      s += "failure";
   end:
      delete ids;

   } else
      s += QString( "not supported" );

   // GL_ATI_meminfo
   s += "\nGL_ATI_meminfo: ";
   if( supportsExtension( "GL_ATI_meminfo" ) ) {

      s += QString( " available: %1MB, targest block: %2MB" )
                   .arg( (getGLInteger( GL_TEXTURE_FREE_MEMORY_ATI, 0 ) + 512) / 1024 )
                   .arg( (getGLInteger( GL_TEXTURE_FREE_MEMORY_ATI, 1 ) + 512) / 1024 );

   } else
      s += QString( "not supported" );

   return s;
}


//
//  ATI/AMD Catalyst version
//
// It is returned as a part of glGetString( GL_RENDERER ).
// For example, "4.0.9901 Compatibility Profile Context".
// 9901 is driver build number. Translation table to Catalyst
// version is available at http://developer.amd.com/drivers/ccc/Pages/default.aspx .
//
// More information can be found in the registry (on Windows platform).
// There are all display adapters listed bellow following key:
// HKLM\SYSTEM\CurrentControlSet\Control\Class\{4D36E968-E325-11CE-BFC1-08002BE10318}\
// First display adapter is bellow 0000 key, second (if present) in 0001, third 0002, etc.
//
// Bellow adapter key (for example, 0000), there are following useful values
// (listed with an example value):
//
// For Catalyst 10.6 on Windows 7:
// Catalyst_Version     10.6
// DriverDate           5-27-2010
// DriverVersion        8.741.0.0
// Device Description   ATI Radeon HD 5700
// DriverDesc           ATI Radeon HD 5700
// ProviderName         ATI Technologies Inc.
// ReleaseVersion       8.741-100527a-100948C-ATI
// Settings\Device Description  ATI Radeon HD 5700 Series
//
// On Catalyst 9.7 (build number: 8787) on Windows 7:
// Catalyst_Version     09.7
// DriverDate           8-17-2009
// DriverVersion        8.632.1.2000
// Device Description   ATI Mobility Radeon HD 3670
// DriverDesc           ATI Mobility Radeon HD 3670
// ProviderName         ATI Technologies Inc.
// ReleaseVersion       8.632.1.2-090817a-086997C-ATI
// Settings\Device Description  ATI Mobility Radeon HD 3670
//
// On Catalyst ?? (build number: 5883) on Windows XP:
// DriverDate           5-23-2006
// DriverVersion        8.261.0.0
// DriverDesc           ATI Mobility Radeon X1300
// ProviderName         ATI Technologies Inc.
// ReleaseVersion       8.36-070314a3-045540C-Dell
// Settings\Device Description  ATI Mobility Radeon X1300
// Settings\ReleaseVersion      8.261-060523a1-033841C-Dell  (this seems to be shown in Catalyst Control Center)
//
//
//  NVIDIA Detonator version
//
// Bellow adapter key (for example, 0000), there are following useful values
// (listed with an example value):
//
// On Detonator 258.96 (build number 5896) on Windows 7:
// Device Description   NVIDIA GeForce GTX 260
// DriverDesc           NVIDIA GeForce GTX 260
// DriverDate           7-9-2010
// DriverVersion        8.17.12.5896
// HardwareInformation.AdapterString  GeForce GTX 260
// HardwareInformation.ChipType       GeForce GTX 260
// HardwareInformation.MemorySize     0x38000000   (896MB)
// HardwareInformation.qwMemorySize   0x38000000   (896MB)
// ProviderName         NVIDIA
// Settings\Device Description  NVIDIA GeForce GTX 260
//
// For Detonator 197.16 on Windows Vista:
// DriverDesc           NVIDIA GeForce 8400M GS
// DriverDate           3-16-2010
// DriverVersion        6.14.11.9716
// ProviderName         NVIDIA
// Settings\Device Description  NVIDIA GeForce 8400M GS
//
//
//  Intel
//
// On GMA driver (build number: 4926) on Windows XP:
// DriverDesc           Mobile Intel(R) 945 Express Chipset Family
// DriverDate           2-15-2008
// DriverVersion        6.14.10.4926
// ProviderName         Intel Corporation
// Settings\Device Description  Mobile Intel(R) 945 Express Chipset Family
//

struct Build2VersionRec {
   int build;
   char version[6];
};

// Conversion table from driver build version to Catalyst version.
// The most recent driver must be on index 0.
// Table taken from http://developer.amd.com/drivers/ccc/Pages/default.aspx .
Build2VersionRec catalystBuild2version[] = {
   { 10317, "10.11" },
   { 10243, "10.10" }, // Win7, Vista
   { 10237, "10.10" }, // WinXP, seems Linux as well (at least on my computer)
   { 10188, "10.9" },
   { 10151, "10.8" },
   { 10061, "10.7" },
   {  9901, "10.6" },
   {  9836, "10.5" },
   {  9756, "10.4" },
   {  9704, "10.3" },
   {  9551, "10.2" },
   {  9252, "10.1" },
   {  9232, "9.12" },
   {  9116, "9.11" },
   {  9026, "9.10" },
   {  8918, "9.9" },
   {  8870, "9.8" },
   {  8787, "9.7" },
   {  8673, "9.6" },
   {  8664, "9.5" },
   {  8577, "9.4" },
   {  8543, "9.3" },
   {  8494, "9.2" },
   {  8395, "9.1" },
   {  8304, "8.12" },
   {  8201, "8.11" },
   {  8086, "8.10" },
   {  7976, "8.9" },
   {  7873, "8.8" },
   {  7767, "8.7" },
   {  7659, "8.6" },
   {  7537, "8.5" },
   {  7412, "8.3-4" },
   {  7277, "8.2" },
   {  7275, "8.1" },
   {  7169, "7.12" },
   {  7058, "7.11" },
   {  6956, "7.10" },
   {  6847, "7.9" },
   {  6645, "7.7-8" },
   {  6479, "7.6" },
};


static QString buildNumber2catalystVersion( int n )
{
   for( int i=0; i<sizeof(catalystBuild2version)/sizeof(Build2VersionRec); i++ )
      if( catalystBuild2version[i].build == n )
         return QString( catalystBuild2version[i].version );

   return "";
}


#if defined(__WIN32__) || defined(_WIN32)

static QString getGraphicsDriverRegistryInfo( const QString& registryPath )
{
   QString r;
   QString path( registryPath );
   if( path.endsWith( '\\' ) )
      path.chop( 1 );

   // print registry path
   r += "Device found at: HKLM\\" + path + '\n';
   path.append( '\\' );

   // device description
   // for example: "NVIDIA GeForce 8400M GS" or "ATI Radeon HD 5700"
   QString deviceDescription = WinRegistry::getString( HKEY_LOCAL_MACHINE, path + "Settings",
         "Device Description" );
   if( deviceDescription.isEmpty() )
      deviceDescription = WinRegistry::getString( HKEY_LOCAL_MACHINE, path,
         "DriverDesc" );
   if( deviceDescription.isEmpty() )
      deviceDescription = WinRegistry::getString( HKEY_LOCAL_MACHINE, path,
         "Device Description" );
   if( deviceDescription.isEmpty() )
      deviceDescription = "< unknown >";
   r += "   Device description: " + deviceDescription + '\n';

   // provider name
   // for example: "NVIDIA" or "ATI Technologies Inc."
   QString providerName = WinRegistry::getString( HKEY_LOCAL_MACHINE, path, "ProviderName" );
   if( providerName.isEmpty() )
      providerName = "< unknown >";
   r += "   Provider name: " + providerName + '\n';

   // driver date
   // for example: 5-27-2010
   QString driverDate = WinRegistry::getString( HKEY_LOCAL_MACHINE, path, "DriverDate" );
   if( driverDate.isEmpty() )
      driverDate = "< unknown >";
   r += "   Driver date (MM-DD-YYYY): " + driverDate + '\n';

   // driver version
   // example of driverVersion string: "6.14.11.9716"
   QString driverVersion = WinRegistry::getString( HKEY_LOCAL_MACHINE, path, "DriverVersion" );
   if( driverVersion.isEmpty() )
      driverVersion = "< unknown >";
   r += "   Driver version string: " + driverVersion + '\n';

   // driver release version
   // for example: "8.632.1.2-090817a-086997C-ATI" or "8.36-070314a3-045540C-Dell"
   QString releaseVersion = WinRegistry::getString( HKEY_LOCAL_MACHINE, path, "ReleaseVersion" );
   if( releaseVersion.isEmpty() )
      releaseVersion = "< none >";
   r += "   Driver release version: " + releaseVersion + '\n';

   // get Catalyst version
   // example string: 9.7
   if( providerName.contains( "ATI" ) )
   {
      QString catalystVersion = WinRegistry::getString( HKEY_LOCAL_MACHINE, path, "Catalyst_Version" );
      if( catalystVersion.isEmpty() )
         catalystVersion = "< unknown >";
      r += "   Catalyst version: " + catalystVersion + '\n';
   }

   // get Detonator version
   if( providerName.contains( "NVIDIA" ) )
   {
      // example of driverVersion string: "6.14.11.9716"
      // example of detonatorVersion: 197.16
      QString detonatorVersion( driverVersion );
      int i = detonatorVersion.lastIndexOf( '.' );
      if( i >= 0 )
         detonatorVersion.remove( i, 1 );
      if( i >= 2 ) {
         detonatorVersion.remove( 0, i-1 );
         if( detonatorVersion.size()-2 > 0 )
            detonatorVersion.insert( detonatorVersion.size()-2, '.' );
      }
      r += "   NVIDIA driver version: " + detonatorVersion + '\n';
   }

   return r;
}


static void recursivelyFindGraphicsDriverInfo( const QString& path, QString& info )
{
   // examine current path whether it contains a graphics driver,
   // detection starts with ClassGUID value
   if( WinRegistry::exists( HKEY_LOCAL_MACHINE, path, "ClassGUID" ) ) {

      // ClassGUID set to {4D36E968-E325-11CE-BFC1-08002BE10318} indicates "Display adapter"
      QString classGUID = WinRegistry::getString( HKEY_LOCAL_MACHINE, path, "ClassGUID" );
      if( classGUID.compare( "{4D36E968-E325-11CE-BFC1-08002BE10318}", Qt::CaseInsensitive ) == 0 ) {

         // find out "driver path" as driver path contains much more info about the graphics card and its driver
         if( WinRegistry::exists( HKEY_LOCAL_MACHINE, path, "Driver" ) ) {

            // get graphics driver info
            QString driverPath = WinRegistry::getString( HKEY_LOCAL_MACHINE, path, "Driver" );
            info += getGraphicsDriverRegistryInfo( "SYSTEM\\CurrentControlSet\\Control\\Class\\" + driverPath );
         }
      }
   }

   // check for registry errors while ignoring ACCESS_DENIED
   int e = WinRegistry::getError();
   if( e != 0 && e != ERROR_ACCESS_DENIED )
      OSG_WARN << "Error in the registry: " << e << std::endl;


   // get subkeys
   std::list< std::wstring > keyList = WinRegistry::getSubElements( HKEY_LOCAL_MACHINE,
      std::wstring( (const wchar_t*)path.constData(), path.length() ), WinRegistry::KEYS );

   // check for registry errors while ignoring ACCESS_DENIED
   e = WinRegistry::getError();
   if( e != 0 && e != ERROR_ACCESS_DENIED )
      OSG_WARN << "Error in the registry enumeration: " << e << std::endl;

   // iterate through subkeys
   for( std::list< std::wstring >::iterator it = keyList.begin();
        it != keyList.end(); it++ )
   {
      // recursively call this function on all subkeys
      recursivelyFindGraphicsDriverInfo( QString( path ) + '\\' +
         QString( (const QChar*)it->c_str(), it->length() ), info );
   }

}


static QString getAllGraphicsDriversInfoFromRegistry()
{
   int e = WinRegistry::getError();
   if( e != 0 )
      OSG_WARN << "Uncatched error in the registry: " << e << std::endl;

   QString info;
   recursivelyFindGraphicsDriverInfo( "SYSTEM\\CurrentControlSet\\Enum", info );

   return info;
}

#endif


QString SysInfo::getGraphicsDriverInfo()
{
   QString r( "Graphics driver info from OpenGL context:\n" );
   QString glVersionString( (const char*)glGetString( GL_VERSION ) );
   QString glVendorString( (const char*)glGetString( GL_VENDOR ) );

   // Attempt to parse NVIDIA string.
   // For example: "2.1.2 NVIDIA 169.12"
   int i = glVersionString.indexOf( "NVIDIA" );
   if( i != -1 ) {
      QString buildNumber( glVersionString );
      buildNumber.remove( 0, i );
      buildNumber.remove( 0, 6 ); // remove "NVIDIA"
      r += "   NVIDIA driver version: " + buildNumber.trimmed();
   }
   else
   // Attempt to parse Mesa string.
   // For example: "2.1 Mesa 7.5.1"
   if( (i = glVersionString.indexOf( "Mesa" ) ) != -1 ) {
      QString buildNumber( glVersionString );
      buildNumber.remove( 0, i );
      buildNumber.remove( 0, 4 ); // remove "Mesa"
      r += "   Mesa driver version: " + buildNumber.trimmed();
   }
   else
   {
      // Parse it as ATI style version string.
      // For example: "2.1.7412 Release" (ATI Mobility Radeon X1300)
      //              "1.4.0 - Build 7.14.10.4926" (Intel 945GM)
      QString buildNumber( glVersionString );
      i = buildNumber.lastIndexOf( '.' );
      buildNumber.remove( 0 , i+1 );
      for( i=0; i<buildNumber.size(); i++ )
         if( buildNumber[i].isSpace() )
            break;
      buildNumber.truncate( i );
      bool ok;
      int buildNumberInt = buildNumber.toInt( &ok );

      if( buildNumberInt == 0 )
         r += "   No version strig in graphics context.\n"
              "   (OpenGL vendor: (" + glVendorString + "), glGetString(GL_VERSION): \"" + glVersionString + "\")\n";
      else
      {
         // decode
         if( glVendorString.contains( "ATI" ) || glVendorString.contains( "AMD" ) )
         {
            r += "   Catalyst driver version: ";
            QString catalystVersion = buildNumber2catalystVersion( buildNumberInt );
            if( !catalystVersion.isEmpty() )
               r += catalystVersion + " (build number: " + buildNumber + ")";
            else
               if( buildNumberInt > catalystBuild2version[0].build )
                  r += ">" + QString( catalystBuild2version[0].version ) +
                     "(build number: " + buildNumber + ")\n"
                     "   Look at AMD website for your particular driver version.\n"
                     "   (You may try link http://developer.amd.com/drivers/ccc/Pages/default.aspx)";
               else
                  r += "unknown (build number: " + buildNumber + ")\n"
                     "   Look at AMD website for list of driver versions.\n"
                     "   (You may try link http://developer.amd.com/drivers/ccc/Pages/default.aspx)";
         }
         else
         {
            r += "   Driver build number: " + buildNumber;
         }
      }
   }

#if defined(__WIN32__) || defined(_WIN32)

   // get drivers info from Windows registry
   r += "Installed graphics drivers in the system:\n";
   r += getAllGraphicsDriversInfoFromRegistry();

#endif

   return r;

}
