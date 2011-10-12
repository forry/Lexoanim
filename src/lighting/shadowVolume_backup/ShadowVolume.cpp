/**
 * @file
 * ShadowVolume class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/FrontFace>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Stencil>
#include <osg/StencilTwoSided>
#include <osg/TriangleFunctor>
#include <osg/ValueObject>
#include <osgUtil/Optimizer>
#include <osgShadow/ShadowedScene>
#include <osgViewer/ViewerBase>
#include <osgViewer/View>
#include <stack>
#include <map>
#include <sstream>
#include <cassert>
#include "ShadowVolume.h"

using namespace osg;
using namespace osgShadow;

#define DEBUG_SHOW_WIREFRAME_VOLUMES 0


static const std::string volumeVertexShader(
"#version 120\n"
"\n"
"void main()\n"
"{\n"
"  gl_Position = gl_ModelViewMatrix * gl_Vertex;\n"
"}" );

static const std::string volumeFragmentShader(
"#version 120\n"
"\n"
"void main()\n"
"{\n"
"  gl_FragColor = gl_Color;\n"
"}" );

static const std::string volumeGeometryShader(
"#version 120\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
"\n"
"uniform vec4 lightpos;\n"
"\n"
"void main()\n"
"{\n"
"  // color and vertices\n"
"  vec4 color;\n"
"  vec4 v0 = gl_PositionIn[0];\n"
"  vec4 v1;\n"
"  vec4 v2;\n"
"\n"
"  // detect facing\n"
"  vec4 edg1 = gl_PositionIn[1] - gl_PositionIn[0];\n"
"  vec4 edg2 = gl_PositionIn[2] - gl_PositionIn[0];\n"
"  vec3 norm = cross( edg1.xyz, edg2.xyz );\n"
"\n"
"  // depending on light-facing of the triangle we must change winding\n"
"  vec4 ref = lightpos - gl_PositionIn[0];\n"
"  if( dot( norm, ref.xyz ) >= 0 ) {\n"
"    color = vec4(0.0,0.5,1.0,1.0); // color for debuging purposes\n"
"    v1 = gl_PositionIn[1];\n"
"    v2 = gl_PositionIn[2];\n"
"  } else {\n"
"    color = vec4(0.5,0.0,1.0,1.0);\n"
"    v1 = gl_PositionIn[2];\n"
"    v2 = gl_PositionIn[1];\n"
"  }\n"
"\n"
"  // vertices in infinity\n"
"  vec4 v0inf = gl_ProjectionMatrix * vec4( v0.xyz - lightpos.xyz, 0.0 );\n"
"  vec4 v1inf = gl_ProjectionMatrix * vec4( v1.xyz - lightpos.xyz, 0.0 );\n"
"  vec4 v2inf = gl_ProjectionMatrix * vec4( v2.xyz - lightpos.xyz, 0.0 );\n"
"\n"
"  v0 = gl_ProjectionMatrix * v0;\n"
"  v1 = gl_ProjectionMatrix * v1;\n"
"  v2 = gl_ProjectionMatrix * v2;\n"
"\n"
"  // 1st side\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v0;\n"
"  EmitVertex();\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v0inf;\n"
"  EmitVertex();\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v1;\n"
"  EmitVertex();\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v1inf;\n"
"  EmitVertex();\n"
"\n"
"  //2nd side\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v2;\n"
"  EmitVertex();\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v2inf;\n"
"  EmitVertex();\n"
"\n"
"  //3rd side\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v0;\n"
"  EmitVertex();\n"
"  gl_FrontColor = color;\n"
"  gl_Position = v0inf;\n"
"  EmitVertex();\n"
"}" );


class ShadowVolumeGeometryGenerator : public NodeVisitor
{
public:

    enum GenerateOptions {
        NO_CAPS = 0,
        GENERATE_CAPS = 1 << 0,
        CAPS_SEPARATE_ARRAY = 2 << 1,
    };

    ShadowVolumeGeometryGenerator( const Vec4& lightPos,
                                   ShadowVolume::ShadowCastingFace shadowCastingFace,
                                   int options,
                                   Matrix* matrix = NULL ) :
        NodeVisitor( NodeVisitor::TRAVERSE_ACTIVE_CHILDREN ),
        _coords( new Vec4Array ),
        _capsCoords( new Vec4Array ),
        _lightPos( lightPos ),
        _options( options ),
        _shadowCastingFace( shadowCastingFace )
    {
        if( matrix )
            pushMatrix( *matrix );

        _photorealismData.push( std::map< std::string, std::string >() );
    }

    virtual ~ShadowVolumeGeometryGenerator()
    {
        assert( _photorealismData.size() >= 1 && "_photorealismData underflow." );
        assert( _photorealismData.size() <= 1 && "_photorealismData overflow." );
        _photorealismData.pop();
    }

    META_NodeVisitor( "osgShadow", "ShadowVolumeGeometryGenerator" )

    void reset()
    {
        _coords = new Vec4Array;
        _capsCoords = new Vec4Array;
    }

    int getNumTriangles() const
    {
        return _coords->size() / 3 + _capsCoords->size() / 3;
    }

    Geometry* createGeometry( bool onlyCaps )
    {
        // create Geometry
        Geometry *g = new Geometry;

        // coordinates
        Vec4Array *a = onlyCaps ? _capsCoords : _coords;
        g->setVertexArray( a );

#if DEBUG_SHOW_WIREFRAME_VOLUMES // debugging code to display wireframe of shadow volumes

        // color
        Vec4Array *c = new Vec4Array;
        c->push_back( Vec4( 1.,0.,0.,1. ) );
        g->setColorArray( c );
        g->setColorBinding( Geometry::BIND_OVERALL );

        // primitive set
        g->addPrimitiveSet( new DrawArrays( PrimitiveSet::LINES, 0, a->size() ) );

#else

        // primitive set
        g->addPrimitiveSet( new DrawArrays( PrimitiveSet::TRIANGLES, 0, a->size() ) );

#endif

        return g;
    }

    void apply( Node& node )
    {
        pushState( node );

        traverse( node );

        popState( node );
    }

    void apply( Transform& transform )
    {
        pushState( transform );

        Matrix matrix;
        if( !_matrixStack.empty() )
            matrix = _matrixStack.back();

        transform.computeLocalToWorldMatrix( matrix, this );

        pushMatrix( matrix );

        traverse( transform );

        popMatrix();

        popState( transform );
    }

    void apply( Geode& geode )
    {
        pushState( geode );

        for( unsigned int i=0; i<geode.getNumDrawables(); ++i )
        {
            Drawable* drawable = geode.getDrawable( i );

            pushState( drawable->getStateSet() );

            apply( drawable );

            popState( drawable->getStateSet() );
        }

        popState( geode );
    }

    inline void pushState( const osg::Node &node )  { pushState( node.getStateSet(), &node ); }
    inline void popState( const osg::Node &node )  { popState( node.getStateSet(), &node ); }

    void pushState( const StateSet* stateset, const osg::Node *node = NULL )
    {
        if( stateset )
        {
            // get current blend value
            StateAttribute::GLModeValue prevBlendModeValue;
            if( _blendModeStack.empty() ) prevBlendModeValue = StateAttribute::INHERIT;
            else prevBlendModeValue = _blendModeStack.back();

            // get new blend value
            StateAttribute::GLModeValue newBlendModeValue = stateset->getMode( GL_BLEND );

            // handle protected and override on blend value
            if( !(newBlendModeValue & StateAttribute::PROTECTED) &&
                (prevBlendModeValue & StateAttribute::OVERRIDE) )
            {
                newBlendModeValue = prevBlendModeValue;
            }

            // push new blend value
            _blendModeStack.push_back( newBlendModeValue );
        }

        if( node ) {

            std::string str;
            node->getUserValue< std::string >( "Photorealism", str );

            if( !str.empty() ) {

                std::map< std::string, std::string > data( _photorealismData.top() );

                std::stringstream in( str );
                std::string currentKey;
                std::string currentValue;
                while( in )
                {
                    std::string s;
                    in >> s;
                    if( s.empty() )
                        continue;
                    char c = s[0];
                    bool isKey = !(( c >= '0' && c <= '9' ) || c == '.' || c == '"' || c == '\'' );
                    if( isKey )
                    {
                        if( !currentKey.empty() )
                            data.insert( make_pair( currentKey, currentValue ) );
                        currentKey = s;
                        currentValue = "";
                    }
                    else
                    {
                        if( currentValue.empty() )
                            currentValue = s;
                        else
                            currentValue += ' ' + s;
                    }
                }
                if( !currentKey.empty() )
                    data.insert( make_pair( currentKey, currentValue ) );

                _photorealismData.push( data );

            }
        }
    }

    void popState( const osg::StateSet *ss, const osg::Node *node = NULL )
    {
        if( ss )
            _blendModeStack.pop_back();

        if( node )
        {
            std::string str;
            node->getUserValue< std::string >( "Photorealism", str );

            if( !str.empty() )
                _photorealismData.pop();
        }
    }

    void pushMatrix(Matrix& matrix)
    {
        _matrixStack.push_back( matrix );
    }

    void popMatrix()
    {
        _matrixStack.pop_back();
    }

    void apply( Drawable* drawable )
    {
        StateAttribute::GLModeValue blendModeValue;
        if( _blendModeStack.empty() ) blendModeValue = StateAttribute::INHERIT;
        else blendModeValue = _blendModeStack.back();
        if( blendModeValue & StateAttribute::ON )
        {
            // ignore transparent drawables
            //return; // FIXME: There are semi-transparent parts in our models. How should we handle it?
        }

        std::map< std::string, std::string > &photorealism = _photorealismData.top();
        std::map< std::string, std::string >:: iterator it = photorealism.find( "Material.castShadow" );
        if( it != photorealism.end() )
        {
            std::stringstream in( it->second );
            bool castShadow;
            in >> castShadow;
            if( !castShadow )
                // do not process drawables with disabled shadow casting
                return;
        }

        TriangleCollectorFunctor tc( _coords, _capsCoords, _matrixStack.empty() ? NULL : &_matrixStack.back(),
                                     _lightPos, _shadowCastingFace, _options );
        drawable->accept( tc );
    }

protected:

    struct TriangleCollector
    {
        Vec4Array* _vertices;
        Vec4Array* _capsVertices;
        Matrix*    _matrix;
        Vec4       _lightPos;
        ShadowVolume::ShadowCastingFace _shadowCastingFace;
        int        _options;

        static inline Vec3 toVec3( const Vec4& v4)
        {
            // do not divide when 1. or 0.
            if( v4[3] == 1. || v4[3] == 0. )
                return Vec3( v4[0], v4[1], v4[2] );

            // perform division by third component
            Vec4::value_type n = 1. / v4[3];
            return Vec3( v4[0]*n, v4[1]*n, v4[2]*n );
        }

        inline void operator ()( const osg::Vec3& v1, const osg::Vec3& v2,
                                 const osg::Vec3& v3, bool treatVertexDataAsTemporary )
        {
            // transform vertices
            Vec3 t1( v1 );
            Vec3 t2( v2 );
            Vec3 t3( v3 );
            Vec3 lp3 = toVec3( _lightPos ); // lightpos converted to Vec3
            if( _matrix ) {
                t1 = t1 * *_matrix;
                t2 = t2 * *_matrix;
                t3 = t3 * *_matrix;
            }

            // compute which triangle side faces the light
            Vec3 n = (t2-t1) ^ (t3-t1);         // normal of the face
            bool isBack;
            if( _lightPos[3] == 0. )
               isBack = ( n * lp3 ) < 0;       // dot product with directional light vector
            else
               isBack = ( n * ( lp3-t1 ) ) < 0; // dot product with light position vector

            if( _shadowCastingFace != ShadowVolume::FRONT_AND_BACK ) {

                // skip faces that should not generate shadows
                // (when casting from FRONT or BACK face only)
                if( (_shadowCastingFace==ShadowVolume::FRONT) == isBack )
                    return;

            }

            // set correct vertex ordering
            if( isBack )
            {
                Vec3 tmp( t3 );
                t3 = t2;
                t2 = tmp;
            }

            // final vertices
            Vec4 f1( t1, 1. );
            Vec4 f2( t2, 1. );
            Vec4 f3( t3, 1. );
            Vec4 f4, f5, f6;
            if( _lightPos[3] == 0. ) {
                // directional light
                f4 = Vec4( -lp3, 0. );
                f5 = Vec4( -lp3, 0. );
                f6 = Vec4( -lp3, 0. );
            } else {
                // point light or spot light
                f4 = Vec4( t1 - lp3, 0. );
                f5 = Vec4( t2 - lp3, 0. );
                f6 = Vec4( t3 - lp3, 0. );
            }

            // store vertices
#if DEBUG_SHOW_WIREFRAME_VOLUMES // debugging code to display wireframe of shadow volumes
            _vertices->push_back( f1 );
            _vertices->push_back( f2 );
            _vertices->push_back( f2 );
            _vertices->push_back( f3 );
            _vertices->push_back( f3 );
            _vertices->push_back( f1 );
            _vertices->push_back( f1 );
            _vertices->push_back( f4 );
            _vertices->push_back( f2 );
            _vertices->push_back( f5 );
            _vertices->push_back( f3 );
            _vertices->push_back( f6 );
            if( _options & GENERATE_CAPS )
            {
                if( _options & CAPS_SEPARATE_ARRAY )
                {
                    _capsVertices->push_back( f1 );
                    _capsVertices->push_back( f2 );
                    _capsVertices->push_back( f2 );
                    _capsVertices->push_back( f3 );
                    _capsVertices->push_back( f3 );
                    _capsVertices->push_back( f1 );
                    _capsVertices->push_back( f4 );
                    _capsVertices->push_back( f5 );
                    _capsVertices->push_back( f5 );
                    _capsVertices->push_back( f6 );
                    _capsVertices->push_back( f6 );
                    _capsVertices->push_back( f4 );
                }
                else
                {
                    _vertices->push_back( f1 );
                    _vertices->push_back( f2 );
                    _vertices->push_back( f2 );
                    _vertices->push_back( f3 );
                    _vertices->push_back( f3 );
                    _vertices->push_back( f1 );
                    _vertices->push_back( f4 );
                    _vertices->push_back( f5 );
                    _vertices->push_back( f5 );
                    _vertices->push_back( f6 );
                    _vertices->push_back( f6 );
                    _vertices->push_back( f4 );
                }
            }
#else
            _vertices->push_back( f1 );
            _vertices->push_back( f4 );
            _vertices->push_back( f2 );

            _vertices->push_back( f2 );
            _vertices->push_back( f4 );
            _vertices->push_back( f5 );

            _vertices->push_back( f2 );
            _vertices->push_back( f5 );
            _vertices->push_back( f3 );

            _vertices->push_back( f3 );
            _vertices->push_back( f5 );
            _vertices->push_back( f6 );

            _vertices->push_back( f3 );
            _vertices->push_back( f6 );
            _vertices->push_back( f1 );

            _vertices->push_back( f1 );
            _vertices->push_back( f6 );
            _vertices->push_back( f4 );

            if( _options & GENERATE_CAPS )
            {
                if( _options & CAPS_SEPARATE_ARRAY )
                {
                    _capsVertices->push_back( f1 );
                    _capsVertices->push_back( f2 );
                    _capsVertices->push_back( f3 );
                    _capsVertices->push_back( f4 );
                    _capsVertices->push_back( f6 );
                    _capsVertices->push_back( f5 );
                }
                else
                {
                    _vertices->push_back( f1 );
                    _vertices->push_back( f2 );
                    _vertices->push_back( f3 );
                    _vertices->push_back( f4 );
                    _vertices->push_back( f6 );
                    _vertices->push_back( f5 );
                }
            }
#endif
        }
    };

    class TriangleCollectorFunctor : public TriangleFunctor< TriangleCollector >
    {
    public:
        TriangleCollectorFunctor( Vec4Array *vertices, Vec4Array *capsVertices,
                                  Matrix *m, const Vec4& lightPos,
                                  ShadowVolume::ShadowCastingFace shadowCastingFace,
                                  int options )
        {
            _vertices = vertices;
            _capsVertices = capsVertices;
            _matrix = m;
            _lightPos = lightPos;
            _shadowCastingFace = shadowCastingFace;
            _options = options;
        }
    };
    typedef std::vector< Matrix > MatrixStack;
    typedef std::vector< StateAttribute::GLModeValue > ModeStack;

    MatrixStack              _matrixStack;
    ModeStack                _blendModeStack;
    std::stack< std::map< std::string, std::string > > _photorealismData;

    ref_ptr< Vec4Array >     _coords;
    ref_ptr< Vec4Array >     _capsCoords;
    Vec4                     _lightPos;
    int                      _options;
    ShadowVolume::ShadowCastingFace _shadowCastingFace;
};


/** Clear drawable encapsulates glClear OpenGL command and enables an user to perform buffer clear
 *  operation during scene rendering. It is useful in multipass algorithms, for example by
 *  shadow volumes that needs to clear stencil buffer before each light-processing pass.
 *
 *  Bitwise OR of following values can be used for specifying which buffers are to be cleared:
 *  GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT and GL_ACCUM_BUFFER_BIT. */
