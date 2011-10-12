
/**
 * @file
 * CentralContainer class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef CENTRAL_CONTAINER_H
#define CENTRAL_CONTAINER_H

#include <QWidget>
#include <QPointer>
#include <list>

namespace osgViewer {
   class GraphicsWindow;
};
class QGLWidget;


/**
 * Central container is meant to hold list of widgets put on the place
 * of central widget of QMainWindow.
 *
 * Central container must be set as central widget of QMainWindow and
 * it allows for switching among the widgets it maintains.
 * Only one widget is active at a time and this widget is visible on the screen.
 * A typical usage is the list of QGLWidgets with various OpenGL
 * capabilities (stereo, non-stereo, antialiasing,...).
 */
class CentralContainer : public QWidget
{
public:

   CentralContainer( QWidget *parent = NULL );

   virtual void addWidget( QWidget *widget, void (*activationFunc)( QWidget *w, osg::ref_ptr< osgViewer::GraphicsWindow > &gw ) = NULL,
                           osgViewer::GraphicsWindow *graphicsWindow = NULL );
   virtual void removeWidget( QWidget *widget );
   virtual void setActiveWidget( QWidget *w );
   inline QWidget* activeWidget();

   virtual void internalSetActiveWidget( QWidget *w );

   inline QGLWidget* lastGLWidget();
   virtual void switchToLastGLWidget();

protected:

   virtual void resizeEvent( QResizeEvent *event );

   struct Item {
      QWidget* _widget;
      void (*_activationFunc)( QWidget *w, osg::ref_ptr< osgViewer::GraphicsWindow > &gw );
      osg::ref_ptr< osgViewer::GraphicsWindow > _graphicsWindow;
      Item( QWidget* widget, void (*activationFunc)( QWidget *w, osg::ref_ptr< osgViewer::GraphicsWindow > &gw ),
            osgViewer::GraphicsWindow *graphicsWindow ) :
         _widget( widget ), _activationFunc( activationFunc ), _graphicsWindow( graphicsWindow )  {}
   };
   typedef std::list< Item > ItemList;

   ItemList           _children;
   QSize              _currentSize;
   QWidget*           _activeWidget;
   QPointer< QGLWidget > _lastGLWidget;
};


//
//  inline methods
//
inline QWidget* CentralContainer::activeWidget()  { return _activeWidget; }
inline QGLWidget* CentralContainer::lastGLWidget()  { return _lastGLWidget; }


#endif /* CENTRAL_CONTAINER_H */
