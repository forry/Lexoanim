/**
 * @file
 * PerPixelLighting class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef PER_PIXEL_LIGHTING_H
#define PER_PIXEL_LIGHTING_H


#include <osg/NodeVisitor>
#include <osg/TexEnv>
#include <osgShadow/ShadowedScene>
#include <stack>

class RefNodePath : public osg::Object, public osg::NodePath
{
public:

   RefNodePath() : osg::Object( false ), osg::NodePath()  {}
   RefNodePath( const osg::NodePath& r ) : osg::Object( false ), osg::NodePath( r )  {}
   RefNodePath( const RefNodePath& r ) : osg::Object( r ), osg::NodePath( r )  {}

   virtual Object* cloneType() const  { return new RefNodePath(); }
   virtual Object* clone( const osg::CopyOp& ) const  { return new RefNodePath(*this); }
   virtual bool isSameKindAs( const Object* obj ) const  { return dynamic_cast< const RefNodePath* >( obj ) != NULL; }
   virtual const char* libraryName() const  { return "Lexolight"; }
   virtual const char* className() const  { return "RefNodePath"; }

protected:

   virtual ~RefNodePath()  {}

};

typedef std::list< osg::ref_ptr< RefNodePath > > RefNodePathList;


/**
 * The class for the per-pixel lighting of the scene.
 * It is used to generate the scene with shaders for the per-pixel lighting
 * from the standard OpenGL scene.
 */
class PerPixelLighting : public osg::Referenced
{
public:

   PerPixelLighting();
   virtual ~PerPixelLighting();

   enum ShadowTechnique { NO_SHADOWS, SHADOW_VOLUMES, SHADOW_MAPS, STANDARD_SHADOW_MAPS,
      MINIMAL_SHADOW_MAPS, LSP_SHADOW_MAP_VIEW_BOUNDS, LSP_SHADOW_MAP_CULL_BOUNDS, LSP_SHADOW_MAP_DRAW_BOUNDS };

   virtual void convert( osg::Node *scene, ShadowTechnique shadowTechnique = NO_SHADOWS );
   inline osg::Node* getScene() const { return newScene; }

   class ShaderGenerator : public osg::Referenced
   {
   public:
      struct VertexShaderParams {
         bool texCoords;
         int shadowMapTexUnit;
         VertexShaderParams( bool texCoords, int shadowMapTextureUnit );
         VertexShaderParams( const osg::StateSet *s, int shadowMapTextureUnit );
         inline bool operator<( const VertexShaderParams &other ) const;
      };
      static osg::Shader* createVertexShader( const VertexShaderParams& vsp );
      static inline osg::Shader* createVertexShader( const osg::StateSet *s, int shadowMapTextureUnit );
      osg::Shader* getVertexShader( const VertexShaderParams& vsp );
      inline osg::Shader* getVertexShader( const osg::StateSet *s, int shadowMapTextureUnit );

      struct FragmentShaderParams {
         std::string baseTextureUniform;
         osg::TexEnv::Mode baseTextureMode;
         std::vector< bool > lights;
         std::vector< std::string > shadowTextureUniforms;
         int shadowMapTexUnit;
         bool cubeShadowMap;
         bool appendGlobalAmbient;
         bool compatibilityParams;
         FragmentShaderParams( const std::string& baseTextureUniform,
                   osg::TexEnv::Mode baseTextureMode, const std::vector< bool >& lights,
                   int shadowMapTextureUnit, bool cubeShadowMap,
                   const std::vector< std::string >& shadowTextureUniforms,
                   bool appendGlobalAmbient, bool compatibilityParams );
         FragmentShaderParams( const osg::StateSet *s, int shadowMapTextureUnit, bool cubeShadowMap,
                               ShadowTechnique shadowTechnique, bool globalAmbient );
         bool operator<( const FragmentShaderParams& other ) const;
      };
      static osg::Shader* createFragmentShader( const FragmentShaderParams& fsp );
      static inline osg::Shader* createFragmentShader( const osg::StateSet *s, int shadowMapTextureUnit, bool cubeShadowMap,
                                                       ShadowTechnique shadowTechnique, bool globalAmbient );
      osg::Shader* getFragmentShader( const FragmentShaderParams& fsp );
      inline osg::Shader* getFragmentShader( const osg::StateSet *s, int shadowMapTextureUnit, bool cubeShadowMap,
                                             ShadowTechnique shadowTechnique, bool globalAmbient );

