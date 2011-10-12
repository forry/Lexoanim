
/**
 * @file
 * OSGWindow class header.
 *
 * @author PCJohn (Jan Pečiva)
 *         Tomáš Pafčo
 */


#ifndef OSG_WINDOW_H
#define OSG_WINDOW_H


#include <osgQt/GraphicsWindowQt>


/**
 * OSGWidget is a widget inherited from osgQt::GLWidget
 * and customized for the purpose of Lexolights.
 */
class OSGWidget : public osgQt::GLWidget
{
   typedef osgQt::GLWidget inherited;

public:

   // constructor & destructor
   OSGWidget( QWidget* parent = NULL, const QGLWidget* shareWidget = NULL, Qt::WindowFlags f = 0 );
   OSGWidget( QGLContext* context, QWidget* parent = NULL, const QGLWidget* shareWidget = NULL, Qt::WindowFlags f = 0 );
   OSGWidget( const QGLFormat& format, QWidget* parent = NULL, const QGLWidget* shareWidget = NULL, Qt::WindowFlags f = 0 );
   ~OSGWidget();

public slots:

   void printOpenGLVersion();
   void printOpenGLExtensions();
   void printOpenGLLimits();
   void printGLSLLimits();
   void printGraphicsDriverInfo();
   void printVideoMemoryInfo();
   void printScreenInfo();

protected:

   void commonConstructor();

   virtual void initializeGL();
   virtual void mouseMoveEvent( QMouseEvent *e );
   virtual void mousePressEvent( QMouseEvent *e );
   virtual void mouseReleaseEvent( QMouseEvent *e );
   virtual void wheelEvent( QWheelEvent *e );
   virtual void keyPressEvent( QKeyEvent *event );
   virtual void keyReleaseEvent( QKeyEvent *event );

private:

   // cursors
   QCursor defaultCursor;
   QCursor zrotCursor;
   QCursor rotCursor;
   QCursor hrotCursor;
   QCursor vrotCursor;
   QCursor movCursor;
   QCursor lookCursor;

};


#endif
