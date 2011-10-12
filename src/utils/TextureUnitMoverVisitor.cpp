
/**
 * @file
 * TextureUnitMoverVisitor class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/Geometry>
#include "utils/TextureUnitMoverVisitor.h"

using namespace osg;
 


void TextureUnitMoverVisitor::apply( StateSet& stateSet )
{
   // attributes
   StateSet::TextureAttributeList& taList = stateSet.getTextureAttributeList();
   int c = taList.size();
   if( _fromUnit < c )
   {
      // make sure there is enough space in taList
      if( _toUnit >= c )
         taList.resize( _toUnit+1 );

      // copy attribute list
      taList[ _toUnit ] = taList[ _fromUnit ];

      // clear source texturing unit
      taList[ _fromUnit ].clear();
   }

   // mode
   StateSet::TextureModeList& tmList = stateSet.getTextureModeList();
   c = tmList.size();
   if( _fromUnit < c )
   {
      // make sure there is enough space in taList
      if( _toUnit >= c )
         tmList.resize( _toUnit+1 );

      // copy attribute list
      tmList[ _toUnit ] = tmList[ _fromUnit ];

      // clear source texturing unit
      tmList[ _fromUnit ].clear();
   }
}

void TextureUnitMoverVisitor::apply( Drawable& drawable )
{
   Geometry *g = drawable.asGeometry();
   if( !g )
      return;

   Array *a = g->getTexCoordArray( _fromUnit );
   if( a )
      g->setTexCoordArray( _toUnit, a );
   else
   {
      if( g->getTexCoordArray( _toUnit ) )
         g->setTexCoordArray( _toUnit, NULL );
   }
}