class Clear : public osg::Drawable
{
   typedef osg::Drawable inherited;
public:
   /** Constructor that sets buffer mask to its default value,
    *  e.g. color and depth buffer will be cleared by the drawable. */
   Clear() : _bufferMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) {}
   /** Constructor taking buffer mask as parameter. */
   Clear( unsigned int bufferMask ) : _bufferMask( bufferMask ) {}
   /** Copy constructor using CopyOp to manage deep vs shallow copy. */
   Clear( const Clear& clear, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY ) :
      inherited( clear, copyop ) {}
   /** Destructor. */
   virtual ~Clear() {}

   virtual osg::Object* cloneType() const { return new Clear(); }
   virtual osg::Object* clone( const osg::CopyOp& copyop ) const { return new Clear( *this, copyop ); }
   virtual bool isSameKindAs( const osg::Object* obj ) const { return dynamic_cast< const Clear* >( obj ) != NULL; }
   virtual const char* libraryName() const { return "osg"; }
   virtual const char* className() const { return "Clear"; }

   /** Sets which buffers will be cleared. By default, color and depth buffers are cleared. */
   virtual void setBufferMask( unsigned int bufferMask ) { _bufferMask = bufferMask; }
   /** Returns bitmask indicating which buffers are cleared by the class. */
   inline unsigned int getBufferMask() const { return _bufferMask; }

   /** Draw implementation that performs stencil buffer clear. */
   virtual void drawImplementation( osg::RenderInfo& renderInfo ) const
   {
       glClear( _bufferMask );
   }

