
/**
 * @file
 * SystemInfoDialog class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/Version>
#include "gui/SystemInfoDialog.h"
#include "ui_SystemInfoDialog.h"
#include "utils/BuildTime.h"
#include "utils/SysInfo.h"



SystemInfoDialog::SystemInfoDialog( QWidget *parent )
   : inherited( parent )
{
   ui = new Ui::SystemInfoDialog;
   ui->setupUi( this );
   refreshInfo();
}


SystemInfoDialog::~SystemInfoDialog()
{
   delete ui;
}


static void putCaption( QString &info, const QString &caption )
{
   info += "<tr><td colspan=7><b>" + caption + "</b></td></tr>\n";
}


static void putRow( QString &info, const QString &descr, const QString &value,
                    int indent, const QString &valueAlignment, int valueSpan )
{
   QString nbrDescr( descr );
   nbrDescr.replace( " ", "&nbsp;" );
   QString nbrValue( value );
   nbrValue.replace( " ", "&nbsp;" );

   info += "<tr><td>";
   for( int i=0; i<indent; i++ )
      info += "&nbsp;&nbsp;&nbsp;&nbsp;";
   info += nbrDescr + "</td>";
   info += "<td width=15></td>";
   info += "<td " + ( valueSpan==1 ? "width=15" : "colspan="+QString::number( valueSpan ) ) +
           " align=" + valueAlignment + ">" + nbrValue + "</td>";
   for( int i=valueSpan; i<5; i++ )
      info += "<td></td>";
   info += "</tr>\n";
}


static void putRow( QString &info, const QString &descr, const QString &value )
{
   putRow( info, descr, value, 1, "right", 1 );
}


static void putLongRow( QString &info, const QString &descr, const QString &value )
{
   putRow( info, descr, value, 1, "left", 5 );
}


static void putLongSubRow( QString &info, const QString &descr, const QString &value )
{
   putRow( info, descr, value, 2, "left", 5 );
}


void SystemInfoDialog::refreshInfo()
{
   QString info;

   //
   // application info
   //
   info += "<table>";
   putCaption( info, "Application" );

   // Lexolights versions and buid date and time
#if 0
   putLongRow( info, "Lexolights version", "???" );
   putLongSubRow( info, "Internal version", QString() + LEXOLIGHTS_VERSION_MAJOR + "." LEXOLIGHTS_VERSION_MINOR );
#else
   putLongRow( info, "Lexolights version", QString() + LEXOLIGHTS_VERSION_MAJOR + "." LEXOLIGHTS_VERSION_MINOR );
#endif
   putLongRow( info, "Build date", buildDate );
   putLongRow( info, "Build time", buildTime );

   // architecture and compiler
#if defined(__WIN32__) || defined(_WIN32)
# if defined(_M_AMD64) || defined(_M_X64) || defined(_M_IX86)
#  ifdef _WIN64
   putLongRow( info, "Architecture", "x64 (64-bit)" );
#  else
   putLongRow( info, "Architecture", "x86 (32-bit)" );
#  endif
# else
#  ifdef _M_IA64
   putLongRow( info, "Architecture", "Itanium" );
#  else
   putLongRow( info, "Architecture", "unknown" );
#  endif
# endif
# if(_MSC_VER >= 1600)
   putLongRow( info, "Compiler", QString( "MSVC 2010" ) );
# elif(_MSC_VER >= 1500)
   putLongRow( info, "Compiler", QString( "MSVC 2008" ) );
# elif(_MSC_VER >= 1400)
   putLongRow( info, "Compiler", QString( "MSVC 2005" ) );
# elif(_MSC_VER >= 1310)
   putLongRow( info, "Compiler", QString( "MSVC 2003" ) );
# elif(_MSC_VER >= 1300)
   putLongRow( info, "Compiler", QString( "MSVC 2002" ) );
# else
   putLongRow( info, "Compiler", QString( "unknown" ) );
# endif
# ifdef _MSC_VER
   putLongSubRow( info, "MSVC internal version", QString::number( _MSC_VER ) );
# endif
#else
   putLongRow( info, "Architecture", sizeof( void* ) == 4 ? "32-bit" :
                                     sizeof( void* ) == 8 ? "64-bit" : "unknown" );
# ifdef __GNUC__
   putLongRow( info, "Compiler", QString( "GNU C/C++ " ) + __VERSION__ );
# else
   putLongRow( info, "Compiler", __VERSION__ );
# endif
#endif

   // OSG and Qt versions
   putLongRow( info, "OSG runtime version", QString( osgGetVersion() ) + "  (SOVERSION: " + osgGetSOVersion() + ")" );
   putLongRow( info, "OSG compile version", QString::number( OPENSCENEGRAPH_MAJOR_VERSION ) + "." +
               QString::number( OPENSCENEGRAPH_MINOR_VERSION ) + "." +
               QString::number( OPENSCENEGRAPH_PATCH_VERSION ) + "  (SOVERSION: " +
               QString::number( OPENSCENEGRAPH_SOVERSION ) + ")" );
   putLongRow( info, "Qt runtime version", QString( qVersion() ) );
   putLongRow( info, "Qt compile version", QString( QT_VERSION_STR ) );

   putCaption( info, "System" );

   int width, height, bpp, freq;
   SysInfo::getDisplayAttributes( &width, &height, &bpp, &freq );
   putLongRow( info, "Display", QString::number( width ) + "x" + QString::number( height ) +
                                ":" + QString::number( bpp ) + "@" + QString::number( freq ) );

   info += "</table>";

   ui->infoText->setText( info );
}
