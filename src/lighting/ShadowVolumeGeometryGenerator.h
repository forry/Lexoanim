/**
 * @file
 * ShadowVolumeGeometryGenerator class.
 *
 * @author PCJohn (Jan Peèiva), Foreigner (Tomas Starka)
 */
#ifndef _SVGG
#define _SVGG

#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Stencil>
#include <osgShadow/ShadowedScene>
#include <osg/Drawable>
#include <osg/Referenced>
#include <osg/FrontFace>
#include <osg/CullFace>

//debug
#include <osg/io_utils>
//#include "EdgesObserver.h"


using namespace osg;

namespace osgShadow{

class ShadowVolumeGeometryGenerator : public NodeVisitor
{
public:
   /**
    * Edge struct
    * For now we assumes only two triangles for one edge.
    */
   struct Edge;
   struct TriangleOnlyCollector;
   class  TriangleOnlyCollectorFunctor;
   struct IndexVec4PtrPair;
protected:
   /* TYPEDEFS - not all of them*/
    typedef std::vector<Matrix> MatrixStack;
    typedef std::vector<StateAttribute::GLModeValue> ModeStack;
    typedef std::vector<osg::Vec4> Vec4List;
    typedef std::vector<osg::Vec3> Vec3List;
    typedef std::vector<GLuint> UIntList;
    typedef std::set<Edge> EdgeSet;
    typedef std::vector<Edge> EdgeList;
    typedef std::vector<std::vector<unsigned int>> points_edges;

public:

   enum Modes{
      CPU_RAW              = 1, // CPU extrude shadow volume from each triangle
      CPU_SILHOUETTE       = 2, // CPU computes silhouette and then extrude shadow volume
      GPU_RAW              = 3, // GPU extrude shadow volume from each triangle using geometry shader
      GPU_SILHOUETTE       = 4, // GPU computes silhouette and then extrude shadow volume using adjacency information
      CPU_FIND_GPU_EXTRUDE = 5, // CPU finds a silhouette but GPU extrude shadow volume using geometry shader
      SILHOUETTES_ONLY     = 6
   };

   enum Methods{
      ZPASS = 1,
      ZFAIL = 2
   };

   /**
    * The orientation of the faces that will cast a shadow. It may seem, from constants,
    * that the orientation will be culled, but that's just a coincidence. AUTO means
    * that the value will be taken from statesets.
    */
   enum ShadowCastingFace {
      FRONT = CullFace::FRONT, BACK = CullFace::BACK, FRONT_AND_BACK = CullFace::FRONT_AND_BACK, CF_AUTO
   };

   /**
    * The winding we sould consider as front-facing. Auto means that the value will be
    * taken from statesets.
    */
   enum FaceOrdering {
      CW = FrontFace::CLOCKWISE, CCW = FrontFace::COUNTER_CLOCKWISE, FO_AUTO
   };

   META_NodeVisitor( "osgShadow", "ShadowVolumeGeometryGenerator" )

   /**
    * Default constructor.
    */
   ShadowVolumeGeometryGenerator();

   /**
    * Constructor. Do the same thing as setup().
    * @param lightPos   Specify the light for shadow computation
    * @param matrix     Specify the matrix for light
    */
   ShadowVolumeGeometryGenerator( const Vec4& lightPos, Matrix* matrix = NULL );
   
   /**
    * Destructor. Releases reference of vectors used for storing information
    * about Shadow Volumes.
    */
   ~ShadowVolumeGeometryGenerator();

   /**
    * Setup light for shadow volume computation.
    */
   virtual void setup(const Vec4& lightPos, Matrix* matrix = NULL );

   /**
    * Create (if needed) and pass the shadow volumes geometry.
    */
   virtual ref_ptr<Geometry> createGeometry();

   /**
    * Returns shadow caps geometry. Both light and dark caps.
    */
   inline virtual ref_ptr<Geometry> getCapsGeometry();

   virtual void apply( Node& node );

   virtual void apply( Transform& transform );

   virtual void apply( Geode& geode );

   virtual void apply( Drawable* drawable );


   virtual void pushState( StateSet* stateset );

   inline virtual void popState();

   inline virtual void pushMatrix(Matrix& matrix);

   inline virtual void popMatrix();

   /**
    * For Debuging purposes. Prints number of references to some vectors.
    */
   void printRefs(){
      notify(NOTICE)<<"references :"<<std::endl
               <<"_edges_geo: "<<_edges_geo->referenceCount()<<std::endl
               <<"_coords: "<<_coords->referenceCount()<<std::endl
               <<"_edge_vert: "<<_edge_vert->referenceCount()<<std::endl
               <<"_normals: "<<_normals->referenceCount()<<std::endl
               <<"_edge_col: "<<_edge_col->referenceCount()<<std::endl;

   }

   /**
    * Sets the _mode variable. All important changes are made in ShadowVolume class.
    * This method should be called only from there.
    */
   virtual void setMode(Modes mode);