      static class osg::Program* createProgram( const VertexShaderParams& vsp,
            const FragmentShaderParams& fsp );
      static class osg::Program* createProgram( const osg::StateSet *s, int shadowMapTextureUnit, bool cubeShadowMap,
                                                ShadowTechnique shadowTechnique, bool globalAmbient );
      class osg::Program* getProgram( const VertexShaderParams& vsp, const FragmentShaderParams& fsp );
      class osg::Program* getProgram( const osg::StateSet *s, int shadowMapTextureUnit, bool cubeShadowMap,
                                      ShadowTechnique shadowTechnique, bool globalAmbient );

      static bool hasTexture2D( const osg::StateSet *s, unsigned int unit );
      static bool isShadowTexture( const osg::StateSet *s, unsigned int unit );
      static osg::TexEnv::Mode getTexEnv( const osg::StateSet *s, unsigned int unit );

   public:
   protected:
      enum LanguageVersion { COMPATIBILITY, VERSION_1_3 };
   private:
      std::map< VertexShaderParams, osg::ref_ptr< osg::Shader > > vertexShaders;
      std::map< FragmentShaderParams, osg::ref_ptr< osg::Shader > > fragmentShaders;

      struct ProgramShaders {
         const osg::Shader *vertexShader;
         const osg::Shader *fragmentShader;
         ProgramShaders( const osg::Shader *vs, const osg::Shader *fs ) :
               vertexShader( vs ), fragmentShader( fs )  {}
         inline bool operator<( const ProgramShaders& other ) const;
      };
      std::map< ProgramShaders, osg::ref_ptr< osg::Program > > shaderPrograms;

   };

   class CollectLightVisitor : public osg::NodeVisitor
   {
   public:
      CollectLightVisitor();

      virtual void reset();
      virtual void apply( osg::LightSource &lightSource );

      typedef std::map< const osg::LightSource*, RefNodePathList > LightSourceList;

      inline const LightSourceList& getLightSourceList() const;
      inline LightSourceList& getLightSourceList();
      int getNumLights() const;

   protected:
      LightSourceList lightSourceList;
   };

   class ConvertVisitor : public osg::NodeVisitor
   {
   public:

      ConvertVisitor();
      virtual ~ConvertVisitor();
      META_NodeVisitor( "", "PerPixelLighting::ConvertVisitor" )

      inline osg::Node* getScene() { return newScene; }

      void setShadowTechnique( ShadowTechnique shadowTechnique );
      ShadowTechnique getShadowTechnique() const { return shadowTechnique; }

      typedef std::map< const osg::Light*, osg::ref_ptr< osgShadow::ShadowedScene > > ShadowedScenes;
      inline ShadowedScenes& getShadowedScenes() { return shadowedScenes; }

      struct MultipassData {
         bool globalAmbient;
         int lightBaseIndex;
         int shadowMapTexUnit;
         osg::ref_ptr< const RefNodePath > activeLightSourcePath;
         osg::ref_ptr< const osg::Light > activeLight;
         osg::Light *newLight;
         bool newLightCubeShadowMap;
      };
      inline MultipassData& getMultipassData();
      inline const MultipassData& getMultipassData() const;
      inline bool getMultipass();
      inline void setMultipass( bool active );

      virtual class osg::Program* createShaderProgram( const osg::StateSet *s, int shadowMapTextureUnit,
                                                       bool cubeShadowMap, bool globalAmbient );

      virtual void apply( osg::Node &node );
      virtual void apply( osg::Geode &geode );
      virtual void apply( osg::Group &group );
      virtual void apply( osg::LightSource &lightSource );

   protected:

      osg::Object* processState( osg::StateSet *s, osg::Drawable *d = NULL );
      void unprocessState();
      osg::StateSet* adjustStateSet( const osg::StateSet *ss, bool removeLightAttribs = true,
                                     bool removeLightModes = true, bool adjustTransparency = true ) const;
      osg::StateSet* getCumulatedStateSet();
      int getLastStateSetPathIndex();
      osg::Node* cloneCurrentPath();
      osg::Node* cloneCurrentPathUpToParent();
      osg::Node* clonePathUpToIndex( int i );
      void purgeEmptyNodes( osg::Group &parent );

