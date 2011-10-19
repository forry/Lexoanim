#include "ShadowVolumeGeometryGenerator.h"
#include <osg/TriangleFunctor>
#include <osg/StateAttribute>

using namespace osgShadow;


struct ShadowVolumeGeometryGenerator::TriangleOnlyCollector
{
   Vec4Array *_data;
   Vec4Array *_caps_vert;
   Vec4Array *_caps_col;
   Matrix *_matrix;
   Vec4 _lightPos;
   ShadowVolumeGeometryGenerator::Modes _mode;
   ShadowVolumeGeometryGenerator::Methods _method;
   unsigned int _fronface;
   unsigned int _cullface;

   static inline Vec3 toVec3( const Vec4& v4)
   {
      if( v4[3] == 1. || v4[3] == 0)
            return Vec3( v4[0], v4[1], v4[2] );

      Vec4::value_type n = 1. / v4[3];
      return Vec3( v4[0]*n, v4[1]*n, v4[2]*n );
   }

   void operator ()( const osg::Vec3& s1, const osg::Vec3& s2,
                           const osg::Vec3& s3, bool treatVertexDataAsTemporary )
   {

      Vec3 v1 = s1;
      Vec3 v2 = s2;
      Vec3 v3 = s3;
      if( _matrix ) {
            v1 = v1 * *_matrix;
            v2 = v2 * *_matrix;
            v3 = v3 * *_matrix;
      }
            
      if(_mode == ShadowVolumeGeometryGenerator::CPU_RAW || _mode == ShadowVolumeGeometryGenerator::CPU_SILHOUETTE){
         Vec3 n;
         if(_fronface == ShadowVolumeGeometryGenerator::CCW)
            n = (v2-v1) ^ (v3-v1);          // normal of the face
         else
            n = (v3-v1) ^ (v2-v1);          // normal of the face
         Vec3 lp3 = toVec3( _lightPos );       // lightpos converted to Vec3
         //notify(NOTICE)<<"Lightpos "<<lp3.x()<<" "<<lp3.y()<<" "<<lp3.z()<<" "<<std::endl;
         bool front = ( n * ( lp3-v1 * _lightPos.w() ) ) > 0; // dot product, when the face is parallel to light ( the normal is orthogonal) the face is considered back face. This helps with objects with holes (non-solid) so when we are computing silhouette in z-fail we are not making light caps out of it (causing problems with directional light).
         if(_cullface = ShadowVolumeGeometryGenerator::BACK)
            front = !front;
         if(_mode == ShadowVolumeGeometryGenerator::CPU_RAW && !front && _cullface == ShadowVolumeGeometryGenerator::FRONT_AND_BACK){
            Vec3 tmp = v1;
            v1 = v2;
            v2 = tmp;
            //return;
         }
         else if(front && _mode == ShadowVolumeGeometryGenerator::CPU_SILHOUETTE && _method == ShadowVolumeGeometryGenerator::ZFAIL && _caps_vert != NULL){
            //notify(NOTICE)<<"ADD CAP"<<std::endl;
            Vec4 t1(v1,1.0);
            Vec4 t2(v2,1.0);
            Vec4 t3(v3,1.0);
            _caps_vert->push_back(t1);
            _caps_vert->push_back(t2);
            _caps_vert->push_back(t3);

            _caps_col->push_back(Vec4(1.0,0.0,0.0,1.0));
            _caps_col->push_back(Vec4(1.0,0.0,0.0,1.0));
            _caps_col->push_back(Vec4(1.0,0.0,0.0,1.0));

            /* dark caps (lame) */
            Vec4 t0inf = projectToInf(t1, _lightPos);
            Vec4 t1inf = projectToInf(t2, _lightPos);
            Vec4 t2inf = projectToInf(t3, _lightPos);

            _caps_vert->push_back(t0inf);
            _caps_vert->push_back(t2inf);
            _caps_vert->push_back(t1inf);

            _caps_col->push_back(Vec4(0.0,0.0,1.0,1.0));
            _caps_col->push_back(Vec4(0.0,0.0,1.0,1.0));
            _caps_col->push_back(Vec4(0.0,0.0,1.0,1.0));
         }
      }

      // transform vertices
      Vec4 t1( v1,1.);
      Vec4 t2( v2,1.);
      Vec4 t3( v3,1.);
            
      // skip if not front-facing the light
      //Vec3 n = (t2-t1) ^ (t3-t1);          // normal of the face
      //Vec3 lp3 = toVec3( _lightPos );       // lightpos converted to Vec3
      //bool front = ( n * ( lp3-t1 ) ) >= 0; // dot product
      //if( !front )
      //   return;

      // final vertices
      /*Vec4 f1( t1, 1. );
      Vec4 f2( t2, 1. );
      Vec4 f3( t3, 1. );*/
      // store vertices

      _data->push_back( t1 );
      _data->push_back( t2 );
      _data->push_back( t3 );
   }
};

    
class ShadowVolumeGeometryGenerator::TriangleOnlyCollectorFunctor : public TriangleFunctor< ShadowVolumeGeometryGenerator::TriangleOnlyCollector >
{
public:
   TriangleOnlyCollectorFunctor( Vec4Array *data, Matrix *m, const Vec4& lightPos,
      unsigned int frontface, unsigned int cullface,
      ShadowVolumeGeometryGenerator::Modes mode = ShadowVolumeGeometryGenerator::CPU_SILHOUETTE,
      ShadowVolumeGeometryGenerator::Methods met = ShadowVolumeGeometryGenerator::ZPASS,
      Vec4Array *caps = NULL, Vec4Array *cols = NULL)
   {
      _data = data;
      _caps_vert = caps;
      _caps_col = cols;
      _matrix = m;
      _lightPos = lightPos;
      _mode = mode;
      _method = met;
      _fronface = frontface;
      _cullface = _cullface;
   }
};