protected:
   unsigned int _bufferMask;
};


ShadowVolume::ShadowVolume() :
    _method( ZFAIL ),
    _renderingImplementation( CPU_TRIANGLE_SHADOW ),
    _stencilImplementation( STENCIL_AUTO ),
    _updateStrategy( UPDATE_EACH_FRAME ),
    _ambientPassDisabled( false ),
    _clearStencil( true ),
    _shadowCastingFace( FRONT_AND_BACK )
{
}


ShadowVolume::~ShadowVolume()
{
}


void ShadowVolume::setLight( Light* light )
{
    _light = light;


    //
    //  First pass state set
    //
    _ss1 = new StateSet;
    _ss1->setRenderBinDetails( 1, "RenderBin" );

    // disable the light
    if( _light )
        _ss1->setMode( GL_LIGHT0 + _light->getLightNum(),
                       StateAttribute::OFF | StateAttribute::OVERRIDE );

    // lighting should be on
    _ss1->setMode( GL_LIGHTING, StateAttribute::ON | StateAttribute::OVERRIDE );
}


void ShadowVolume::init()
{
    _dirty = false;

    // clear drawable used for clearing stencil buffer
    _clearDrawable = new Clear( GL_STENCIL_BUFFER_BIT );
    _clearDrawable->setUseDisplayList( false );
    _clearDrawable->getOrCreateStateSet()->setRenderBinDetails( 1, "RenderBin" );


    //
    //  First pass state set
    //
    //  _ss1 is initialized in setLight() method
    //


    //
    //  Second and third pass
    //
    _ss2 = new StateSet;
    _ss2->setRenderBinDetails( 2, "RenderBin" );
    _ss3 = new StateSet;
    _ss3->setRenderBinDetails( 3, "RenderBin" );
    _ss2caps = new StateSet;
    _ss2caps->setRenderBinDetails( 2, "RenderBin" );
    _ss3caps = new StateSet;
    _ss3caps->setRenderBinDetails( 3, "RenderBin" );
    _ss23 = new StateSet;
    _ss23->setRenderBinDetails( 2, "RenderBin" );
    _ss23caps = new StateSet;
    _ss23caps->setRenderBinDetails( 2, "RenderBin" );

    // no lighting
    _ss2->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss3->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss2caps->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss3caps->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23caps->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );

