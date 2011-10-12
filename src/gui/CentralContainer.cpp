
/**
 * @file
 * CentralContainer class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <QResizeEvent>
#include <QGLWidget>
#include <osgViewer/GraphicsWindow>
#include "CentralContainer.h"
#include "utils/Log.h"

using namespace std;
using namespace osg;


CentralContainer::CentralContainer( QWidget *parent )
   : QWidget( parent ),
     _currentSize( parent->size() ),
     _activeWidget( NULL )
{
}


void CentralContainer::addWidget( QWidget *widget, void (*activationFunc)( QWidget *w, ref_ptr< osgViewer::GraphicsWindow > &gw ),
                                  osgViewer::GraphicsWindow *graphicsWindow )
{
   // ignore NULL widgets
   if( widget == NULL )
      return;

   bool firstWidget = _children.empty();

   // update widgets
   widget->setParent( this );
   widget->setFixedSize( _currentSize );

   // put the widget to the children list
   _children.push_back( Item( widget, activationFunc, graphicsWindow ) );

   // update widgets visibility
   if( firstWidget )
      setActiveWidget( widget );
   else
      widget->hide();
}


void CentralContainer::removeWidget( QWidget *widget )
{
   // ignore NULL widgets
   if( widget == NULL )
      return;

   std::list< Item >::iterator it;
   for( it = _children.begin(); it != _children.end(); it++ )
      if( it->_widget == widget )
      {
         _children.erase( it );
         if( _activeWidget == widget )
            switchToLastGLWidget();

         widget->setParent( NULL );
         return;
      }
}

void CentralContainer::setActiveWidget( QWidget *w )
{
   if( _activeWidget != w ) {

      // if not in the children list, append it
      ItemList::iterator it;
      for( it = _children.begin(); it != _children.end(); it++ )
      {
         if( it->_widget == w )  break;
      }
      if( it == _children.end() )
      {
         // append widget
         addWidget( w );

         // if widget was activated during append operation, just return
         if( _activeWidget == w )
            return;

         // find the widget
         for( it = _children.begin(); it != _children.end(); it++ )
         {
            if( it->_widget == w )  break;
         }
      }

      // hide previously active widget
      if( _activeWidget )
         _activeWidget->hide();

      if( it->_activationFunc )
      {
         it->_activationFunc( it->_widget, it->_graphicsWindow );
      }
      else
      {
         internalSetActiveWidget( w );
      }
   }
}


void CentralContainer::internalSetActiveWidget( QWidget *w )
{
   // keep record of the recently active QGLWidget
   QGLWidget *previousGLWidget = dynamic_cast< QGLWidget* >( _activeWidget );
   if( previousGLWidget )
      _lastGLWidget = previousGLWidget;

   // set active widget
   _activeWidget = w;

   // show the widget
   if( w )
      w->show();
}


void CentralContainer::resizeEvent( QResizeEvent *event )
{
   for( ItemList::iterator it = _children.begin(); it != _children.end(); it++ )
      it->_widget->setFixedSize( event->size() );
   _currentSize = event->size();
}


void CentralContainer::switchToLastGLWidget()
{
   if( _lastGLWidget != NULL &&
       _lastGLWidget != _activeWidget )
   {
      setActiveWidget( _lastGLWidget );
   }
   else
   {
      // fallback: switch to any GLWidget (the first one in _children, except the active one)
      bool activeFound = false;
      for( ItemList::iterator it = _children.begin(); it != _children.end(); it++ )
         if( dynamic_cast< QGLWidget* >( it->_widget ) != NULL )
         {
            // handle 
            if( it->_widget == _activeWidget )
            {
               activeFound = true;
               continue;
            }

            Log::info() << "CentralContainer::switchToLastGLWidget(): the last QGLWidget not found.\n"
                           "   (It was destroyed or no QGLWidget was ever made active.)\n"
                           "   Using another QGLWidget from CentralContainer." << endl;
            setActiveWidget( it->_widget );
            return;
         }

      // keep active QGLWidget if no other QGLWidgets exists
      if( activeFound )
         return;

      // no OSGWidgets exist
      Log::warn() << "CentralContainer::switchToLastGLWidget(): no QGLWidget exists in CentralContainer." << endl;
   }
}
