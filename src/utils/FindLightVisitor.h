#include <osg/NodeVisitor>
#include <osg/LightSource>

using namespace osg;
//using namespace osgViewer;


class FindLightVisitor : public NodeVisitor
{
    typedef NodeVisitor inherited;
public:

    FindLightVisitor() :
        NodeVisitor( NodeVisitor::TRAVERSE_ALL_CHILDREN ),
        _light( NULL )
    {
    }

    META_NodeVisitor( "", "FindLightVisitor" )

    Light* getLight() const { return _light; }

    virtual void reset()
    {
        inherited::reset();
        _light = NULL;
    }

    virtual void apply( LightSource& ls )
    {
        Light *l = ls.getLight();
        if( l && !_light ) {
            _light = l;
        }
    }

protected:
    ref_ptr< Light > _light;
};
