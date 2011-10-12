
/**
 * @file
 * TextureUnitsUsageVisitor class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef TEXTURE_UNITS_USAGE_VISITOR_H
#define TEXTURE_UNITS_USAGE_VISITOR_H

#include "utils/StateSetVisitor.h"


class TextureUnitsUsageVisitor : public StateSetVisitor {
public:

   virtual void apply( osg::StateSet& stateSet );

   std::vector< bool > _attributesFound;
   std::vector< bool > _modeOn;

};


#endif /* TEXTURE_UNITS_USAGE_VISITOR_H */
