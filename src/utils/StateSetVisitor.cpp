
/**
 * @file
 * StateSetVisitor class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/Drawable>
#include <osg/Geode>
#include "utils/StateSetVisitor.h"

using namespace osg;



void StateSetVisitor::apply( Drawable& drawable )
{
   StateSet *ss = drawable.getStateSet();
   if( ss )
      apply( *ss );
}


void StateSetVisitor::apply( Node& node )
{
   StateSet *ss = node.getStateSet();
   if( ss )
      apply( *ss );

   traverse( node );
}


void StateSetVisitor::apply( Geode& geode )
{
   StateSet *ss = geode.getStateSet();
   if( ss )
      apply( *ss );

   for( unsigned int i=0; i<geode.getNumDrawables(); ++i )
      apply( *geode.getDrawable( i ) );

   traverse( geode );
}
