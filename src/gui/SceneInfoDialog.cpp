
/**
 * @file
 * SceneInfoDialog class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/LightSource>
#include <osgUtil/Statistics>
//#include "Lexolights.h"
#include "LexoanimQtApp.h"
#include "LexolightsDocument.h"
#include "gui/SceneInfoDialog.h"
#include "ui_SystemInfoDialog.h"
#include "utils/Log.h"

using namespace osg;
using namespace osgUtil;



SceneInfoDialog::SceneInfoDialog( QWidget *parent )
   : inherited( parent )
{
   ui = new Ui::SystemInfoDialog;
   ui->setupUi( this );
   setWindowTitle( "Scene Information" );
   resize( width() + 100, height() + 100 );
   refreshInfo();
}


SceneInfoDialog::~SceneInfoDialog()
{
   delete ui;
}


static void putCaption( QString &info, const QString &caption )
{
   info += "<tr><td colspan=7><b>" + caption + "</b></td></tr>\n";
}


static void putRow( QString &info, const QString &descr, const QString &value1,
                    int indent, const QString &valueAlignment1, int valueSpan,
                    const QString &value2 = "", const QString &valueAlignment2 = "" )
{
   QString nbrDescr( descr );
   nbrDescr.replace( " ", "&nbsp;" );
   QString nbrValue1( value1 );
   nbrValue1.replace( " ", "&nbsp;" );
   QString nbrValue2( value2 );
   nbrValue2.replace( " ", "&nbsp;" );

   info += "<tr><td>";
   for( int i=0; i<indent; i++ )
      info += "&nbsp;&nbsp;&nbsp;&nbsp;";
   info += nbrDescr + "</td>";
   info += "<td width=15></td>";
   info += "<td " + ( valueSpan==1 ? "width=15" : "colspan="+QString::number( valueSpan ) ) +
           " align=" + valueAlignment1 + ">" + nbrValue1 + "</td>";
   if( !value2.isEmpty() )
      info += "<td width=15></td><td align=" +
              (valueAlignment2.isEmpty() ? "center" : valueAlignment2 ) + ">" + nbrValue2 + "</td>";
   for( int i=( value2.isEmpty() ? valueSpan : valueSpan+2 ); i<4; i++ )
      info += "<td></td>";
   info += "</tr>\n";
}


static void putRow( QString &info, const QString &descr, const QString &value )
{
   putRow( info, descr, value, 1, "center", 1 );
}


static void putLongRow( QString &info, const QString &descr, const QString &value )
{
   putRow( info, descr, value, 1, "left", 5 );
}


static void putLongSubRow( QString &info, const QString &descr, const QString &value )
{
   putRow( info, descr, value, 2, "left", 5 );
}


static void putRow2( QString &info, const QString &descr, const QString &value1, const QString &value2 )
{
   putRow( info, descr, value1, 1, "center", 1, value2, "center" );
}


static void putMergedRow2( QString &info, const QString &descr, const QString &value )
{
   putRow( info, descr, value, 1, "center", 3 );
}


static inline void putRow( QString &info, const QString &descr, int value )
{
   putRow( info, descr, QString::number( value ) );
}


static inline void putRow2( QString &info, const QString &descr, int value1, int value2 )
{
   putRow2( info, descr, QString::number( value1 ), QString::number( value2 ) );
}


static inline void putMergedRow2( QString &info, const QString &descr, int value )
{
   putMergedRow2( info, descr, QString::number( value ) );
}


static inline void putRow2_tryMerge( QString &info, const QString &descr, int value1, int value2 )
{
   if( value1 == value2 )
      putMergedRow2( info, descr, value1 );
   else
      putRow2( info, descr, value1, value2 );
}


class MyStatsVisitor : public StatsVisitor
{
   typedef StatsVisitor inherited;

public:

   MyStatsVisitor() : _numInstancedLightSources( 0 ), _numInstancedTextures( 0 ),
                      _numInstancedShaderPrograms( 0 )  {}

   virtual void reset()
   {
      inherited::reset();

      _numInstancedLightSources = 0;
      _numInstancedTextures = 0;
      _numInstancedShaderPrograms = 0;
      _lightSourceSet.clear();
      _textureSet.clear();
      _shaderProgramSet.clear();
   }

   virtual void apply( StateSet &ss )
   {
      inherited::apply( ss );

      // Textures
      int c = ss.getNumTextureAttributeLists();
      for( int i=0; i<c; i++ )
      {
         Texture *t = dynamic_cast< Texture* >( ss.getTextureAttribute( i, StateAttribute::TEXTURE ) );
         if( t )
         {
            ++_numInstancedTextures;
            _textureSet.insert( t );
         }
      }

      // shader programs
      Program *p = dynamic_cast< Program* >( ss.getAttribute( StateAttribute::PROGRAM ) );
      if( p )
      {
         ++_numInstancedShaderPrograms;
         _shaderProgramSet.insert( p );
      }
   }

   virtual void apply( LightSource &node )
   {
      if( node.getStateSet() )
      {
         apply( *node.getStateSet() );
      }

      ++_numInstancedLightSources;
      _lightSourceSet.insert( &node );
      traverse( node );
   }

   unsigned int _numInstancedLightSources;
   unsigned int _numInstancedTextures;
   unsigned int _numInstancedShaderPrograms;
   NodeSet _lightSourceSet;
   std::set< Texture* > _textureSet;
   std::set< Program* > _shaderProgramSet;
};


static void putSceneGraphInfo( QString &info, MyStatsVisitor &visitor )
{
   putRow2_tryMerge( info, "Triangles (separated)", visitor._instancedStats.getPrimitiveCountMap()[GL_TRIANGLES],      visitor._uniqueStats.getPrimitiveCountMap()[GL_TRIANGLES] );
   putRow2_tryMerge( info, "Triangles in strips",   visitor._instancedStats.getPrimitiveCountMap()[GL_TRIANGLE_STRIP], visitor._uniqueStats.getPrimitiveCountMap()[GL_TRIANGLE_STRIP] );
   putRow2_tryMerge( info, "Triangles in fans",     visitor._instancedStats.getPrimitiveCountMap()[GL_TRIANGLE_FAN],   visitor._uniqueStats.getPrimitiveCountMap()[GL_TRIANGLE_FAN] );
   putRow2_tryMerge( info, "Lines (separated)", visitor._instancedStats.getPrimitiveCountMap()[GL_LINES],      visitor._uniqueStats.getPrimitiveCountMap()[GL_LINES] );
   putRow2_tryMerge( info, "Lines in strips",   visitor._instancedStats.getPrimitiveCountMap()[GL_LINE_STRIP], visitor._uniqueStats.getPrimitiveCountMap()[GL_LINE_STRIP] );
   putRow2_tryMerge( info, "Points",            visitor._instancedStats.getPrimitiveCountMap()[GL_POINTS], visitor._uniqueStats.getPrimitiveCountMap()[GL_POINTS] );
   putRow2_tryMerge( info, "Quads (separated)", visitor._instancedStats.getPrimitiveCountMap()[GL_QUADS], visitor._uniqueStats.getPrimitiveCountMap()[GL_QUADS] );
   putRow2_tryMerge( info, "Quads in strips",   visitor._instancedStats.getPrimitiveCountMap()[GL_QUAD_STRIP], visitor._uniqueStats.getPrimitiveCountMap()[GL_QUAD_STRIP] );
   putRow2_tryMerge( info, "Polygons",          visitor._instancedStats.getPrimitiveCountMap()[GL_POLYGON], visitor._uniqueStats.getPrimitiveCountMap()[GL_POLYGON] );
   putRow2_tryMerge( info, "Textures",  visitor._numInstancedTextures, visitor._textureSet.size() );
   putRow2_tryMerge( info, "Shader Programs",  visitor._numInstancedShaderPrograms, visitor._shaderProgramSet.size() );
   putRow2_tryMerge( info, "StateSets", visitor._numInstancedStateSet, visitor._statesetSet.size() );
   putRow2_tryMerge( info, "Drawables", visitor._numInstancedDrawable, visitor._drawableSet.size() );
   putRow2_tryMerge( info, "Slow Geometries", visitor._numInstancedGeometry - visitor._numInstancedFastGeometry,
                                               visitor._geometrySet.size() - visitor._fastGeometrySet.size() );
   putRow2_tryMerge( info, "Geodes",    visitor._numInstancedGeode, visitor._geodeSet.size() );
   putRow2_tryMerge( info, "Groups",     visitor._numInstancedGroup, visitor._groupSet.size() );
   putRow2_tryMerge( info, "Transforms", visitor._numInstancedTransform, visitor._transformSet.size() );
   putRow2_tryMerge( info, "Lights",     visitor._numInstancedLightSources, visitor._lightSourceSet.size() );
   putRow2_tryMerge( info, "LODs",       visitor._numInstancedLOD, visitor._lodSet.size() );
   putRow2_tryMerge( info, "Switches",   visitor._numInstancedSwitch, visitor._switchSet.size() );
}


void SceneInfoDialog::refreshInfo()
{
   // collect stats
   MyStatsVisitor visitor;
   if( LexoanimQtApp::activeDocument() )
      LexoanimQtApp::activeDocument()->getOriginalScene()->accept( visitor );
   visitor.totalUpStats();

   // start table
   QString info;
   info += "<table>";
   putCaption( info, "Model Summary" );

   // summary
   putRow( info, "Vertices", visitor._instancedStats._vertexCount );
   putRow( info, "Triangles", visitor._instancedStats.getPrimitiveCountMap()[GL_TRIANGLES] +
                              visitor._instancedStats.getPrimitiveCountMap()[GL_TRIANGLE_STRIP] +
                              visitor._instancedStats.getPrimitiveCountMap()[GL_TRIANGLE_FAN] );
   putRow( info, "Lines", visitor._instancedStats.getPrimitiveCountMap()[GL_LINES] +
                          visitor._instancedStats.getPrimitiveCountMap()[GL_LINE_STRIP] );
   putRow( info, "Others", visitor._instancedStats.getPrimitiveCountMap()[GL_POINTS] +
                           visitor._instancedStats.getPrimitiveCountMap()[GL_QUADS] +
                           visitor._instancedStats.getPrimitiveCountMap()[GL_QUAD_STRIP] +
                           visitor._instancedStats.getPrimitiveCountMap()[GL_POLYGON] );
   putRow( info, "Drawables", visitor._numInstancedDrawable );
   putRow( info, "Textures", visitor._textureSet.size() );
   putRow( info, "Lights", visitor._numInstancedLightSources );

   // detailed model info
   putRow( info, "", "" );
   putCaption( info, "Model Details" );
   putRow2( info, "", "Instanced", "   Unique   " );
   putSceneGraphInfo( info, visitor );

   // PPL scene details
   putRow( info, "", "" );
   putCaption( info, "Rendering Data Details" );
   putRow2( info, "", "Instanced", "   Unique   " );
   visitor.reset();
   if( LexoanimQtApp::activeDocument() )
      LexoanimQtApp::activeDocument()->getPPLScene()->accept( visitor );
   putSceneGraphInfo( info, visitor );

   // finish table
   info += "</table>";

   ui->infoText->setText( info );
}