#if DEBUG_SHOW_WIREFRAME_VOLUMES // debugging code to display wireframe of shadow volumes

    // blend
    BlendFunc* blend2 = new BlendFunc;
    blend2->setFunction( BlendFunc::ONE, BlendFunc::ONE );
    _ss2->setAttributeAndModes( blend2, StateAttribute::ON | StateAttribute::OVERRIDE );

    // depthFunc to LEQUAL (LESS does not work!)
    Depth* depth2 = new Depth;
    depth2->setWriteMask( false );
    depth2->setFunction( Depth::LEQUAL );
    _ss2->setAttributeAndModes( depth2, StateAttribute::ON );

#else

    // do not write to color buffers
    ColorMask *colorMask23 = new ColorMask;
    colorMask23->setMask( false, false, false, false );
    _ss2->setAttribute( colorMask23 );
    _ss3->setAttribute( colorMask23 );
    _ss2caps->setAttribute( colorMask23 );
    _ss3caps->setAttribute( colorMask23 );
    _ss23->setAttribute( colorMask23 );
    _ss23caps->setAttribute( colorMask23 );

    // depth
    Depth *depth23 = new Depth;
    depth23->setWriteMask( false );
    depth23->setFunction( Depth::LEQUAL );
    _ss2->setAttributeAndModes( depth23, StateAttribute::ON | StateAttribute::OVERRIDE );
    _ss3->setAttributeAndModes( depth23, StateAttribute::ON | StateAttribute::OVERRIDE );
    _ss23->setAttributeAndModes( depth23, StateAttribute::ON | StateAttribute::OVERRIDE );

    // caps do not need depth test (note: disabling GL_DEPTH_TEST disables depth buffer updates as well)
    _ss2caps->setMode( GL_DEPTH_TEST, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss3caps->setMode( GL_DEPTH_TEST, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23caps->setMode( GL_DEPTH_TEST, StateAttribute::OFF | StateAttribute::OVERRIDE );

    // stencil function (pass 2)
    _stencil2 = new Stencil;
    _ss2->setAttributeAndModes( _stencil2, StateAttribute::ON | StateAttribute::OVERRIDE );

    // stencil function (pass 3)
    _stencil3 = new Stencil;
    _ss3->setAttributeAndModes( _stencil3, StateAttribute::ON | StateAttribute::OVERRIDE );

    // stencil function (pass 2, caps, zFail only, back faces)
    Stencil *stencil2caps = new Stencil;
    stencil2caps->setFunction( Stencil::NEVER, 0, ~0u );
    stencil2caps->setOperation( Stencil::INCR_WRAP, Stencil::KEEP, Stencil::KEEP );
    _ss2caps->setAttributeAndModes( stencil2caps, StateAttribute::ON | StateAttribute::OVERRIDE );

    // stencil function (pass 3, caps, zFail only, front faces)
    Stencil *stencil3caps = new Stencil;
    stencil3caps->setFunction( Stencil::NEVER, 0, ~0u );
    stencil3caps->setOperation( Stencil::DECR_WRAP, Stencil::KEEP, Stencil::KEEP );
    _ss3caps->setAttributeAndModes( stencil3caps, StateAttribute::ON | StateAttribute::OVERRIDE );

    // two sided stenciling
    _stencil23 = new StencilTwoSided;
    _ss23->setAttributeAndModes( _stencil23, StateAttribute::ON | StateAttribute::OVERRIDE );

    // two sided stenciling (caps, zFail only)
    StencilTwoSided *stencil23caps = new StencilTwoSided;
    stencil23caps->setFunction( StencilTwoSided::FRONT, StencilTwoSided::NEVER, 0, ~0u );
    stencil23caps->setFunction( StencilTwoSided::BACK,  StencilTwoSided::NEVER, 0, ~0u );
    stencil23caps->setOperation( StencilTwoSided::FRONT, StencilTwoSided::INCR_WRAP, StencilTwoSided::KEEP, StencilTwoSided::KEEP );
    stencil23caps->setOperation( StencilTwoSided::BACK,  StencilTwoSided::DECR_WRAP, StencilTwoSided::KEEP, StencilTwoSided::KEEP );
    _ss23caps->setAttributeAndModes( stencil23caps, StateAttribute::ON | StateAttribute::OVERRIDE );

    // cull back faces (pass 2)
    _cullFace2 = new CullFace;
    _ss2->setAttributeAndModes( _cullFace2, StateAttribute::ON | StateAttribute::OVERRIDE );
    _ss2caps->setAttributeAndModes( _cullFace2, StateAttribute::ON | StateAttribute::OVERRIDE );

    // cull front faces (pass 3)
    _cullFace3 = new CullFace;
    _ss3->setAttributeAndModes( _cullFace3, StateAttribute::ON | StateAttribute::OVERRIDE );
    _ss3caps->setAttributeAndModes( _cullFace3, StateAttribute::ON | StateAttribute::OVERRIDE );

    // cull no faces
    _ss23->setMode( GL_CULL_FACE, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23caps->setMode( GL_CULL_FACE, StateAttribute::OFF | StateAttribute::OVERRIDE );

    // perform stencil and culling setup
    updateStencilingAndCulling();

    // geometry shader (if required)
    if( _renderingImplementation == GEOMETRY_SHADER_TRIANGLE_SHADOW ) {

        _volumeProgram = new Program;
        _volumeProgram->addShader( new Shader( Shader::VERTEX, volumeVertexShader ) );
        _volumeProgram->addShader( new Shader( Shader::GEOMETRY, volumeGeometryShader ) );
        _volumeProgram->addShader( new Shader( Shader::FRAGMENT, volumeFragmentShader ) );
        _volumeProgram->setParameter( GL_GEOMETRY_VERTICES_OUT_EXT, 8 );
        _volumeProgram->setParameter( GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES );
        _volumeProgram->setParameter( GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP );

        _lightPosUniform = new Uniform( "lightpos", Vec4() );

        _ss2->setAttribute( _volumeProgram );
        _ss2->addUniform( _lightPosUniform );
        _ss3->setAttribute( _volumeProgram );
        _ss3->addUniform( _lightPosUniform );
        _ss23->setAttribute( _volumeProgram );
        _ss23->addUniform( _lightPosUniform );

    }
#endif


    //
    //  Fourth pass state set
    //
    _ss4 = new StateSet;
    _ss4->setRenderBinDetails( 4, "RenderBin" );

    // lighting have to be ON
    _ss4->setMode( GL_LIGHTING, StateAttribute::ON | StateAttribute::OVERRIDE );

    // blend
    BlendFunc* blend4 = new BlendFunc;
    blend4->setFunction( BlendFunc::ONE, BlendFunc::ONE );
    _ss4->setAttributeAndModes( blend4, StateAttribute::ON | StateAttribute::OVERRIDE );

    // depthFunc to LEQUAL (LESS does not work!)
    Depth* depth4 = new Depth;
    depth4->setWriteMask( false );
    depth4->setFunction( Depth::LEQUAL );
    _ss4->setAttributeAndModes( depth4, StateAttribute::ON | StateAttribute::OVERRIDE );

    // ambient intensity to zero
    // FIXME: what about other LightModel components?
    LightModel* lm4 = new LightModel;
    lm4->setAmbientIntensity( Vec4( 0., 0., 0., 0. ) );
    _ss4->setAttribute( lm4 );

    Stencil *stencil4 = new Stencil;
    stencil4->setFunction( Stencil::EQUAL, 0, ~0u );
    stencil4->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::KEEP );
    _ss4->setAttributeAndModes( stencil4, StateAttribute::ON | StateAttribute::OVERRIDE );
}


void ShadowVolume::updateStencilingAndCulling()
{
    // do nothing if objects were not created yet
    if( !_stencil2.valid() )
        return;

    // stencil function (pass 2)
    _stencil2->setFunction( Stencil::ALWAYS, 0, ~0u );
    if( _method == ZPASS )
        _stencil2->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::INCR_WRAP );
    else
        _stencil2->setOperation( Stencil::KEEP, Stencil::INCR_WRAP, Stencil::KEEP );
    //_stencil2->setOperation( Stencil::KEEP, Stencil::INCR, Stencil::KEEP );

    // stencil function (pass 3)
    _stencil3->setFunction( Stencil::ALWAYS, 0, ~0u );
    if( _method == ZPASS )
        _stencil3->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::DECR_WRAP );
    else
        _stencil3->setOperation( Stencil::KEEP, Stencil::DECR_WRAP, Stencil::KEEP );
    //_stencil3->setOperation( Stencil::KEEP, Stencil::DECR, Stencil::KEEP );

    // two sided stenciling
    _stencil23->setFunction( StencilTwoSided::FRONT, StencilTwoSided::ALWAYS, 0, ~0u );
    _stencil23->setFunction( StencilTwoSided::BACK,  StencilTwoSided::ALWAYS, 0, ~0u );
    if( _method == ZPASS ) {
        _stencil23->setOperation( StencilTwoSided::FRONT, StencilTwoSided::KEEP, StencilTwoSided::KEEP, StencilTwoSided::INCR_WRAP );
        _stencil23->setOperation( StencilTwoSided::BACK,  StencilTwoSided::KEEP, StencilTwoSided::KEEP, StencilTwoSided::DECR_WRAP );
    } else {
        _stencil23->setOperation( StencilTwoSided::FRONT, StencilTwoSided::KEEP, StencilTwoSided::INCR_WRAP, StencilTwoSided::KEEP );
        _stencil23->setOperation( StencilTwoSided::BACK,  StencilTwoSided::KEEP, StencilTwoSided::DECR_WRAP, StencilTwoSided::KEEP );
    }

    // cull back faces (pass 2)
    _cullFace2->setMode( _method == ZPASS ? CullFace::BACK : CullFace::FRONT );

    // cull front faces (pass 3)
    _cullFace3->setMode( _method == ZPASS ? CullFace::FRONT : CullFace::BACK );
}