struct ShadowVolumeGeometryGenerator::Edge
{
   Edge():
         _p1(0),
         _p2(0),
         _t1(-1),
         _t2(-1) {}

   Edge(unsigned int p1, unsigned int p2):
         _p1(p1),
         _p2(p2),
         _t1(-1),
         _t2(-1)
   {
         if (p1>p2)
         {
            // swap ordering so p1 is less than or equal to p2
            _p1 = p2;
            _p2 = p1;
         }
   }
         
   inline bool operator < (const Edge& rhs) const
   {
         if (_p1 < rhs._p1) return true;
         if (_p1 > rhs._p1) return false;
         return (_p2 < rhs._p2);
   }
         
   bool addTriangle(unsigned int tri) const
   {
         if (_t1<0)
         {
            _t1 = tri;
            return true;
         }
         else if (_t2<0)
         {
            _t2 = tri;
            return true;
         }
         // argg more than two triangles assigned
         return false;
   }
         
   bool boundaryEdge() const { return _t2<0; }
     
   /* indices to _data vector (vector of points) */
   unsigned int    _p1;
   unsigned int    _p2;
         
   /* triangle indices - index*3 (e.g. _t1*3) is first point of triangle */
   mutable int     _t1;
   mutable int     _t2;
         
   mutable osg::Vec3   _normal;
};

struct ShadowVolumeGeometryGenerator::IndexVec4PtrPair
{
      IndexVec4PtrPair():
         vec(0),
         index(0) {}

      IndexVec4PtrPair(const osg::Vec4* v, unsigned int i):
         vec(v),
         index(i) {}

      inline bool operator < (const IndexVec4PtrPair& rhs) const
      {
         return *vec < *rhs.vec;
      }

      inline bool operator == (const IndexVec4PtrPair& rhs) const
      {
         return *vec == *rhs.vec;

//        return (*vec - *rhs.vec).length2() < 1e-2;

      }

      const osg::Vec4* vec;
      unsigned int index;
};


ShadowVolumeGeometryGenerator::ShadowVolumeGeometryGenerator() :
      NodeVisitor( NodeVisitor::TRAVERSE_ACTIVE_CHILDREN ),
        _dirty(true),
        _coords( new Vec4Array ),
        _edges_geo(new Geometry),
        _edge_vert(new Vec4Array),
        _edge_col(new Vec4Array),
        _normals(new Vec3Array),
        _caps_geo(new Geometry),
        _caps_vert(new Vec4Array),
        _caps_col(new Vec4Array),
        _mode(CPU_RAW),
        _method(ZPASS),
        _shadowCastingFace(AUTO),
        _faceOredering(AUTO)
{}

ShadowVolumeGeometryGenerator::ShadowVolumeGeometryGenerator( const Vec4& lightPos, Matrix* matrix) :
        NodeVisitor( NodeVisitor::TRAVERSE_ACTIVE_CHILDREN ),
        _dirty(true),
        _coords( new Vec4Array ),
        _lightPos( lightPos ),
        _edges_geo(new Geometry),
        _edge_vert(new Vec4Array),
        _edge_col(new Vec4Array),
        _normals(new Vec3Array),
        _caps_geo(new Geometry),
        _caps_vert(new Vec4Array),
        _caps_col(new Vec4Array),
        _mode(CPU_RAW),
        _method(ZPASS),
        _shadowCastingFace(AUTO),
        _faceOredering(AUTO)
      
{
   if( matrix )
      pushMatrix( *matrix );
}

ShadowVolumeGeometryGenerator::~ShadowVolumeGeometryGenerator(){
   _edges_geo = NULL;
   _coords = NULL;
   _normals = NULL;
   _edge_vert = NULL;
   _edge_col = NULL;      
}

