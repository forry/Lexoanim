/**
 * @file
 * ShadowVolume class implementation.
 *
 * @author PCJohn (Jan Pečiva), Foreigner (Tomas Starka)
 */

#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Stencil>
#include <osg/StencilTwoSided>
#include <osg/TriangleFunctor>
#include <osg/GraphicsContext>
#include <osgShadow/ShadowedScene>
#include <osgViewer/ViewerBase>
#include <osgViewer/View>
#include <iostream>

#include "ShadowVolume.h"
#include "ClearGLBuffersDrawable.h"
//#include "SVKeyboardHandler.h"
//#include "RealizeOperation.h"

#include "shader_utils.h"

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

ShadowVolume::ShadowVolume():
   //_occluders_dirty(true),
ShadowTechnique(),
   _megaTheFuckDontCallMeAgain(true),
_mode(ShadowVolumeGeometryGenerator::CPU_RAW),
_exts(0),
_stencilImplementation(STENCIL_AUTO),
_ambientPassDisabled(false),
_updateStrategy(MANUAL_INVALIDATE),
_clearDrawable(new ClearGLBuffersDrawable(GL_STENCIL_BUFFER_BIT))
{
   init();
}


ShadowVolume::~ShadowVolume()
{
}


void ShadowVolume::setLight( Light* light )
{
    _light = light;
    //Vec4 lp3 = light->getPosition();

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
   if(!_dirty || !_megaTheFuckDontCallMeAgain) return;
   _megaTheFuckDontCallMeAgain = false;

   _clearDrawable->setUseDisplayList( false );
   _clearDrawable->getOrCreateStateSet()->setRenderBinDetails( 1, "RenderBin" );
   //notify(NOTICE)<<"INIT"<<std::endl;

    /* FIXME: shaders programs are allocated on heap and there is no handle to release them.
     * Could try to use some OSG string with ref_ptr if any.
     */
    //_volumeShader = createProgram("shadowtest.vert","shadowtest.glsl","shadowtest.frag");
    _volumeShader = new Program;
    _volumeShader->addShader( new Shader( Shader::VERTEX, volumeVertexShader ) );
    _volumeShader->addShader( new Shader( Shader::GEOMETRY, volumeGeometryShader ) );
    _volumeShader->addShader( new Shader( Shader::FRAGMENT, volumeFragmentShader ) );
    // _volumeShader = createProgram("shadowtest.vert",NULL,"shadowtest.frag");
    _lightPosUniform = new Uniform("lightpos", Vec4());
    _mode_unif = new Uniform("mode",_mode);
    _just_sides = new Uniform("just_caps",0);
    _just_caps = new Uniform("just_caps",1);
    _volumeShader->setParameter( GL_GEOMETRY_VERTICES_OUT_EXT, 8 );
    _volumeShader->setParameter( GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES );
    _volumeShader->setParameter( GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP );
    

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

    //shaders
    _ss2->addUniform(_lightPosUniform);
    _ss2->addUniform(_mode_unif);
    _ss2->addUniform(_just_sides);
    _ss3->addUniform(_lightPosUniform);
    _ss3->addUniform(_mode_unif);
    _ss3->addUniform(_just_sides);

    // no lighting
    _ss2->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss3->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );

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
    //debug
    //ColorMask *colorMask1 = new ColorMask;
    //colorMask1->setMask( false, false, false, false );
    //_ss1->setAttribute( colorMask1 );

    // do not write to color buffers
    ColorMask *colorMask23 = new ColorMask;
    colorMask23->setMask( false, false, false, false );
    _ss2->setAttribute( colorMask23 );
    _ss3->setAttribute( colorMask23 );

    /*BlendFunc* blend = new BlendFunc;
    blend->setFunction( BlendFunc::ONE, BlendFunc::ONE );*/
    //_ss2->setAttributeAndModes( blend, StateAttribute::ON | StateAttribute::OVERRIDE );
    //_ss3->setAttributeAndModes( blend, StateAttribute::ON | StateAttribute::OVERRIDE );


    // depth
    Depth *depth23 = new Depth;
    depth23->setWriteMask( false );
    depth23->setFunction( Depth::LEQUAL );
    _ss2->setAttributeAndModes( depth23, StateAttribute::ON );
    _ss3->setAttributeAndModes( depth23, StateAttribute::ON );


    // stencil function (pass 2)
    Stencil* stencil2 = new Stencil;
    stencil2->setFunction( Stencil::ALWAYS, 0, ~0u );
    stencil2->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::INCR_WRAP );
    _ss2->setAttributeAndModes( stencil2, StateAttribute::ON );

    // stencil function (pass 3)
    Stencil* stencil3 = new Stencil;
    stencil3->setFunction( Stencil::ALWAYS, 0, ~0u );
    stencil3->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::DECR_WRAP );
    _ss3->setAttributeAndModes( stencil3, StateAttribute::ON );

    // cull back faces (pass 2)
    CullFace *cullFace2 = new CullFace;
    cullFace2->setMode( CullFace::BACK );
    _ss2->setAttributeAndModes( cullFace2, StateAttribute::ON | StateAttribute::OVERRIDE );

    // cull front faces (pass 3)
    CullFace *cullFace3 = new CullFace;
    cullFace3->setMode( CullFace::FRONT );
    _ss3->setAttributeAndModes( cullFace3, StateAttribute::ON | StateAttribute::OVERRIDE );

