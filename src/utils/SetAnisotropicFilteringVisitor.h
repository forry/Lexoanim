
/**
 * @file
 * SetAnisotropicFilteringVisitor class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef SET_ANISOTROPIC_FILTERING_VISITOR_H
#define SET_ANISOTROPIC_FILTERING_VISITOR_H

#include "utils/StateSetVisitor.h"


class SetAnisotropicFilteringVisitor : public StateSetVisitor
{
public:

    SetAnisotropicFilteringVisitor( TraversalMode tm = TRAVERSE_ALL_CHILDREN ) : StateSetVisitor( tm ), _value( 1.f ) {}
    SetAnisotropicFilteringVisitor( VisitorType type, TraversalMode tm = TRAVERSE_ALL_CHILDREN ) : StateSetVisitor( type, tm ), _value( 1.f ) {}
    SetAnisotropicFilteringVisitor( float value, VisitorType type = NODE_VISITOR, TraversalMode tm = TRAVERSE_ALL_CHILDREN ) : StateSetVisitor( type, tm ), _value( value ) {}

    inline float getValue() const  { return _value; }
    virtual void setValue( float value )  { _value = value; }


    META_NodeVisitor( "Lexolights", "SetAnisotropicFilteringVisitor" )


    virtual void apply( osg::StateSet& stateSet );

protected:

    float _value;

};


#endif /* SET_ANISOTROPIC_FILTERING_VISITOR_H */