void ShadowVolumeGeometryGenerator::setup(const Vec4& lightPos, Matrix* matrix)
{
   if( matrix )
      pushMatrix( *matrix );
   _lightPos = lightPos;
}

ref_ptr<Geometry> ShadowVolumeGeometryGenerator::createGeometry()
{
   if(!_dirty){
      return _edges_geo;
   }

   /* else we must recompute all! */
   /* all arrays should be already empty here */

   if(_mode == CPU_RAW){
      /* FIX ME 
         * removing null triangles could be done here */

      for(Vec4Array::iterator it = _coords->begin(); it!=_coords->end();){
         Vec4 v0( *it++);
         Vec4 v1( *it++);
         Vec4 v2( *it++);


         Vec4 v0inf = projectToInf(v0, _lightPos);
         Vec4 v1inf = projectToInf(v1, _lightPos);
         Vec4 v2inf = projectToInf(v2, _lightPos);

         _edge_vert->push_back(v0);
         _edge_vert->push_back(v0inf);
         _edge_vert->push_back(v1inf);
         _edge_vert->push_back(v1);

         _edge_col->push_back(Vec4(1.0,0.0,0.0,0.1));
         _edge_col->push_back(Vec4(0.0,0.0,1.0,0.1));
         _edge_col->push_back(Vec4(0.0,0.0,1.0,0.1));
         _edge_col->push_back(Vec4(1.0,0.0,0.0,0.1));

         _edge_vert->push_back(v1);
         _edge_vert->push_back(v1inf);
         _edge_vert->push_back(v2inf);
         _edge_vert->push_back(v2);

         _edge_col->push_back(Vec4(1.0,0.0,0.0,0.1));
         _edge_col->push_back(Vec4(0.0,0.0,1.0,0.1));
         _edge_col->push_back(Vec4(0.0,0.0,1.0,0.1));
         _edge_col->push_back(Vec4(1.0,0.0,0.0,0.1));

         _edge_vert->push_back(v2);
         _edge_vert->push_back(v2inf);
         _edge_vert->push_back(v0inf);
         _edge_vert->push_back(v0);

         _edge_col->push_back(Vec4(1.0,0.0,0.0,0.1));
         _edge_col->push_back(Vec4(0.0,0.0,1.0,0.1));
         _edge_col->push_back(Vec4(0.0,0.0,1.0,0.1));
         _edge_col->push_back(Vec4(1.0,0.0,0.0,0.1));
            
         /* generate caps */
         if(_method == ZFAIL){
            /* light cap */
            _caps_vert->push_back(v0);
            _caps_vert->push_back(v1);
            _caps_vert->push_back(v2);

            _caps_col->push_back(Vec4(1.0,0.0,0.0,1.0));
            _caps_col->push_back(Vec4(1.0,0.0,0.0,1.0));
            _caps_col->push_back(Vec4(1.0,0.0,0.0,1.0));
               
            /* dark cap */
            _caps_vert->push_back(v0inf);
            _caps_vert->push_back(v2inf);
            _caps_vert->push_back(v1inf);

            _caps_col->push_back(Vec4(0.0,0.0,1.0,1.0));
            _caps_col->push_back(Vec4(0.0,0.0,1.0,1.0));
            _caps_col->push_back(Vec4(0.0,0.0,1.0,1.0));

         }         
      }

      _edges_geo->setVertexArray(_edge_vert);
      _edges_geo->setColorArray(_edge_col);
      _edges_geo->setColorBinding(Geometry::BIND_PER_VERTEX);
      _edges_geo->addPrimitiveSet( new DrawArrays( PrimitiveSet::QUADS, 0, _edge_vert->size() ) );

         
      //notify(NOTICE)<<"CAPS SIZE: "<<_caps_vert->size()<<std::endl;

      _caps_geo->setVertexArray(_caps_vert);
      _caps_geo->setColorArray(_caps_col);
      _caps_geo->setColorBinding(Geometry::BIND_PER_VERTEX);
      _caps_geo->addPrimitiveSet( new DrawArrays( PrimitiveSet::TRIANGLES, 0, _caps_vert->size() ) );


   }
   else if(_mode == CPU_SILHOUETTE){
      //notify(NOTICE)<<"vert count: "<<(*_coords.get()).size()<<std::endl;
      //notify(NOTICE)<<"vert count: "<<(*_coords).size()<<std::endl;
      removeDuplicateVertices(_triangleIndices);
      computeNormals();
      buildEdgeMap(_triangleIndices);
      computeSilhouette();
      
      int ii = 0;
      for(UIntList::iterator it = _silhouetteIndices.begin(); it != _silhouetteIndices.end(); ii+=2){
        
         Vec4 v0( (*_coords)[*it++]);
         Vec4 v1( (*_coords)[*it++]);
         Vec4 v0inf = projectToInf(v0, _lightPos);
         Vec4 v1inf = projectToInf(v1, _lightPos);

         /* all points should be in correct order now so let's construct the side of volume */
         _edge_vert->push_back(v1);
         _edge_vert->push_back(v0);
         _edge_col->push_back(Vec4(1.0,0.0,0.0,1.0));_edge_col->push_back(Vec4(1.0,0.0,0.0,1.0));

         _edge_vert->push_back(v0inf);
         _edge_vert->push_back(v1inf);        
         _edge_col->push_back(Vec4(0.0,0.0,1.0,1.0));
         _edge_col->push_back(Vec4(0.0,0.0,1.0,1.0));

      }

      _edges_geo->setVertexArray(_edge_vert);
      _edges_geo->setColorArray(_edge_col);
      _edges_geo->setColorBinding(Geometry::BIND_PER_VERTEX);
      _edges_geo->addPrimitiveSet( new DrawArrays( PrimitiveSet::QUADS, 0, _edge_vert->size()));

      //notify(NOTICE)<<"CAPS SIZE: "<<_caps_vert->size()<<std::endl;

      _caps_geo->setVertexArray(_caps_vert);
      _caps_geo->setColorArray(_caps_col);
      _caps_geo->setColorBinding(Geometry::BIND_PER_VERTEX);
      _caps_geo->addPrimitiveSet( new DrawArrays( PrimitiveSet::TRIANGLES, 0, _caps_vert->size()));
   }
   else if(_mode == SILHOUETTES_ONLY){ //for debugging purposses
      removeDuplicateVertices(_triangleIndices);
      computeNormals();
      buildEdgeMap(_triangleIndices);
      computeSilhouette();

      for(UIntList::iterator it = _silhouetteIndices.begin(); it != _silhouetteIndices.end(); ){

         Vec4 v0( (*_coords)[*it++]);
         Vec4 v1( (*_coords)[*it++]);
         
         /* all points should be in correct order now so let's construct the side of volume */
         _edge_vert->push_back(v1);
         _edge_vert->push_back(v0);
         _edge_col->push_back(Vec4(0.0,1.0,0.0,1.0));
         _edge_col->push_back(Vec4(1.0,0.0,0.0,1.0));
      }
      _edges_geo->setVertexArray(_edge_vert);
      _edges_geo->setColorArray(_edge_col);
      _edges_geo->setColorBinding(Geometry::BIND_PER_VERTEX);
      _edges_geo->addPrimitiveSet( new DrawArrays( PrimitiveSet::LINES, 0, _edge_vert->size()));
   }
   else if(_mode == GPU_RAW){
      _edges_geo->setVertexArray(_coords);
      //notify(NOTICE)<<"SIZE: "<<_coords->size()<<std::endl;
      //_edges_geo->setColorArray(_edge_col);
      //_edges_geo->setColorBinding(Geometry::BIND_PER_VERTEX);
      _edges_geo->addPrimitiveSet( new DrawArrays( PrimitiveSet::TRIANGLES, 0, _coords->size() ) );
   }
   else if(_mode == CPU_FIND_GPU_EXTRUDE){
      //notify(NOTICE)<<"vert count: "<<(*_coords.get()).size()<<std::endl;
      //notify(NOTICE)<<"vert count: "<<(*_coords).size()<<std::endl;
      removeDuplicateVertices(_triangleIndices);
      computeNormals();
      buildEdgeMap(_triangleIndices);
   }
   _dirty = false;
   //notify(NOTICE)<<"Returning new Geometry"<<std::endl;
   return _edges_geo;
}

