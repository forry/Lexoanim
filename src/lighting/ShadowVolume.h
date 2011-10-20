/**
 * @file
 * ShadowVolume class header.
 *
 * @author PCJohn (Jan Pečiva), Foreigner (Tomas Starka)
 */

#ifndef SHADOW_VOLUME_H
#define SHADOW_VOLUME_H

#include <osgShadow/ShadowTechnique>
#include "ShadowVolumeGeometryGenerator.h"

namespace osg {
   class CullFace;
   class StencilTwoSided;
   class ClearGLBuffersDrawable;
};

namespace osgViewer {
   class View;
};

class FindLightVisitor;

namespace osgShadow {


class ShadowVolume : public ShadowTechnique
{
    typedef ShadowTechnique inherited;

public:

   /*enum Exts{
      STENCIL_TWO_SIDED = 1 << 0,
   };*/

   enum StencilImplementation {
        STENCIL_ONE_SIDED, STENCIL_TWO_SIDED, STENCIL_AUTO
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

    virtual void setLight( osg::Light* light );
    inline osg::Light* getLight();
    inline const osg::Light* getLight() const;

    //void switch23Mask(){
    //   //ColorMask *cm = dynamic_cast<ColorMask *> (_ss23->getAttribute(StateAttribute::COLORMASK));
    //   //notify(NOTICE)<<"MASK "<</*cm<<" "<<cm->getRedMask()<<*/std::endl;
    //   ///*if(cm)
    //   //  cm->setMask(!(cm->getRedMask()), !(cm->getGreenMask()), !(cm->getBlueMask()), !(cm->getAlphaMask()));*/
    //}

    virtual void setMode(ShadowVolumeGeometryGenerator::Modes mode);
    inline virtual int getMode(){ return _mode;}

    virtual void setMethod(ShadowVolumeGeometryGenerator::Methods met);
    inline virtual int getMethod(){ return _svgg.getMethod();}

    /*inline virtual int isExt(int ext){ return _exts & ext;}
    inline virtual void setExt(int ext, int state = 1){
       if(state)
         _exts |= ext; 
       else
         _exts &= ~ext;
    }*/

    inline virtual void disableAmbientPass( bool value ){_ambientPassDisabled = value;}
    inline bool isAmbientPassDisabled() const;

    inline virtual void setStencilImplementation( StencilImplementation implementation ){_stencilImplementation = implementation;}
    inline StencilImplementation getStencilImplementation() const { return _stencilImplementation;}

    inline virtual void setUpdateStrategy( UpdateStrategy strategy ){_updateStrategy = strategy;}
    inline UpdateStrategy getUpdateStrategy() const {return _updateStrategy;}

    inline virtual void setShadowCastingFace( ShadowVolumeGeometryGenerator::ShadowCastingFace shadowCastingFace ){_svgg.setShadowCastingFace(shadowCastingFace);}
    inline ShadowVolumeGeometryGenerator::ShadowCastingFace getShadowCastingFace() const {return _svgg.getShadowCastingFace();}

    inline virtual void setFaceOrdering( ShadowVolumeGeometryGenerator::FaceOrdering faceOrdering ){_svgg.setFaceOrdering(faceOrdering);}
    inline ShadowVolumeGeometryGenerator::FaceOrdering getFaceOrdering() const {return _svgg.getFaceOrdering();}


    inline virtual void setClearStencil( bool value ){ _clearStencil = value;}
    inline virtual bool getClearStencil() const { return _clearStencil;}



protected:


    osg::ref_ptr< osg::Light >    _light;
    osg::ref_ptr< osg::StateSet > _ss1;
    osg::ref_ptr< osg::StateSet > _ss2;
    osg::ref_ptr< osg::StateSet > _ss3;
    osg::ref_ptr< osg::StateSet > _ss23;
    osg::ref_ptr< osg::StateSet > _ss2_caps;
    osg::ref_ptr< osg::StateSet > _ss3_caps;
    osg::ref_ptr< osg::StateSet > _ss23_caps;
    osg::ref_ptr< osg::StateSet > _ss4;
    osg::ref_ptr< osg::StateSet > _ssd;

    ///modes of SV computing
    ShadowVolumeGeometryGenerator::Modes   _mode;
    StencilImplementation                  _stencilImplementation;
    int                                    _exts; //bit mask for extension control
    bool                                   _ambientPassDisabled;
    bool                                   _clearStencil;
    UpdateStrategy                         _updateStrategy;

    bool _initialized;

    ///SV geometry generator - for creating it only once
    ShadowVolumeGeometryGenerator _svgg;
    ref_ptr<ClearGLBuffersDrawable> _clearDrawable;
    //bool _occluders_dirty;

    ///shaders - so we compile them only once
    osg::ref_ptr<osg::Program> _volumeShader;
    osg::ref_ptr<osg::Uniform> _lightPosUniform; //world coords light position for shaders
    osg::ref_ptr<osg::Uniform> _mode_unif; //implementation mode for shader use
    osg::ref_ptr<osg::Uniform> _just_sides; //method (PASS or FAIL) for shader use
    osg::ref_ptr<osg::Uniform> _just_caps; //render/compute shadow caps only

    static bool getLightPositionalState( osgUtil::CullVisitor& cv,
        const osg::Light* light, osg::Vec4& lightPos, osg::Vec3& lightDir );
};


// inline methods
osg::Light* ShadowVolume::getLight() { return _light.get(); }
const osg::Light* ShadowVolume::getLight() const { return _light.get(); }

}


#endif /* SHADOW_VOLUME_H */
