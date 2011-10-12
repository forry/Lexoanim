
/**
 * @file
 * TextureUnitMoverVisitor class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef TEXTURE_UNIT_MOVER_VISITOR_H
#define TEXTURE_UNIT_MOVER_VISITOR_H

#include "utils/StateSetVisitor.h"


class TextureUnitMoverVisitor : public StateSetVisitor {
public:

   TextureUnitMoverVisitor( int fromUnit, int toUnit )
      : StateSetVisitor(),
        _fromUnit( fromUnit ),
        _toUnit( toUnit ) {}

   virtual void apply( osg::StateSet& stateSet );
   virtual void apply( osg::Drawable& drawable );

protected:
   int _fromUnit;
   int _toUnit;
};


#endif /* TEXTURE_UNIT_MOVER_VISITOR_H */