void ShadowVolume::update( NodeVisitor& nv )
{
}


void ShadowVolume::cull( osgUtil::CullVisitor& cv )
{
    // get glFrontFace settings
#if 0
    // FIXME: there should be osg::State::getCurrentAttribute(), currently we need to
    //        "capture" whole state set
    // note: This code is not debugged yet. The most important problem seems to be
    //       a kind of threading issue in captureCurrentState...
    State* state = cv.getState();
    ref_ptr< StateSet> currentState = new StateSet;
    state->captureCurrentState( *currentState.get() );
    const FrontFace* frontFace = dynamic_cast< const FrontFace* >(
            currentState->getAttribute( StateAttribute::FRONTFACE ) );
    FrontFace::Mode frontFaceMode = frontFace ? frontFace->getMode() : FrontFace::COUNTER_CLOCKWISE;
#else
    //FrontFace::Mode frontFaceMode = FrontFace::COUNTER_CLOCKWISE;
#endif

    // if no light, render scene default way
    if( !_light ) {
        _shadowedScene->Group::traverse( cv );
        return;
    }

    // pass 4: add light using stencil
    // The pass is culled in the first place as it is always culled (in contrary to pass 1).
    // Its traversed lights are used in following getLightPositionalState.
    // Note: traversing order of passes 1 to 4 does not matter. Rendering order is given
    //       by RenderBin's setRenderBinDetails().
    cv.pushStateSet( _ss4 );
    _shadowedScene->Group::traverse( cv );
    cv.popStateSet();

    // clear stencil buffer - bin number 1 schedules it before 2nd and 3rd pass
    if( _clearStencil )
        cv.addDrawable( _clearDrawable, cv.getModelViewMatrix() );

    // get light positional state,
    // if no light, return
    Vec4 lightPos;
    Vec3 lightDir;
    if( !getLightPositionalState( cv, _light, lightPos, lightDir,
                                  _renderingImplementation != GEOMETRY_SHADER_TRIANGLE_SHADOW ) )
        return;

    // pass 1: ambient pass
    if( !_ambientPassDisabled )
    {
        cv.pushStateSet( _ss1 );
        _shadowedScene->Group::traverse( cv );
        cv.popStateSet();
    }

    // pass 2 and 3
    if( _renderingImplementation != GEOMETRY_SHADER_TRIANGLE_SHADOW  )
    {

        // create shadow volume geometry
        if( _updateStrategy == UPDATE_EACH_FRAME ||
           (_updateStrategy == MANUAL_INVALIDATE && _shadowGeometry == NULL ) )
        {
            updateShadowData( lightPos );
        }

        if( _shadowGeometry ) {

            bool twoSidedStencil = _stencilImplementation == STENCIL_TWO_SIDED;

            // if STENCIL_AUTO, assign proper value to usedStencil
            if( _stencilImplementation == STENCIL_AUTO )
            {
#if 0
                // get extensions
                StencilTwoSided::Extensions *extensions = StencilTwoSided::getExtensions(
                    cv.getState()->getContextID(), false );

                // see two sided stenciling availability, otherwise fallback to one side stenciling
                // note: extensions may be null and currently, it is user responsibility to allocate them
                //       for the graphics context, for example in window realize callback
                if( extensions && ( extensions->isOpenGL20Supported() ||
                    extensions->isEXTstencilTwoSideSupported() || extensions->isATIseparateStencilSupported() ) )
                    twoSidedStencil = true;
#else
                GraphicsContext *gc = cv.getState()->getGraphicsContext();
#if 0
                if( gc->getGLVersionNumber() >= 2.0 ||
                    //osg::isGLExtensionSupported( cv.getState()->getContextID(), "GL_EXT_stencil_two_side" ) ||
                    gc->isGLExtensionSupported( "GL_EXT_stencil_two_side" ) ||
                    gc->isGLExtensionSupported( "GL_ATI_separate_stencil" ) )
#elif 1
                if( gc->isGLExtensionSupported( 2.0, "GL20_separate_stencil" ) ||
                    gc->isGLExtensionSupported( "GL_EXT_stencil_two_side" ) ||
                    gc->isGLExtensionSupported( "GL_ATI_separate_stencil" ) )
#elif 0
                if( gc->isGLVersionFeatureOrExtensionSupported( 2.0, "GL20_separate_stencil",
                                                                "GL_EXT_stencil_two_side",
                                                                "GL_ATI_separate_stencil" ) )
#endif
                    twoSidedStencil = true;
#endif
            }

            // put message to log about one/two sided stenciling
            OSG_DEBUG << "ShadowVolumes: Using " << ( twoSidedStencil ? "two" : "one" ) << " side stenciling." << std::endl;

            if( twoSidedStencil ) {

                // two sided pass
                cv.pushStateSet( _ss23 );
                _shadowGeometry->accept( cv );
                cv.popStateSet();

                // two sided pass for caps
                if( _method != ZPASS ) {
                    cv.pushStateSet( _ss23caps );
                    _shadowCapsGeometry->accept( cv );
                    cv.popStateSet();
                }

            } else {

                // pass 2
                cv.pushStateSet( _ss2 );
                _shadowGeometry->accept( cv );
                cv.popStateSet();

                // pass 2 caps
                if( _method != ZPASS ) {
                    cv.pushStateSet( _ss2caps );
                    _shadowCapsGeometry->accept( cv );
                    cv.popStateSet();
                }

                // pass 3
                cv.pushStateSet( _ss3 );
                _shadowGeometry->accept( cv );
                cv.popStateSet();

                // pass 3 caps
                if( _method != ZPASS ) {
                    cv.pushStateSet( _ss3caps );
                    _shadowCapsGeometry->accept( cv );
                    cv.popStateSet();
                }

            }
        }

    } else {

        // setup uniforms
        _lightPosUniform->set( lightPos );

        // pass 2
        cv.pushStateSet( _ss2 );
        _shadowedScene->Group::traverse( cv );
        cv.popStateSet();

        // pass 3
        cv.pushStateSet( _ss3 );
        _shadowedScene->Group::traverse( cv );
        cv.popStateSet();

    }
}