#endif

    /* TwoSided Stencil state set */
    _ss23 = new StateSet;
    _ss23->setRenderBinDetails( 2, "RenderBin" );
    
    _ss23->addUniform(_lightPosUniform);
    _ss23->addUniform(_mode_unif);
    _ss23->addUniform(_just_sides);

    StencilTwoSided *stencil23 = new StencilTwoSided;
    stencil23->setFunction(StencilTwoSided::FRONT, StencilTwoSided::ALWAYS, 0, ~0u);
    stencil23->setOperation(StencilTwoSided::FRONT, StencilTwoSided::KEEP, StencilTwoSided::KEEP, StencilTwoSided::INCR_WRAP);
    stencil23->setFunction(StencilTwoSided::BACK, StencilTwoSided::ALWAYS, 0, ~0u);
    stencil23->setOperation(StencilTwoSided::BACK, StencilTwoSided::KEEP, StencilTwoSided::KEEP, StencilTwoSided::DECR_WRAP);

    _ss23->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23->setMode( GL_CULL_FACE, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23->setAttribute( colorMask23 );
    _ss23->setAttributeAndModes( depth23, StateAttribute::ON );
    _ss23->setAttributeAndModes( stencil23, StateAttribute::ON );

    /* Caps StateSets */
    _ss2_caps = new StateSet;
    _ss2_caps->setRenderBinDetails( 2, "RenderBin" );

    Stencil* stencil_caps2 = new Stencil;
    stencil_caps2->setFunction( Stencil::NEVER, 0, ~0u );
    stencil_caps2->setOperation( Stencil::INCR_WRAP, Stencil::KEEP, Stencil::KEEP );

    _ss2_caps->setMode( GL_DEPTH_TEST, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss2_caps->setAttributeAndModes( cullFace2, StateAttribute::ON | StateAttribute::OVERRIDE );
    //_ss2_caps->setMode( GL_CULL_FACE, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss2_caps->setAttributeAndModes( stencil_caps2, StateAttribute::ON );
    _ss2_caps->addUniform(_lightPosUniform);
    _ss2_caps->addUniform(_mode_unif);
    _ss2_caps->addUniform(_just_caps);
    _ss2_caps->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss2_caps->setAttribute( colorMask23 );



    _ss3_caps = new StateSet;
    _ss3_caps->setRenderBinDetails( 3, "RenderBin" );

    Stencil* stencil_caps3 = new Stencil;
    stencil_caps3->setFunction( Stencil::NEVER, 0, ~0u );
    stencil_caps3->setOperation( Stencil::DECR_WRAP, Stencil::KEEP, Stencil::KEEP );

    _ss3_caps->setMode( GL_DEPTH_TEST, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss3_caps->setAttributeAndModes( cullFace3, StateAttribute::ON | StateAttribute::OVERRIDE );
    _ss3_caps->setAttributeAndModes( stencil_caps3, StateAttribute::ON );
    //_ss3_caps->setMode( GL_CULL_FACE, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss3_caps->addUniform(_lightPosUniform);
    _ss3_caps->addUniform(_mode_unif);
    _ss3_caps->addUniform(_just_caps);
    _ss3_caps->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss3_caps->setAttribute( colorMask23 );


    _ss23_caps = new StateSet;
    _ss23_caps->setRenderBinDetails( 2, "RenderBin" );
    
    _ss23_caps->addUniform(_lightPosUniform);
    _ss23_caps->addUniform(_mode_unif);

    /*Depth *depth23_debug = new Depth;
    depth23_debug->setWriteMask( false );
    depth23_debug->setFunction( Depth::ALWAYS );
    _ss23_caps->setAttributeAndModes(depth23_debug);*/
    //_ss23_caps->setAttribute(cullFace3);

    StencilTwoSided *stencil23_caps = new StencilTwoSided;
    stencil23_caps->setFunction(StencilTwoSided::FRONT, StencilTwoSided::NEVER, 0, ~0u);
    stencil23_caps->setOperation(StencilTwoSided::FRONT, StencilTwoSided::INCR_WRAP, StencilTwoSided::KEEP, StencilTwoSided::KEEP);
    stencil23_caps->setFunction(StencilTwoSided::BACK, StencilTwoSided::NEVER, 0, ~0u);
    stencil23_caps->setOperation(StencilTwoSided::BACK, StencilTwoSided::DECR_WRAP, StencilTwoSided::KEEP, StencilTwoSided::KEEP);

    _ss23_caps->setMode( GL_LIGHTING, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23_caps->setMode( GL_CULL_FACE, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23_caps->setMode( GL_DEPTH_TEST, StateAttribute::OFF | StateAttribute::OVERRIDE );
    _ss23_caps->setAttribute( colorMask23 );
    _ss23_caps->addUniform(_lightPosUniform);
    _ss23_caps->addUniform(_mode_unif);
    _ss23_caps->addUniform(_just_caps);
    _ss23_caps->setAttributeAndModes( stencil23_caps, StateAttribute::ON );




    //
    //  Fourth pass state set
    //
    _ss4 = new StateSet;
    _ss4->setRenderBinDetails( 4, "RenderBin" );

    // lighting have to be ON
    _ss4->setMode( GL_LIGHTING, StateAttribute::ON | StateAttribute::OVERRIDE );
    /*_ss4->setMode( GL_LIGHT0 + _light->getLightNum(),
                       StateAttribute::ON | StateAttribute::OVERRIDE );*/

    // blend
    BlendFunc* blend4 = new BlendFunc;
    blend4->setFunction( BlendFunc::ONE, BlendFunc::ONE );
    _ss4->setAttributeAndModes( blend4, StateAttribute::ON | StateAttribute::OVERRIDE );

    // depthFunc to LEQUAL (LESS does not work!)
    Depth* depth4 = new Depth;
    depth4->setWriteMask( false );
    depth4->setFunction( Depth::LEQUAL );
    _ss4->setAttributeAndModes( depth4, StateAttribute::ON );

    // ambient intensity to zero
    // FIXME: what about other LightModel components?
    LightModel* lm4 = new LightModel;
    lm4->setAmbientIntensity( Vec4( 0., 0., 0., 0. ) );
    _ss4->setAttribute(lm4);

    Stencil *stencil4 = new Stencil;
    stencil4->setFunction( Stencil::EQUAL, 0, ~0u );
    stencil4->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::KEEP );
    _ss4->setAttributeAndModes( stencil4, StateAttribute::ON );
    
    /* 
     * Very important. Otherwise the ShadowTechnique::Traverse
     * will call init on every frame. The shaders files must be freed
     * at the end of program. Which currently isn't possible. 
     */
    _dirty = false;
}


void ShadowVolume::update( NodeVisitor& nv )
{
}


void ShadowVolume::cull( osgUtil::CullVisitor& cv )
{
   //notify(NOTICE)<<"CULL BEGIN"<<std::endl;
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
    if( _clearStencil)
       //_clearDrawable->setBufferMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
       cv.addDrawable( _clearDrawable, cv.getModelViewMatrix() );

    // pass 1: ambient pass
    if(!_ambientPassDisabled)
    {
       cv.pushStateSet( _ss1 );
       _shadowedScene->Group::traverse( cv );
       cv.popStateSet();
    }

    // get light positional state,
    // if no light, return
    Vec4 lightPos;
    Vec3 lightDir;
    if( !getLightPositionalState( cv, _light, lightPos, lightDir ) )
        return;

    /******************PASS 2,3/: Shadow geometry into stencil buffer*********************/
    
    if(   _mode == ShadowVolumeGeometryGenerator::CPU_FIND_GPU_EXTRUDE
       || _mode == ShadowVolumeGeometryGenerator::GPU_RAW
       || _mode == ShadowVolumeGeometryGenerator::GPU_SILHOUETTE)
    {    
       //notify(NOTICE)<<"LIGHTPOS: "<<lightPos.x()<<" "<<lightPos.y()<<" "<<lightPos.z()<<" "<<std::endl;
       _lightPosUniform->set(lightPos);
    }

    if(_updateStrategy == UPDATE_EACH_FRAME)
      _svgg.dirty();

   if(_svgg.isDirty()){
          //_svgg.clearGeometry();
          _svgg.setup(lightPos);
          _shadowedScene->Group::traverse( _svgg );
   }

   bool twoSidedStencil = _stencilImplementation == STENCIL_TWO_SIDED;
   if( _stencilImplementation == STENCIL_AUTO )
   {
      GraphicsContext *gc = cv.getState()->getGraphicsContext();
      if( gc->isGLExtensionSupported( 2.0, "GL20_separate_stencil" ) ||
          gc->isGLExtensionSupported( "GL_EXT_stencil_two_side" ) ||
          gc->isGLExtensionSupported( "GL_ATI_separate_stencil" ) )
      {
         twoSidedStencil = true;
      }
   }

   if(twoSidedStencil){
      cv.pushStateSet( _ss23 );

      ref_ptr< Drawable > d = _svgg.createGeometry();
      d->setUseDisplayList( false );
      ref_ptr< Geode > geode = new Geode;
      geode->addDrawable( d );
      geode->accept( cv );

      cv.popStateSet();

      if(_svgg.getMethod() == ShadowVolumeGeometryGenerator::ZFAIL){
         if(   _svgg.getMode() == ShadowVolumeGeometryGenerator::CPU_RAW 
             || _svgg.getMode() == ShadowVolumeGeometryGenerator::CPU_SILHOUETTE)
          {
            ref_ptr< Drawable > caps;
            caps = _svgg.getCapsGeometry();
            caps->setUseDisplayList( false );
            ref_ptr< Geode > geode_caps = new Geode;
            geode_caps->addDrawable( caps );

            cv.pushStateSet( _ss23_caps );
            geode_caps->accept( cv );
            cv.popStateSet();
         }
         else if(   _svgg.getMode() == ShadowVolumeGeometryGenerator::GPU_RAW 
                 || _svgg.getMode() == ShadowVolumeGeometryGenerator::GPU_SILHOUETTE)
         {
            cv.pushStateSet( _ss23_caps );
            geode->accept( cv );
            cv.popStateSet();
         }
      }
   }
   else{
       ref_ptr< Drawable > d = _svgg.createGeometry();
       d->setUseDisplayList( false );
       ref_ptr< Geode > geode = new Geode;
       geode->addDrawable( d );

       cv.pushStateSet( _ss2 );               
       geode->accept( cv );
       cv.popStateSet();

       // pass 3
       cv.pushStateSet( _ss3 );
       geode->accept( cv );
       cv.popStateSet();
   
       if(_svgg.getMethod() == ShadowVolumeGeometryGenerator::ZFAIL){          
          if(   _svgg.getMode() == ShadowVolumeGeometryGenerator::CPU_RAW 
             || _svgg.getMode() == ShadowVolumeGeometryGenerator::CPU_SILHOUETTE)
          {
             ref_ptr< Drawable > caps;
             ref_ptr< Geode > geode_caps = new Geode;
             caps = _svgg.getCapsGeometry();
             caps->setUseDisplayList( false );
             geode_caps->addDrawable( caps );

             cv.pushStateSet( _ss2_caps );               
             geode_caps->accept( cv );
             cv.popStateSet();

             cv.pushStateSet( _ss3_caps );               
             geode_caps->accept( cv );
             cv.popStateSet();
          }
          else if(   _svgg.getMode() == ShadowVolumeGeometryGenerator::GPU_RAW 
                  || _svgg.getMode() == ShadowVolumeGeometryGenerator::GPU_SILHOUETTE)
          {
             //_just_caps->set(1);
             cv.pushStateSet( _ss2_caps );
             geode->accept( cv );
             cv.popStateSet();

             cv.pushStateSet( _ss3_caps );
             geode->accept( cv );
             cv.popStateSet();
             //_just_caps->set(0);
          }
       }
               
   }

    //notify(NOTICE)<<"CULL END!!!"<<std::endl;
}


void ShadowVolume::cleanSceneGraph()
{
}


bool ShadowVolume::getLightPositionalState( osgUtil::CullVisitor& cv,
        const Light* light, Vec4& lightPos, Vec3& lightDir )
{
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

    if( !found )
        return false;

    // transform light to world space
    Matrix localToWorld = Matrix::inverse( *cv.getModelViewMatrix() );
    if( matrix ) localToWorld.preMult( *matrix );

    lightPos = light->getPosition();

    if( lightPos[3] == 0 )
        lightDir.set( -lightPos[0], -lightPos[1], -lightPos[2] );
    else
        lightDir = light->getDirection();

    lightPos = lightPos * localToWorld;
    lightDir = Matrix::transform3x3( lightDir, localToWorld );
    lightDir.normalize();

    return true;
}


void ShadowVolume::setMode(ShadowVolumeGeometryGenerator::Modes mode){
       if(_mode == mode) return;
       _mode_unif->set(mode);
       if(   mode == ShadowVolumeGeometryGenerator::CPU_FIND_GPU_EXTRUDE
          || mode == ShadowVolumeGeometryGenerator::GPU_RAW
          || mode == ShadowVolumeGeometryGenerator::GPU_SILHOUETTE)
       {  
          if(!( _mode == ShadowVolumeGeometryGenerator::CPU_FIND_GPU_EXTRUDE
             || _mode == ShadowVolumeGeometryGenerator::GPU_RAW
             || _mode == ShadowVolumeGeometryGenerator::GPU_SILHOUETTE) )
          {
             //notify(NOTICE)<<"adding shaders to statesets"<<std::endl;
             _ss2->setAttribute(_volumeShader);
             _ss3->setAttribute(_volumeShader);
             _ss23->setAttribute(_volumeShader);
             _ss2_caps->setAttribute(_volumeShader);
             _ss3_caps->setAttribute(_volumeShader);
             _ss23_caps->setAttribute(_volumeShader);
          }
       }

       if(   mode == ShadowVolumeGeometryGenerator::CPU_RAW
          || mode == ShadowVolumeGeometryGenerator::CPU_SILHOUETTE)
       {
          if(!(   _mode == ShadowVolumeGeometryGenerator::CPU_RAW
               || _mode == ShadowVolumeGeometryGenerator::CPU_SILHOUETTE) )
          {
             //notify(NOTICE)<<"removing shaders from statesets"<<std::endl;
             _ss2->removeAttribute(StateAttribute::PROGRAM);
             _ss3->removeAttribute(StateAttribute::PROGRAM);
             _ss23->removeAttribute(StateAttribute::PROGRAM);
             _ss2_caps->removeAttribute(StateAttribute::PROGRAM);
             _ss3_caps->removeAttribute(StateAttribute::PROGRAM);
             _ss23_caps->removeAttribute(StateAttribute::PROGRAM);
          }
       }
       _mode_unif->set(_mode);
       _mode = mode;
       _svgg.setMode(mode);
 }

void ShadowVolume::setMethod(ShadowVolumeGeometryGenerator::Methods met){
   if(_svgg.getMethod() == met) return;
   if(met == ShadowVolumeGeometryGenerator::ZPASS){
      //shader
      _volumeShader->setParameter( GL_GEOMETRY_VERTICES_OUT_EXT, 8 );

      // stencil function (pass 2)
      Stencil* stencil2 = new Stencil;
      stencil2->setFunction( Stencil::ALWAYS, 0, ~0u );
      stencil2->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::INCR_WRAP );
      _ss2->setAttributeAndModes( stencil2, StateAttribute::ON );

      CullFace *cullFace2 = new CullFace;
      cullFace2->setMode( CullFace::BACK );
      _ss2->setAttributeAndModes( cullFace2, StateAttribute::ON | StateAttribute::OVERRIDE );

      // stencil function (pass 3)
      Stencil* stencil3 = new Stencil;
      stencil3->setFunction( Stencil::ALWAYS, 0, ~0u );
      stencil3->setOperation( Stencil::KEEP, Stencil::KEEP, Stencil::DECR_WRAP );
      _ss3->setAttributeAndModes( stencil3, StateAttribute::ON );

      CullFace *cullFace3 = new CullFace;
      cullFace3->setMode( CullFace::FRONT );
      _ss3->setAttributeAndModes( cullFace3, StateAttribute::ON | StateAttribute::OVERRIDE );

      // stencil two-sided function
      StencilTwoSided *stencil23 = new StencilTwoSided;
      stencil23->setFunction(StencilTwoSided::FRONT, StencilTwoSided::ALWAYS, 0, ~0u);
      stencil23->setOperation(StencilTwoSided::FRONT, StencilTwoSided::KEEP, StencilTwoSided::KEEP, StencilTwoSided::INCR_WRAP);
      stencil23->setFunction(StencilTwoSided::BACK, StencilTwoSided::ALWAYS, 0, ~0u);
      stencil23->setOperation(StencilTwoSided::BACK, StencilTwoSided::KEEP, StencilTwoSided::KEEP, StencilTwoSided::DECR_WRAP);
      _ss23->setAttributeAndModes( stencil23, StateAttribute::ON );
   }
   else{ //ZFAIL
      //shader

      // stencil function (pass 2)
      Stencil* stencil2 = new Stencil;
      stencil2->setFunction( Stencil::ALWAYS, 0, ~0u );
      stencil2->setOperation( Stencil::KEEP, Stencil::INCR_WRAP, Stencil::KEEP );
      _ss2->setAttributeAndModes( stencil2, StateAttribute::ON );

      CullFace *cullFace2 = new CullFace;
      cullFace2->setMode( CullFace::BACK );
      _ss2->setAttributeAndModes( cullFace2, StateAttribute::ON | StateAttribute::OVERRIDE );

      // stencil function (pass 3)
      Stencil* stencil3 = new Stencil;
      stencil3->setFunction( Stencil::ALWAYS, 0, ~0u );
      stencil3->setOperation( Stencil::KEEP, Stencil::DECR_WRAP, Stencil::KEEP );
      _ss3->setAttributeAndModes( stencil3, StateAttribute::ON );

      CullFace *cullFace3 = new CullFace;
      cullFace3->setMode( CullFace::FRONT );
      _ss3->setAttributeAndModes( cullFace3, StateAttribute::ON | StateAttribute::OVERRIDE );

      // stencil two-sided function
      StencilTwoSided *stencil23 = new StencilTwoSided;
      stencil23->setFunction(StencilTwoSided::FRONT, StencilTwoSided::ALWAYS, 0, ~0u);
      stencil23->setOperation(StencilTwoSided::FRONT, StencilTwoSided::KEEP, StencilTwoSided::INCR_WRAP, StencilTwoSided::KEEP);
      stencil23->setFunction(StencilTwoSided::BACK, StencilTwoSided::ALWAYS, 0, ~0u);
      stencil23->setOperation(StencilTwoSided::BACK, StencilTwoSided::KEEP, StencilTwoSided::DECR_WRAP, StencilTwoSided::KEEP);
      _ss23->setAttributeAndModes( stencil23, StateAttribute::ON );
   }
   _svgg.setMethod(met);
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
   //camera->setProjectionMatrixAsFrustum( left, right, bottom, top, zNear, FLT_MAX );
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

//#include <osgDB/ReadFile>
//#include <osgViewer/Viewer>
//#include <osgViewer/ViewerEventHandlers>
//
//using namespace osg;
//using namespace osgViewer;
//
//
//class FindLightVisitor : public NodeVisitor
//{
//    typedef NodeVisitor inherited;
//public:
//
//    FindLightVisitor() :
//        NodeVisitor( NodeVisitor::TRAVERSE_ALL_CHILDREN ),
//        _light( NULL )
//    {
//    }
//
//    META_NodeVisitor( "", "FindLightVisitor" )
//
//    Light* getLight() const { return _light; }
//
//    virtual void reset()
//    {
//        inherited::reset();
//        _light = NULL;
//    }
//
//    virtual void apply( LightSource& ls )
//    {
//        Light *l = ls.getLight();
//        if( l && !_light ) {
//            _light = l;
//        }
//    }
//
//protected:
//    ref_ptr< Light > _light;
//};


//int main(int argc, char **argv)
//{
//
//   osg::ArgumentParser arguments(&argc,argv);
//   
//   arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
//   arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is the example application for Shadow Volumes technique.");
//   arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" filename");
//
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("F1","Set CPU_RAW mode (Default).");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("F2","Set CPU_SILHOUETTE mode.");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("F3","Set GPU_RAW mode.");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("t","Turn on two-sided stencil. Only if it is supported.");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("o","turn on one-sided stencil (Default).");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("p","Z-pass method (Default).");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("i","Z-fail method.");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("s","Turn on stats handler.");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("f","Toggle windowed/fullscreen mode.");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("Space","Returns camera to its home position.");
//   arguments.getApplicationUsage()->addKeyboardMouseBinding("Esc","Quit.");
//
//   unsigned int helpType = 0;
//   if ((helpType = arguments.readHelpType()))
//   {
//       arguments.getApplicationUsage()->write(std::cout, helpType);
//       return 1;
//   }
//   if (arguments.errors())
//   {
//     arguments.writeErrorMessages(std::cout);
//     return 1;
//   }
//   if (arguments.argc()<=1)
//   {
//     arguments.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);
//     return 1;
//    }
//   // load scene
//   osg::ref_ptr<osg::Node> model = osgDB::readNodeFiles(arguments);
//   //Node *model = osgDB::readNodeFile( "./models/cubemap-test.osg" );
//   //Node *model = osgDB::readNodeFile( "./models/PointLight_Cubes10x10.osg" );
//   //Node *model = osgDB::readNodeFile( "./models/PointLight_Cubes40x25.osg" );
//   //Node *model = osgDB::readNodeFile( "./models/treppeWithLights.osg" );
//   /*StateSet *set = model->getOrCreateStateSet();
//   set->setMode( GL_LIGHTING, StateAttribute::ON | StateAttribute::OVERRIDE );
//   set->setMode( GL_LIGHT0, StateAttribute::ON | StateAttribute::OVERRIDE );*/
//   //set->setMode( GL_LIGHT1, StateAttribute::ON | StateAttribute::OVERRIDE );
//
//   if( !model ) {
//      notify(FATAL) << "Can not load model." << std::endl;
//      return -1;
//   }
//
//   // find light
//   FindLightVisitor flv;
//   model->accept( flv );
//   if( !flv.getLight() )
//      notify(WARN) << "No light in the model. No shadows will be displayed." << std::endl;
//
//   // setup shadows
//   osgShadow::ShadowedScene *ss = new osgShadow::ShadowedScene;
//   ss->addChild( model );
//   osgShadow::ShadowVolume *sv = new osgShadow::ShadowVolume;
//   sv->setLight( flv.getLight() );
//   ss->setShadowTechnique( sv );
//
//   // setup keyboard handler
//   ref_ptr<SVKeyboardHandler> svkh = new SVKeyboardHandler(sv);
//
//
//   // setup viewer
//   ref_ptr<RealizeOperation> operation = new RealizeOperation;
//   Viewer viewer(arguments);
//   viewer.setRealizeOperation(operation);
//   viewer.addEventHandler(svkh);
//
//   arguments.reportRemainingOptionsAsUnrecognized();
//   if (arguments.errors())
//   {
//     arguments.writeErrorMessages(std::cout);
//     return 1;
//   }
//
//   // forces allocation of stencil buffer////
//   DisplaySettings::instance()->setMinimumNumStencilBits( 8 );
//   //ref_ptr<GraphicsContext::Traits> traits = new GraphicsContext::Traits;
//   //traits->vsync = false;
//   //ref_ptr<GraphicsContext> gc = GraphicsContext::createGraphicsContext(traits);
//   //viewer.getCamera()->setGraphicsContext(gc);
//   ///
//   viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded); //for keyboard implementation change isn't yet thread safe
//   ///
//   viewer.addEventHandler( new osgViewer::StatsHandler );
//   viewer.addEventHandler(new osgViewer::WindowSizeHandler);
//   viewer.setSceneData( ss );
//   //viewer.setSceneData( model );
//   double left, right, bottom, top, zNear, zFar;
//   viewer.getCamera()->getProjectionMatrixAsFrustum( left, right, bottom, top, zNear, zFar );
//   viewer.getCamera()->setProjectionMatrixAsFrustum( left, right, bottom, top, zNear, FLT_MAX );
//   viewer.getCamera()->setComputeNearFarMode( CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
//   viewer.getCamera()->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
//   viewer.realize();
//
//   viewer.run();
//
//   return 0;
//}
