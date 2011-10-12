
/**
 * @file
 * TextureUnitsUsageVisitor class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include "utils/TextureUnitsUsageVisitor.h"

using namespace osg;
 


void TextureUnitsUsageVisitor::apply( StateSet& stateSet )
{
   // attribute
   const StateSet::TextureAttributeList& taList = stateSet.getTextureAttributeList();
   unsigned int c = taList.size();
   if( _attributesFound.size() < c )
      _attributesFound.resize( c, false );
   for( unsigned int i=0; i<c; i++ )
   {
      // if attribute presence is already set, can skip all the tests for this texture unit
      if( _attributesFound[i] )
         continue;

      // find any texture attribute for the particular texture unit
      const StateSet::AttributeList& aList = taList[i];
      for( StateSet::AttributeList::const_iterator it = aList.begin(); it != aList.end(); it++ )
      {
         if( it->second.first.get() != NULL )
         {
            _attributesFound[i] = true;
            break;
         }
      }
   }

   // mode
   const StateSet::TextureModeList& tmList = stateSet.getTextureModeList();
   c = tmList.size();
   if( _modeOn.size() < c )
      _modeOn.resize( c, false );
   for( unsigned int i=0; i<c; i++ )
   {
      // if mode is already set, can skip all the tests for this texture unit
      if( _modeOn[i] )
         continue;

      const StateSet::ModeList& mList = tmList[i];
      StateSet::ModeList::const_iterator it = mList.find( GL_TEXTURE_2D );
      if( it != mList.end() )
      {
         const StateAttribute::GLModeValue value = it->second;
         if( (value & StateAttribute::ON) != 0 )
            _modeOn[i] = true;
      }
   }
}
