
/**
 * @file
 * SetAnisotropicFilteringVisitor class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/Texture>
#include "utils/SetAnisotropicFilteringVisitor.h"

using namespace osg;
 


void SetAnisotropicFilteringVisitor::apply( StateSet& stateSet )
{
   for( int i=0, c=stateSet.getNumTextureAttributeLists(); i<c; i++ )
   {
      StateAttribute *a = stateSet.getTextureAttribute( i, StateAttribute::TEXTURE );
      if( !a )
         continue;
      Texture *t = dynamic_cast< Texture* >( a );
      if( !t )
         continue;

      t->setMaxAnisotropy( _value );
   }
}