ref_ptr<Geometry> ShadowVolumeGeometryGenerator::getCapsGeometry(){
   return _caps_geo;
}

void ShadowVolumeGeometryGenerator::apply( Node& node )
{
   StateSet *ss = node.getStateSet();
   if(ss)
      setCurrentFacingAndOrdering(ss);
   if( node.getStateSet() )
      pushState( node.getStateSet() );

   traverse( node );

   if(ss)
      setCurrentFacingAndOrdering(ss);
   if( node.getStateSet() )
      popState();
}

void ShadowVolumeGeometryGenerator::apply( Transform& transform )
{
   StateSet *ss = transform.getStateSet();
   if(ss)
      setCurrentFacingAndOrdering(ss);
   if( transform.getStateSet() )
      pushState( transform.getStateSet() );

   Matrix matrix;
   if( !_matrixStack.empty() )
      matrix = _matrixStack.back();

   transform.computeLocalToWorldMatrix( matrix, this );

   pushMatrix( matrix );

   traverse( transform );

   popMatrix();

   if(ss)
      setCurrentFacingAndOrdering(ss);
   if( transform.getStateSet() )
      popState();
}

void ShadowVolumeGeometryGenerator::apply( Geode& geode )
{
   StateSet *ss = geode.getStateSet();
   if(ss)
      setCurrentFacingAndOrdering(ss);

   if( geode.getStateSet() )
      pushState( geode.getStateSet() );

   for( unsigned int i=0; i<geode.getNumDrawables(); ++i )
   {
      Drawable* drawable = geode.getDrawable( i );

      if( drawable->getStateSet() )
            pushState( drawable->getStateSet() );

      apply( geode.getDrawable( i ) );

      if( drawable->getStateSet() )
            popState();
   }

   if(ss)
      setCurrentFacingAndOrdering(ss);
   if( geode.getStateSet() )
      popState();
}

