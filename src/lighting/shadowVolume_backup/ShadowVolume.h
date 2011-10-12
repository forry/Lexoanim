/**
 * @file
 * ShadowVolume class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef SHADOW_VOLUME_H
#define SHADOW_VOLUME_H

#include <osgShadow/ShadowTechnique>

namespace osg {
   class CullFace;
   class Stencil;
   class StencilTwoSided;
};
namespace osgViewer {
   class View;
};

namespace osgShadow {


/**
 *  Shadow volumes is widely used shadow technique utilizing stencil buffer
 *  to produce per-pixel accurate shadows. The per-pixel accurancy
 *  is usually the main benefit over shadow mapping techniques.
 *
 *  In general, shadow mapping techniques are considered faster
 *  while shadow volumes provides higher quality.
 *
 *  The current implementation has the following limitations (some are given by the current OSG limitations):
 *  - if shadow casting face is set to FRONT or BACK, it is expected that glFrontFace is set to CCW,
 *    otherwise FRONT and BACK may be swapped.
 *  - geometry rendered using blending and alpha test: There were attempt to disable shadows for
 *    semi-transparent geometry but the implementation is not complete, partially caused by OSG limitations.
 *  - all components of osg::LightModel are reset is OpenGL state by ShadowVolume class as
 *    OSG does not allow to set global ambient intensity only.
 *  - dots in the shadows appear from time to time. These are given by projecting one shadow volume from
 *    each triangle. Shared triangle edges may produce dots in shadows in some circumstancies.
 *    These may be eliminated by using silhouettes.
 *  - geometry shader (if used) generates shadow volumes for both, front and back triangles.
 *    One side only generation is not implemented yet, as the geometry shader approach seems too slow.
 *    In numbers: about two times slower than on CPU on GF-260 and i920@2.66GHz, 100 cube model, ~1200 triangles.
 *    On R5750, i920@2.66GHz, it is about four times slower.
 */
class ShadowVolume : public ShadowTechnique
{
    typedef ShadowTechnique inherited;

public:

    enum Method {
        ZPASS, ZFAIL
    };

    enum RenderingImplementation {
        CPU_TRIANGLE_SHADOW, GEOMETRY_SHADER_TRIANGLE_SHADOW
    };

    enum StencilImplementation {
        STENCIL_ONE_SIDED, STENCIL_TWO_SIDED, STENCIL_AUTO
    };

    enum ShadowCastingFace {
        FRONT = GL_FRONT, BACK = GL_BACK, FRONT_AND_BACK = GL_FRONT_AND_BACK
    };

    enum UpdateStrategy {
        UPDATE_EACH_FRAME, MANUAL_INVALIDATE
    };


    ShadowVolume();
    virtual ~ShadowVolume();

    static void setup( osgViewer::View *view );
    static void setupCamera( osg::Camera *camera );
    static void setupDisplaySettings( osg::DisplaySettings *ds );
    static void setupDisplaySettings( osgViewer::View *view );

    virtual void init();
    virtual void update( osg::NodeVisitor& nv );
    virtual void cull( osgUtil::CullVisitor& cv );
    virtual void cleanSceneGraph();

    virtual void setMethod( Method method );
    inline Method getMethod() const;

    virtual void setRenderingImplementation( RenderingImplementation implementation );
    inline RenderingImplementation getRenderingImplementation() const;

    virtual void setStencilImplementation( StencilImplementation implementation );
    inline StencilImplementation getStencilImplementation() const;

    virtual void updateShadowData( const osg::Vec4 &lightPos );
    virtual void invalidateShadowData();
    virtual void setUpdateStrategy( UpdateStrategy strategy );
    inline UpdateStrategy getUpdateStrategy() const;

    virtual void setLight( osg::Light* light );
    inline osg::Light* getLight();
    inline const osg::Light* getLight() const;

    virtual void disableAmbientPass( bool value );
    inline bool isAmbientPassDisabled() const;

    virtual void setClearStencil( bool value );
    inline bool getClearStencil() const;

    virtual void setShadowCastingFace( ShadowCastingFace shadowCastingFace );
    inline ShadowCastingFace getShadowCastingFace() const;

protected:

    osg::ref_ptr< osg::Light >    _light;
    osg::ref_ptr< osg::StateSet > _ss1;
    osg::ref_ptr< osg::StateSet > _ss2;
    osg::ref_ptr< osg::StateSet > _ss3;
    osg::ref_ptr< osg::StateSet > _ss2caps;
    osg::ref_ptr< osg::StateSet > _ss3caps;
    osg::ref_ptr< osg::StateSet > _ss23;
    osg::ref_ptr< osg::StateSet > _ss23caps;
    osg::ref_ptr< osg::StateSet > _ss4;
    osg::ref_ptr< osg::Program >  _volumeProgram;
    osg::ref_ptr< osg::Uniform >  _lightPosUniform;
    osg::ref_ptr< osg::Node >     _shadowGeometry;
    osg::ref_ptr< osg::Node >     _shadowCapsGeometry;
    osg::ref_ptr< osg::Drawable > _clearDrawable;
    osg::ref_ptr< osg::Stencil >  _stencil2;
    osg::ref_ptr< osg::Stencil >  _stencil3;
    osg::ref_ptr< osg::CullFace > _cullFace2;
    osg::ref_ptr< osg::CullFace > _cullFace3;
    osg::ref_ptr< osg::StencilTwoSided > _stencil23;

    Method            _method;
    RenderingImplementation _renderingImplementation;
    StencilImplementation   _stencilImplementation;
    UpdateStrategy    _updateStrategy;
    bool              _ambientPassDisabled;
    bool              _clearStencil;
    ShadowCastingFace _shadowCastingFace;

    static bool getLightPositionalState( osgUtil::CullVisitor& cv, const osg::Light* light,
        osg::Vec4& lightPos, osg::Vec3& lightDir, bool resultInLocalCoordinates = true );

    void updateStencilingAndCulling();
};


// inline methods
inline ShadowVolume::Method ShadowVolume::getMethod() const { return _method; }
inline ShadowVolume::RenderingImplementation ShadowVolume::getRenderingImplementation() const
{ return _renderingImplementation; }
inline ShadowVolume::StencilImplementation ShadowVolume::getStencilImplementation() const
{ return _stencilImplementation; }
inline ShadowVolume::UpdateStrategy ShadowVolume::getUpdateStrategy() const
{ return _updateStrategy; }
inline osg::Light* ShadowVolume::getLight() { return _light.get(); }
inline const osg::Light* ShadowVolume::getLight() const { return _light.get(); }
inline bool ShadowVolume::isAmbientPassDisabled() const { return _ambientPassDisabled; }
inline bool ShadowVolume::getClearStencil() const { return _clearStencil; }
inline ShadowVolume::ShadowCastingFace ShadowVolume::getShadowCastingFace() const
{ return _shadowCastingFace; }

}


#endif /* SHADOW_VOLUME_H */