      osg::ref_ptr< ShaderGenerator > shaderGenerator;
      virtual void recreateShaderGenerator();

      typedef std::list< osg::ref_ptr< osg::StateSet > > StateStack;
      StateStack stateStack;
      typedef std::list< osg::ref_ptr< osg::Node > > CloneStack;
      CloneStack cloneStack;

      osg::ref_ptr< osg::Node > newScene;
      ShadowTechnique shadowTechnique;
      ShadowedScenes shadowedScenes;

      // multipass
      bool multipassActive;
      MultipassData mpData;
   };

protected:

   osg::ref_ptr< osg::Node > newScene;
   virtual ConvertVisitor* createConvertVisitor() const;
   virtual CollectLightVisitor* createCollectLightVisitor() const;
};


//
//  inline function implementations
//

inline osg::Shader* PerPixelLighting::ShaderGenerator::createVertexShader(
          const osg::StateSet *s, int shadowMapTextureUnit )
{
   return createVertexShader(
      PerPixelLighting::ShaderGenerator::VertexShaderParams( s, shadowMapTextureUnit ) );
}

inline osg::Shader* PerPixelLighting::ShaderGenerator::getVertexShader(
          const osg::StateSet *s, int shadowMapTextureUnit )
{
   return getVertexShader(
      PerPixelLighting::ShaderGenerator::VertexShaderParams( s, shadowMapTextureUnit ) );
}

inline osg::Shader* PerPixelLighting::ShaderGenerator::createFragmentShader( const osg::StateSet *s,
          int shadowMapTextureUnit, bool cubeShadowMap, ShadowTechnique shadowTechnique, bool globalAmbient )
{
   return createFragmentShader(
      PerPixelLighting::ShaderGenerator::FragmentShaderParams( s, shadowMapTextureUnit, cubeShadowMap,
                                                               shadowTechnique, globalAmbient ) );
}

inline osg::Shader* PerPixelLighting::ShaderGenerator::getFragmentShader( const osg::StateSet *s,
          int shadowMapTextureUnit, bool cubeShadowMap, ShadowTechnique shadowTechnique, bool globalAmbient )
{
   return getFragmentShader(
      PerPixelLighting::ShaderGenerator::FragmentShaderParams( s, shadowMapTextureUnit, cubeShadowMap,
                                                               shadowTechnique, globalAmbient ) );
}

inline bool PerPixelLighting::ShaderGenerator::VertexShaderParams::operator<(
          const VertexShaderParams &other ) const
{
   if( this->texCoords < other.texCoords )
      return true;
   if( this->texCoords > other.texCoords )
      return false;
   return this->shadowMapTexUnit < other.shadowMapTexUnit;
}

inline bool PerPixelLighting::ShaderGenerator::ProgramShaders::operator<(
          const ProgramShaders& other ) const
{
   if( this->fragmentShader < other.fragmentShader)
      return true;
   if( this->fragmentShader > other.fragmentShader)
      return false;
   return this->vertexShader < other.vertexShader;
}

inline PerPixelLighting::ConvertVisitor::MultipassData&
       PerPixelLighting::ConvertVisitor::getMultipassData()
{ return mpData; }

inline const PerPixelLighting::ConvertVisitor::MultipassData&
       PerPixelLighting::ConvertVisitor::getMultipassData() const
{ return mpData; }

inline bool PerPixelLighting::ConvertVisitor::getMultipass()
{ return multipassActive; }

inline void PerPixelLighting::ConvertVisitor::setMultipass( bool active )
{ multipassActive = active; }

inline const PerPixelLighting::CollectLightVisitor::LightSourceList&
       PerPixelLighting::CollectLightVisitor::getLightSourceList() const
{ return lightSourceList; }

inline PerPixelLighting::CollectLightVisitor::LightSourceList&
       PerPixelLighting::CollectLightVisitor::getLightSourceList()
{ return lightSourceList; }


#endif /* PER_PIXEL_LIGHTING_H */