void ShadowVolumeGeometryGenerator::apply( Drawable* drawable )
{
   StateSet *ss = drawable->getStateSet();
   if(ss)
      setCurrentFacingAndOrdering(ss);
   StateAttribute::GLModeValue blendModeValue;
   if( _blendModeStack.empty() ) blendModeValue = StateAttribute::INHERIT;
   else blendModeValue = _blendModeStack.back();
   //if( blendModeValue & StateAttribute::ON )
   //{
   //    // ignore transparent drawables
   //    return;
   //}
        
   if(_mode == CPU_RAW){
      TriangleOnlyCollectorFunctor tc( _coords, _matrixStack.empty() ? NULL : &_matrixStack.back(), _lightPos, _currentFaceOredering, _currentShadowCastingFace,  CPU_RAW );
      drawable->accept( tc );
   }
   else if(_mode == CPU_SILHOUETTE){
      TriangleOnlyCollectorFunctor tc( _coords, _matrixStack.empty() ? NULL : &_matrixStack.back(), _lightPos, _currentFaceOredering, _currentShadowCastingFace, CPU_SILHOUETTE ,_method , _caps_vert, _caps_col);
      drawable->accept( tc );
   }
   else if(_mode == GPU_RAW){
      TriangleOnlyCollectorFunctor tc( _coords, _matrixStack.empty() ? NULL : &_matrixStack.back(), _lightPos, _currentFaceOredering, _currentShadowCastingFace );
      drawable->accept( tc );
   }
   else if(_mode == CPU_FIND_GPU_EXTRUDE){
      TriangleOnlyCollectorFunctor tc( _coords, _matrixStack.empty() ? NULL : &_matrixStack.back(), _lightPos, _currentFaceOredering, _currentShadowCastingFace );
      drawable->accept( tc );
           
   }
   else if(_mode == SILHOUETTES_ONLY){
      TriangleOnlyCollectorFunctor tc( _coords, _matrixStack.empty() ? NULL : &_matrixStack.back(), _lightPos, _currentFaceOredering, _currentShadowCastingFace, CPU_SILHOUETTE ,_method , _caps_vert, _caps_col);
      drawable->accept( tc );
   }

}

void ShadowVolumeGeometryGenerator::pushState( StateSet* stateset )
{
   StateAttribute::GLModeValue prevBlendModeValue;
   if( _blendModeStack.empty() ) prevBlendModeValue = StateAttribute::INHERIT;
   else prevBlendModeValue = _blendModeStack.back();

   StateAttribute::GLModeValue newBlendModeValue = stateset->getMode( GL_BLEND );

   if( !(newBlendModeValue & StateAttribute::PROTECTED) &&
      (prevBlendModeValue & StateAttribute::OVERRIDE) )
   {
      newBlendModeValue = prevBlendModeValue;
   }

   _blendModeStack.push_back( newBlendModeValue );
}

void ShadowVolumeGeometryGenerator::popState()
{
   _blendModeStack.pop_back();
}

void ShadowVolumeGeometryGenerator::pushMatrix(Matrix& matrix)
{
   _matrixStack.push_back( matrix );
}

void ShadowVolumeGeometryGenerator::popMatrix()
{
   _matrixStack.pop_back();
}

void ShadowVolumeGeometryGenerator::setMode(Modes mode){
   if(_mode == mode) return;
   _mode = mode;
   dirty();
}

int ShadowVolumeGeometryGenerator::getMode(){
   return _mode;
}

void ShadowVolumeGeometryGenerator::setMethod(Methods met){
   if(_method == met) return;
   _method = met;
   // FIXME: it's wasting to throw computed geometry or creating it all over again
   dirty();
}

int ShadowVolumeGeometryGenerator::getMethod(){
   return _method;
}

void ShadowVolumeGeometryGenerator::dirty( bool d){
   _dirty = d;
   if(_dirty == true)
      clearGeometry();
}

bool ShadowVolumeGeometryGenerator::isDirty(){
   return _dirty;
}

void ShadowVolumeGeometryGenerator::clearGeometry(){
   _coords->clear();
   _edge_vert->clear();
   _edge_col->clear();
   _normals->clear();
   _edges_geo = new Geometry;

   _caps_vert->clear();
   _caps_col->clear();
   _caps_geo = new Geometry;
}