void ShadowVolume::cleanSceneGraph()
{
}


bool ShadowVolume::getLightPositionalState( osgUtil::CullVisitor& cv, const Light* light,
        Vec4& lightPos, Vec3& lightDir, bool resultInLocalCoordinates )
{
    // no light
    if( !light )
        return false;

    // get positional state
    osgUtil::RenderStage* rs = cv.getRenderStage();
    osgUtil::PositionalStateContainer::AttrMatrixList& aml =
        rs->getPositionalStateContainer()->getAttrMatrixList();

    RefMatrix* matrix = NULL;
    bool found = false;

    // find the light and its matrix
    // note: it finds the last light occurence with its associated matrix
    for( osgUtil::PositionalStateContainer::AttrMatrixList::iterator itr = aml.begin();
        itr != aml.end();
        ++itr )
    {
        const Light* l = dynamic_cast< const Light* >( itr->first.get() );
        if( l && l == light )
        {
            found = true;
            matrix = itr->second.get();
        }
    }

    // handle not found
    if( !found )
        return false;

    // get light position
    lightPos = light->getPosition();

    // get light direction
    if( lightPos[3] == 0 )
        lightDir.set( -lightPos[0], -lightPos[1], -lightPos[2] );
    else
        lightDir = light->getDirection();

    // transform light to the correct coordinate system
    if( resultInLocalCoordinates ) {

        // transform light to the local coordinates
        Matrix localToWorld = Matrix::inverse( *cv.getModelViewMatrix() );
        if( matrix )
            localToWorld.preMult( *matrix );

        lightPos = lightPos * localToWorld;
        lightDir = Matrix::transform3x3( lightDir, localToWorld );

    } else {

        // transform light to the world space
        if( matrix ) {
            lightPos = lightPos * *matrix;
            lightDir = Matrix::transform3x3( lightDir, *matrix );
        }

    }
    lightDir.normalize();

    return true;
}


