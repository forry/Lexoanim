#include <osg/Drawable>

namespace osg
{
/** Clear drawable encapsulates glClear OpenGL command and enables an user to perform buffer clear
 *  operation during scene rendering. It is useful in multipass algorithms, for example by
 *  shadow volumes that needs to clear stencil buffer before each light-processing pass.
 *
 *  Bitwise OR of following values can be used for specifying which buffers are to be cleared:
 *  GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT and GL_ACCUM_BUFFER_BIT. */
class ClearGLBuffersDrawable : public osg::Drawable
{
   typedef osg::Drawable inherited;
public:
   /** Constructor that sets buffer mask to its default value,
    *  e.g. color and depth buffer will be cleared by the drawable. */
   ClearGLBuffersDrawable() : _bufferMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) {}
   /** Constructor taking buffer mask as parameter. */
   ClearGLBuffersDrawable( unsigned int bufferMask ) : _bufferMask( bufferMask ) {}
   /** Copy constructor using CopyOp to manage deep vs shallow copy. */
   ClearGLBuffersDrawable( const ClearGLBuffersDrawable& clear, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY ) :
      inherited( clear, copyop ) {}
   /** Destructor. */
   virtual ~ClearGLBuffersDrawable() {}

   virtual osg::Object* cloneType() const { return new ClearGLBuffersDrawable(); }
   virtual osg::Object* clone( const osg::CopyOp& copyop ) const { return new ClearGLBuffersDrawable( *this, copyop ); }
   virtual bool isSameKindAs( const osg::Object* obj ) const { return dynamic_cast< const ClearGLBuffersDrawable* >( obj ) != NULL; }
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

}