/**********************PROTECTED********************/

void ShadowVolumeGeometryGenerator::removeDuplicateVertices(UIntList& indexMap)
   {
      if (_coords->empty()) return;
       
      typedef std::vector<IndexVec4PtrPair> IndexVec4PtrPairs;
      IndexVec4PtrPairs indexVec4PtrPairs;
      indexVec4PtrPairs.reserve(_coords->size());

      unsigned int i = 0;
      for(Vec4List::iterator vitr = _coords->begin();
         vitr != _coords->end();
         ++vitr, ++i)
      {
         indexVec4PtrPairs.push_back(IndexVec4PtrPair(&(*vitr),i));
      }
      std::sort(indexVec4PtrPairs.begin(),indexVec4PtrPairs.end());;


      // compute size
      IndexVec4PtrPairs::iterator prev = indexVec4PtrPairs.begin();
      IndexVec4PtrPairs::iterator curr = prev;
      ++curr;

      unsigned int numDuplicates = 0;
      unsigned int numUnique = 1;

      for(; curr != indexVec4PtrPairs.end(); ++curr)
      {
         if (*prev==*curr)
         {
            ++numDuplicates;
         }
         else
         {
            prev = curr; 
            ++numUnique;
         }
      }

      //if (numDuplicates==0) return;

      // now assign the unique Vec4 to the newVertices arrays  
      if(!indexMap.empty()) indexMap.clear();
      indexMap.resize(indexVec4PtrPairs.size());

      Vec4List newVertices;
      newVertices.reserve(numUnique);
      unsigned int index = 0;

      prev = indexVec4PtrPairs.begin();
      curr = prev;

      // add first vertex
      indexMap[curr->index] = index;
      newVertices.push_back(*(curr->vec));

      ++curr;

      for(; curr != indexVec4PtrPairs.end(); ++curr)
      {
         if (*prev==*curr) 
         {
            indexMap[curr->index] = index;
         }
         else
         {
            ++index;            
               
            // add new unique vertex
            indexMap[curr->index] = index;
            newVertices.push_back(*(curr->vec));

            prev = curr; 
         }
      }

      // copy over need arrays and index values
      _coords->swap(newVertices);
}

void ShadowVolumeGeometryGenerator::computeNormals()
{
       
   unsigned int numTriangles = _triangleIndices.size() / 3;
   unsigned int redundentIndices = _triangleIndices.size() - numTriangles * 3;
   if (redundentIndices)
   {
      osg::notify(osg::NOTICE)<<"Warning OccluderGeometry::computeNormals() has found redundant trailing indices"<<std::endl;
      _triangleIndices.erase(_triangleIndices.begin() + numTriangles * 3, _triangleIndices.end());
   }
       
   _triangleNormals.clear(); //normals for each triangle (not each vertex)
   _triangleNormals.reserve(numTriangles);
       
   _normals->resize(_coords->size()); //normals for each vertex. Computed as a mean of surrounding normals


   for(UIntList::iterator titr = _triangleIndices.begin();
      titr != _triangleIndices.end();
      )
   {
      /* indices of triangle */
      GLuint p1 = *titr++;
      GLuint p2 = *titr++;
      GLuint p3 = *titr++;
      /* copying vertex coords from vec4 _coords to temporary vec3 items */
      Vec3 v1; v1.set((*_coords)[p1].x(), (*_coords)[p1].y(), (*_coords)[p1].z());       
      Vec3 v2; v2.set((*_coords)[p2].x(), (*_coords)[p2].y(), (*_coords)[p2].z());
      Vec3 v3; v3.set((*_coords)[p3].x(), (*_coords)[p3].y(), (*_coords)[p3].z());
      osg::Vec3 normal = (v2 - v1) ^ (v3 - v2);
      normal.normalize();

      _triangleNormals.push_back(normal); //real computed normal in object space!

      /* let this computed normal to contribute to all of triangle vertices */
      if (!_normals->empty())
      {        
         (*_normals)[p1] += normal;
         (*_normals)[p2] += normal;
         (*_normals)[p3] += normal;
      }
   }
       
   //normalize the vertex normal every time we add. So in the end its normalized and has right direction.
   for(Vec3List::iterator nitr = _normals->begin();
      nitr != _normals->end();
      ++nitr)
   {
      nitr->normalize();
   }
}

