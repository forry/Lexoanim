
/**
 * @file
 * StateSetVisitor class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef STATE_SET_VISITOR_H
#define STATE_SET_VISITOR_H

#include <osg/NodeVisitor>


class StateSetVisitor : public osg::NodeVisitor
{
public:

    StateSetVisitor() : NodeVisitor( NODE_VISITOR, TRAVERSE_ALL_CHILDREN ) {}
    StateSetVisitor( TraversalMode tm ) : NodeVisitor( tm ) {}
    StateSetVisitor( VisitorType type, TraversalMode tm = TRAVERSE_ALL_CHILDREN ) : NodeVisitor( type, tm ) {}

    META_NodeVisitor( "Lexolights", "StateSetVisitor" )

    virtual void apply( osg::StateSet& stateSet ) = 0;

    virtual void apply( osg::Drawable& drawable );
    virtual void apply( osg::Node& node );
    virtual void apply( osg::Geode& geode );

};


#endif /* STATE_SET_VISITOR_H */