void ShadowVolume::updateShadowData( const Vec4 &lightPos )
{
    // get time
    Timer t;

    // create shadow volume geometry
    _shadowGeometry = NULL;
    ShadowVolumeGeometryGenerator svGenerator( lightPos, _shadowCastingFace, _method == ZPASS ?
            ShadowVolumeGeometryGenerator::NO_CAPS :
            ShadowVolumeGeometryGenerator::GENERATE_CAPS | ShadowVolumeGeometryGenerator::CAPS_SEPARATE_ARRAY );
    _shadowedScene->Group::traverse( svGenerator );

    // create basic shadow volume
    Geode *geode = new Geode;
    Drawable *d = svGenerator.createGeometry( false );
    d->setUseDisplayList( true );
    geode->addDrawable( d );
    _shadowGeometry = geode;

    if( _method != ZPASS )
    {
        // create caps as separate drawable
        d = svGenerator.createGeometry( true );
        d->setUseDisplayList( true );
        geode = new Geode;
        geode->addDrawable( d );
        _shadowCapsGeometry = geode;
    }
    else
        _shadowCapsGeometry = NULL;

    // notify about the time and number of triangles
    OSG_NOTICE << "ShadowVolume: Shadow geometry generated in " << int( t.time_m() + 0.5 )
               << "ms using " << svGenerator.getNumTriangles() << " triangles." << std::endl;
}


void ShadowVolume::invalidateShadowData()
{
    _shadowGeometry = NULL;
}


void ShadowVolume::setMethod( Method method )
{
    if( _method == method )
        return;

    _method = method;

    // perform stencil and culling update
    updateStencilingAndCulling();
}


void ShadowVolume::setRenderingImplementation( ShadowVolume::RenderingImplementation implementation )
{
    _renderingImplementation = implementation;
}


void ShadowVolume::setStencilImplementation( ShadowVolume::StencilImplementation implementation )
{
    _stencilImplementation = implementation;
}


void ShadowVolume::setUpdateStrategy( UpdateStrategy strategy )
{
    _updateStrategy = strategy;
}