void ShadowVolumeGeometryGenerator::buildEdgeMap(UIntList &indexMap){
   if(!_edgeSet.empty()) _edgeSet.clear();
   unsigned int triNo = 0; //triangle number (index)
   unsigned int numTriangleErrors = 0;
   for(UIntList::iterator titr = indexMap.begin();
      titr != indexMap.end();
      ++triNo)
   {
      unsigned int p1 = *titr++;
      unsigned int p2 = *titr++;
      unsigned int p3 = *titr++;

      Edge edge12(p1,p2);
      EdgeSet::iterator it = _edgeSet.find(edge12); //find if the edge already exists in set
      if (it == _edgeSet.end()) { //if not then add triangle and add edge to the set
         if (!edge12.addTriangle(triNo)) ++numTriangleErrors;
         _edgeSet.insert(edge12);
      }
      else //if already exists, then just add the triangle
      {
            if (!it->addTriangle(triNo)) ++numTriangleErrors;
      }

      Edge edge23(p2,p3);
      it = _edgeSet.find(edge23);
      if (it == _edgeSet.end()) {
         if (!edge23.addTriangle(triNo)) ++numTriangleErrors;
         _edgeSet.insert(edge23);
      }
      else
      {
            if (!it->addTriangle(triNo)) ++numTriangleErrors;
      }

      Edge edge31(p3,p1);
      it = _edgeSet.find(edge31);
      if (it == _edgeSet.end()) {
         if (!edge31.addTriangle(triNo)) ++numTriangleErrors;
         _edgeSet.insert(edge31);
      }
      else
      {
            if (!it->addTriangle(triNo)) ++numTriangleErrors;
      }

   }
   if(numTriangleErrors > 0)
      notify(WARN)<<"Number of bad triangles: "<<numTriangleErrors<<std::endl;

   unsigned int numEdgesWithNoTriangles = 0;
   unsigned int numEdgesWithOneTriangles = 0;
   unsigned int numEdgesWithTwoTriangles = 0;

   /*
      Compute "normal" for each edge. This vector then helps us decide the order
      edge points in CW or CCW ordering of shadow volume sides.
   */
      
   if(!_edgeList.empty()) _edgeList.clear();
   if(!_points_edge.empty()) _points_edge.clear();
   _points_edge.reserve(_coords->size());
   for(unsigned int i=0; i<_coords->size(); i++){
      _points_edge.push_back( *(new std::vector<unsigned int>));
   }
   unsigned int curr_edge = 0;

   for(EdgeSet::iterator eitr = _edgeSet.begin(); eitr != _edgeSet.end(); ++eitr,++curr_edge)
   {
      const Edge& edge = *eitr;
      osg::Vec4 pos(0.0,0.0,0.0,1.0);
      osg::Vec4 mid = ((*_coords)[edge._p1] + (*_coords)[edge._p2]) * 0.5f;
      unsigned int numTriangles = 0;
      if (edge._t1>=0)
      {
         ++numTriangles;

         GLuint p1 = _triangleIndices[edge._t1*3];
         GLuint p2 = _triangleIndices[edge._t1*3+1];
         GLuint p3 = _triangleIndices[edge._t1*3+2];
         GLuint opposite = p1; //index of the vertex of _t1 triangle opposite to the current edge
         if (p1 != edge._p1 && p1 != edge._p2) opposite = p1;
         else if (p2 != edge._p1 && p2 != edge._p2) opposite = p2;
         else if (p3 != edge._p1 && p3 != edge._p2) opposite = p3;
         pos = (*_coords)[opposite]; //position of opposit vertex
      }
        
      if (edge._t2>=0)
      {
         ++numTriangles;

         GLuint p1 = _triangleIndices[edge._t2*3];
         GLuint p2 = _triangleIndices[edge._t2*3+1];
         GLuint p3 = _triangleIndices[edge._t2*3+2];
         GLuint opposite = p1;
         if (p1 != edge._p1 && p1 != edge._p2) opposite = p1;
         else if (p2 != edge._p1 && p2 != edge._p2) opposite = p2;
         else if (p3 != edge._p1 && p3 != edge._p2) opposite = p3;
         pos += (*_coords)[opposite];
      }

      switch(numTriangles)
      {
         case(0):
               ++numEdgesWithNoTriangles;
               edge._normal.set(0.0,0.0,0.0);
               osg::notify(osg::NOTICE)<<"Warning no triangles on edge."<<std::endl;
               break; 
         case(1):
               ++numEdgesWithOneTriangles;
               pos = pos - mid;
               //edge._normal = pos - mid;
               edge._normal.set(pos.x(), pos.y(),pos.z());
               edge._normal.normalize();
               break; 
         default:
               ++numEdgesWithTwoTriangles; 
               //edge._normal = (pos*0.5f) - mid;
               pos = (pos*0.5f) - mid;
               edge._normal.set(pos.x(), pos.y(),pos.z());
               edge._normal.normalize();
               break; 
      }
      _edgeList.push_back(*eitr);
      _points_edge[(*eitr)._p1].push_back(curr_edge);
      _points_edge[(*eitr)._p2].push_back(curr_edge);

   }
}