   /**
    * Returns the mode of Shadow Volume creation.
    */
   inline virtual int getMode();

   /**
    * Sets new method for Shadow Volume computing if needed. All important changes
    * are made in ShadowVolume class. This method should be called only from there.
    * If new method is set, it invalidates geometry.
    *
    * @see dirty()
    */
   virtual void setMethod(Methods met);

   /**
    * Returns current method of Shadow Volumes computation.
    */
   inline virtual int getMethod();

   /**
    * If called with true, clears geometry information and sets dirty flag, so
    * createGeometry method will recalculate.
    * 
    * @param d true if invalidate.
    */
   virtual void dirty( bool d = true);

   /**
    * Returns true if the geometry of Shadow Volumes is not valid. Beware
    * that all vectors are probably empty. For recalculationg geometry call
    * createGeometry method.
    *
    * @see createGeometry()
    */
   inline virtual bool isDirty();

   /**
    * Clears geometry info. This is also called from dirty() method.
    *
    * @see disrty()
    */
   virtual void clearGeometry();
   
   /**
    * Set/get facing of triangles desired for shadow casting. Default is 
    * CF_AUTO. That means it gets it's value from statesets.
    */
   inline virtual void setShadowCastingFace( ShadowCastingFace shadowCastingFace )
   {
      _shadowCastingFace = shadowCastingFace;
      _currentShadowCastingFace = shadowCastingFace;
   }
   inline ShadowCastingFace getShadowCastingFace() const {return _shadowCastingFace;}

   /**
    * Set/get face ordering of geometry used for shadow casting. Default is 
    * FO_AUTO. That means it gets it's value from statesets.
    */
   inline virtual void setFaceOrdering( FaceOrdering faceOrdering )
   {
      _faceOredering = faceOrdering;
      _currentFaceOredering = faceOrdering;
   }
   inline FaceOrdering getFaceOrdering() const {return _faceOredering;}

protected:


   //from occluderGeometry
   /**
    * This method breaks previously collected vector of vertices into a
    * indexed vector. After this there are never more than one vertex with
    * the same coordinates. And
    * the triangle geometry is maintained by indexMap a.k.a. 
    * _triangleIndices vector. After this method every 3*i, 3*i+1, 3*i+2
    * vertices forms a triangle, where i is integer and 3*i is index into
    * indexMap.
    */
   virtual void removeDuplicateVertices(UIntList& indexMap);

   /**
    * Computes normal of vertices. Call removeDuplicateVertices() first.
    *
    * @see removeDuplicateVertices(UIntList& indexMap)
    */
   virtual void computeNormals();

   /**
    * Builds edge map for collected geometry. Need to remove duplicate
    * vertices and compute normals first.
    */
   virtual void buildEdgeMap(UIntList &indexMap);

     
   /**
    * Compute silhouette in respect to given light position. Need the
    * edge map first.
    */
   virtual void computeSilhouette();

   /**
    * Decide whether is the edge silhouet of given point light.
    */
   virtual bool isLightPointSilhouetteEdge(const osg::Vec4& lightpos, const Edge& edge) const;

   /**
    * Decide whether is the edge silhouet of given directional light.
    */
   virtual bool isLightDirectSilhouetteEdge(const osg::Vec4& lightpos, const Edge& edge) const;

   virtual bool isLightSilhouetteEdge(const osg::Vec4& lightpos, const Edge& edge) const;

   virtual void setCurrentFacingAndOrdering(StateSet * ss);

   /**
    * Computes a point in infinity projected from light. Used for both
    * positional and directional lights.
    */
   static Vec4 projectToInf(Vec4 point, Vec4 light);


    bool                     _dirty;
    MatrixStack              _matrixStack;
    ModeStack                _blendModeStack;

    Modes                    _mode;
    Methods                  _method;
    ShadowCastingFace        _shadowCastingFace;
    FaceOrdering             _faceOredering;

    ShadowCastingFace        _currentShadowCastingFace;
    FaceOrdering             _currentFaceOredering;

    UIntList                 _triangleIndices;
    Vec3List                 _triangleNormals;
    Vec4                     _lightPos;

    ref_ptr<Vec4Array>       _coords;
    ref_ptr<Vec3Array>       _normals;
    ref_ptr<Geometry>        _edges_geo;
    ref_ptr<Vec4Array>       _edge_vert;
    ref_ptr<Vec4Array>       _edge_col;

    ref_ptr<Geometry>        _caps_geo;
    ref_ptr<Vec4Array>       _caps_vert;
    ref_ptr<Vec4Array>       _caps_col;

    points_edges             _points_edge;
    EdgeList                 _edgeList;

    UIntList                 _silhouetteIndices; //indices of vertices of possible silhouette
    EdgeSet _edgeSet;


};

}//namespace osgShadow
#endif //ndef _SVGG