void ShadowVolume::disableAmbientPass( bool value )
{
    _ambientPassDisabled = value;
}


void ShadowVolume::setClearStencil( bool value )
{
    _clearStencil = value;
}


void ShadowVolume::setShadowCastingFace( ShadowCastingFace face )
{
    _shadowCastingFace = face;
}


void ShadowVolume::setup( osgViewer::View *view )
{
   setupDisplaySettings( view );
   setupCamera( view->getCamera() );
}


static Matrixd makeFrustumInfiniteZFar( double left,
                                        double right,
                                        double bottom,
                                        double top,
                                        double zNear )
{
   double x, y, a, b, c, d;
   x = (2.0 * zNear) / (right - left);
   y = (2.0 * zNear) / (top - bottom);
   a = (right + left) / (right - left);
   b = (top + bottom) / (top - bottom);
   c = -1.0;
   d = -2.0 * zNear;
   return Matrixd( x, 0, 0, 0,
                   0, y, 0, 0,
                   a, b, c, -1,
                   0, 0, d, 0 );
}


void ShadowVolume::setupCamera( osg::Camera *camera )
{
   // setup zFar to infinity (required by shadow volume algorithm)
   double left, right, bottom, top, zNear, zFar;
   camera->getProjectionMatrixAsFrustum( left, right, bottom, top, zNear, zFar );
   camera->setProjectionMatrix( makeFrustumInfiniteZFar( left, right, bottom, top, zNear ) );
   camera->setComputeNearFarMode( CullSettings::DO_NOT_COMPUTE_NEAR_FAR );

   // clear stencil
   camera->setClearMask( camera->getClearMask() | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}


void ShadowVolume::setupDisplaySettings( osg::DisplaySettings *ds )
{
   // forces allocation of stencil and depth buffer
   ds->setMinimumNumStencilBits( 8 );
   ds->setDepthBuffer( true );
}


void ShadowVolume::setupDisplaySettings( osgViewer::View *view )
{
   // test whether ViewerBase is still not realized
   osgViewer::ViewerBase *vb = view->getViewerBase();
   if( vb->isRealized() ) {
      OSG_FATAL << "ViewerBase is already realized. Can not setup osgViewer::View's "
                   "DisplaySettings for use with shadow volumes." << std::endl;
      return;
   }

   // allocate display settings if required (by copying global display settings object)
   if( !view->getDisplaySettings() )
      view->setDisplaySettings( new DisplaySettings( *DisplaySettings::instance().get() ) );

   // setup settings
   setupDisplaySettings( view->getDisplaySettings() );
}


#if 0 // test code for ShadowVolume class

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/io_utils>

using namespace osg;
using namespace osgViewer;


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


class Detect : public GraphicsOperation
{
public:

    Detect() : GraphicsOperation( "Detect", false ), done( false ) {}

    virtual void operator ()( GraphicsContext* context )
    {
/*        StencilTwoSided::Extensions *extensions = StencilTwoSided::getExtensions(
            context->getState()->getContextID(), true );
        if( extensions->isOpenGL20Supported() )
            OSG_NOTICE << "StencilTwoSided: using OpenGL 2.0 API." << std::endl;
        else if( extensions->isStencilTwoSidedSupported() )
            OSG_NOTICE << "StencilTwoSided: using EXT_stencil_two_side extension." << std::endl;
        else
            OSG_NOTICE << "StencilTwoSided: not supported." << std::endl;*/
        done = true;
    }

protected:
    bool done;
};


int main()
{
   // load scene
//   Node *model = osgDB::readNodeFile( "cubemap-test.iv" );
//   Node *model = osgDB::readNodeFile( "triangleMix.iv" );
   ref_ptr< Node > model = osgDB::readNodeFile( "PointLight_Cubes10x10.iv" );
//   Node *model = osgDB::readNodeFile( "PointLight_Cubes40x25.iv" );
   if( !model ) {
      OSG_FATAL << "Can not load model." << std::endl;
      return -1;
   }

#if 1
   // optimize the scene graph
   Timer t;
   osgUtil::Optimizer optimizer;
   optimizer.optimize( model, osgUtil::Optimizer::ALL_OPTIMIZATIONS );
   double dt = t.time_m();
   OSG_NOTICE << "Optimization completed in " << dt << "ms." << std::endl;
#endif

   // find light
   FindLightVisitor flv;
   model->accept( flv );
   if( !flv.getLight() )
      OSG_WARN << "No light in the model. No shadows will be displayed." << std::endl;

   // setup shadows
   osgShadow::ShadowedScene *ss = new osgShadow::ShadowedScene;
   ss->addChild( model );
   osgShadow::ShadowVolume *sv = new osgShadow::ShadowVolume;
   sv->setLight( flv.getLight() );
   //sv->setShadowCastingFace( ShadowVolume::FRONT );
   //sv->setShadowCastingFace( ShadowVolume::BACK );
   ss->setShadowTechnique( sv );

   // forces allocation of stencil buffer
   DisplaySettings::instance()->setMinimumNumStencilBits( 8 );

   // setup viewer
   Viewer viewer;
   viewer.setThreadingModel( Viewer::CullThreadPerCameraDrawThreadPerContext );
//   viewer.setThreadingModel( Viewer::SingleThreaded );
   viewer.addEventHandler( new osgViewer::StatsHandler );
   viewer.addEventHandler( new osgViewer::ThreadingHandler );
   viewer.setSceneData( ss );
   double left, right, bottom, top, zNear, zFar;
   viewer.getCamera()->getProjectionMatrixAsFrustum( left, right, bottom, top, zNear, zFar );
   viewer.getCamera()->setProjectionMatrixAsFrustum( left, right, bottom, top, zNear, FLT_MAX );
   viewer.getCamera()->setComputeNearFarMode( CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
   viewer.getCamera()->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
   viewer.setRealizeOperation( new Detect );
   viewer.realize();
   viewer.run();

   return 0;
}

#endif