void ShadowVolumeGeometryGenerator::computeSilhouette(){
      _silhouetteIndices.clear();
      //notify(NOTICE)<<"computeSilhouette():"<<std::endl;
    
      for(EdgeSet::const_iterator eitr = _edgeSet.begin();
         eitr != _edgeSet.end();
         ++eitr)
      {
         const Edge& edge = *eitr;
         if (isLightSilhouetteEdge(_lightPos,edge))
         {
             
            Vec3 v1 = TriangleOnlyCollector::toVec3((*_coords)[edge._p1]); //v1.set((*_coords)[edge._p1].x(), (*_coords)[edge._p1].y(), (*_coords)[edge._p1].z());     
            Vec3 v2 = TriangleOnlyCollector::toVec3((*_coords)[edge._p2]); //v2.set((*_coords)[edge._p2].x(), (*_coords)[edge._p2].y(), (*_coords)[edge._p2].z());
            Vec3 lightpos = TriangleOnlyCollector::toVec3(_lightPos); //lightpos.set(_lightPos.x(), _lightPos.y(), _lightPos.z());
            osg::Vec3 normal = (v2-v1) ^ (v1 * _lightPos.w() - lightpos);
            float dir = normal * edge._normal;
            if (dir > 0.0)
            {        
                  _silhouetteIndices.push_back(edge._p1);
                  _silhouetteIndices.push_back(edge._p2);
            }
            //else if(dir == 0.0f) //silhouette when normal is orthogonal to edge normal means the light is parallel to edge normal and it is a boundary edge (of non-solid object)
            //{
            //   //we only take edge normal, this "normal" points into the object instead of out of it
            //   if( edge._normal
            //}
            else
            {
                  _silhouetteIndices.push_back(edge._p2);
                  _silhouetteIndices.push_back(edge._p1);
            }

         }
      }
      //notify(NOTICE)<<"~~END"<<std::endl;
}


bool ShadowVolumeGeometryGenerator::isLightSilhouetteEdge(const osg::Vec4& lightpos, const Edge& edge) const
{
   if (edge.boundaryEdge()) return true;
      
   float offset = 0.0;
   //float offset = -0.00001;
      
   osg::Vec4 delta(lightpos.x(),lightpos.y(),lightpos.z(),1.0);
   delta = (lightpos - (*_coords)[edge._p1] * lightpos.w());
   osg::Vec3 tolight = TriangleOnlyCollector::toVec3(delta);

   tolight.normalize();
      
   float n1 = tolight * _triangleNormals[edge._t1] + offset;
   float n2 = tolight * _triangleNormals[edge._t2] + offset;

   if (n1==0.0f && n2==0.0f){
      return false;
   }

   //if one fase is parallel with light dir and the other is facing backwards - do not generate silhoute edge
   /*if(n1 == 0.0f || n2 == 0.0f){
      if(n1 < 0.0f || n2 < 0.0f){
         return false;
      }
   }*/
      
   return n1*n2 <= 0.0f; 
}

bool ShadowVolumeGeometryGenerator::isLightPointSilhouetteEdge(const osg::Vec4& lightpos, const Edge& edge) const
{
   if (edge.boundaryEdge()) return true;
      
   float offset = 0.0f;

      
   osg::Vec4 delta(lightpos.x(),lightpos.y(),lightpos.z(),1.0);
   delta = (lightpos - (*_coords)[edge._p1]);
   delta.normalize();
      
   float n1 = delta * _triangleNormals[edge._t1] + offset;
   float n2 = delta * _triangleNormals[edge._t2] + offset;

   if (n1==0.0f && n2==0.0f) return false;
      
   return n1*n2 <= 0.0f; 
}

bool ShadowVolumeGeometryGenerator::isLightDirectSilhouetteEdge(const osg::Vec4& lightpos, const Edge& edge) const
{
   if (edge.boundaryEdge()) return true;
    
      
   osg::Vec4 delta(lightpos.x(),lightpos.y(),lightpos.z(),1.0);
      
   float n1 = delta * _triangleNormals[edge._t1];
   float n2 = delta * _triangleNormals[edge._t2];

   if (n1==0.0f && n2==0.0f) return false;
      
   return n1*n2 <= 0.0f; 
}

Vec4 ShadowVolumeGeometryGenerator::projectToInf(Vec4 point, Vec4 light)
{
   Vec4 res;
   res = point * light.w() - light;
   return res;
}

void ShadowVolumeGeometryGenerator::setCurrentFacingAndOrdering(StateSet *ss)
{
   FrontFace *ff = dynamic_cast<FrontFace *>(ss->getAttribute(StateAttribute::FRONTFACE));
   if(ff && _faceOredering == AUTO)
   {
      _currentFaceOredering = ff->getMode();
   }

   CullFace *cf = dynamic_cast<CullFace *>(ss->getAttribute(StateAttribute::CULLFACE));
   if(cf && _shadowCastingFace == AUTO)
   {
      _currentShadowCastingFace = cf->getMode();
   }
}