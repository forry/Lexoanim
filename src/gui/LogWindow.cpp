/**
 * @file
 * LogWindow class implementation.
 *
 * @author Tomáš "Shamot" Burian
 *         PCJohn (Jan Pečiva)
 */


#include <QComboBox>
#include <QScrollBar>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <cassert>
#include <osg/io_utils>
#include "LogWindow.h"
#include "Lexolights.h"
#include "CadworkViewer.h"
#include "MainWindow.h"
#include "OSGWidget.h"
#include "utils/Log.h"
#include "utils/SysInfo.h"


static OpenThreads::Mutex pendingMessagesMutex;

static QStringList logHighlightTag = ( QStringList()
<< "<font color=\"#0000ff\">" << "</font>"  // ALWAYS
<< "<font color=\"#ff0000\">" << "</font>"  // FATAL
<< "<font color=\"#ff0000\">" << "</font>"  // WARN
<< "<font color=\"#000000\">" << "</font>"  // NOTICE
<< "<font color=\"#6060ff\">" << "</font>"  // INFO
<< "<font color=\"#a0a0ff\">" << "</font>"  // DEBUG_INFO
);



static void getHighlightTag( osg::NotifySeverity severity, QString &highlighting1,
                             QString &highlighting2 )
{
   // check bounds
   if( severity < 0 )
      severity = osg::NotifySeverity( 0 );
   if( severity*2 >= logHighlightTag.size() )
      severity = osg::NotifySeverity( logHighlightTag.size() / 2 - 1 );

   // set highlighting
   highlighting1 = logHighlightTag[severity*2+0];
   highlighting2 = logHighlightTag[severity*2+1];
}


/**
 * Constructor.
 *
 * @param parent Parent widget.
 * @param flags  Window flags (Qt::WindowFlags).
 */
LogWindow::LogWindow( QWidget *parent, Qt::WFlags flags )
    : QDockWidget( parent, flags ),
      showLevel( osg::NOTICE ),
      updateMessagesScheduled( false )
{
   setObjectName( "LogWindow" );

   // build GUI
   this->buildGui();
}


/**
 * Destructor.
 */
LogWindow::~LogWindow()
{
}


/**
 * Main GUI-building function.
 *
 * It is called by constructor. Do not set text messages in this method.
 */
void LogWindow::buildGui()
{
   this->setMinimumSize( 150, 200 );
   this->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::MinimumExpanding );
   this->setAllowedAreas( Qt::BottomDockWidgetArea );

   // update signal
   connect( this, SIGNAL( updateMessagesSignal() ),
            this, SLOT( updateMessages() ), Qt::QueuedConnection );

   // pending messages signal
   connect( this, SIGNAL( pendingMessagesSignal() ),
            this, SLOT( startPendingMessagesTimer() ), Qt::QueuedConnection );

   // main widget
   mainWidget = new QWidget( this );

   // main layout (vertical)
   mainLayout = new QVBoxLayout( mainWidget );
   mainLayout->setMargin( 2 );
   mainLayout->setSpacing( 2 );

   // controls layout (horizontal)
   controlsLayout = new QHBoxLayout();
   controlsLayout->setMargin( 2 );
   controlsLayout->setSpacing( 2 );
   mainLayout->addLayout( controlsLayout );

   showLevelChooser = new QComboBox();
   showLevelChooser->addItem( "FATAL" );
   showLevelChooser->addItem( "WARN" );
   showLevelChooser->addItem( "NOTICE" );
   showLevelChooser->addItem( "INFO" );
   showLevelChooser->addItem( "DEBUG_INFO" );
   int showLevel = 2;
   if( Log::isLogLevelGivenByEnv() )
      switch( osg::getNotifyLevel() ) {
         case osg::ALWAYS: showLevel = 0; break;
         case osg::FATAL:  showLevel = 0; break;
         case osg::WARN:   showLevel = 1; break;
         case osg::NOTICE: showLevel = 2; break;
         case osg::INFO:   showLevel = 3; break;
         case osg::DEBUG_INFO: showLevel = 4; break;
         default: showLevel = 2;
      };
   showLevelChooser->setCurrentIndex( showLevel );
   connect( showLevelChooser, SIGNAL( currentIndexChanged( const QString& ) ),
            this, SLOT( showLevelChanged( const QString& ) ) );
   controlsLayout->addWidget( showLevelChooser );

   // chooseSomething
   chooseSomething = new QComboBox();
   chooseSomething->addItem( "OpenGL version" );
   chooseSomething->addItem( "OpenGL extensions" );
   chooseSomething->addItem( "OpenGL limits" );
   chooseSomething->addItem( "GLSL limits" );
   chooseSomething->addItem( "Graphics driver info" );
   chooseSomething->addItem( "Video memory info" );
   chooseSomething->addItem( "Screen info" );
   chooseSomething->addItem( "Libraries info" );
   chooseSomething->addItem( "Camera view data" );
   controlsLayout->addWidget( chooseSomething );

   // printSomething
   printSomething = new QToolButton();
   printSomething->setText( "Show" );
   connect( printSomething, SIGNAL( pressed() ),
            this, SLOT( printSomethingCB() ) );
   controlsLayout->addWidget( printSomething );

   // add stretch
   controlsLayout->addStretch();

   // messageWidget
   messageWidget = new QTextEdit( mainWidget );
   mainLayout->addWidget( messageWidget );

   // messageWidget settings
   // (style sheet is required to make paragraphs without margin)
   messageWidget->setFrameStyle( QFrame::Box | QFrame::Sunken );
   messageWidget->setFont( QFont("Courier new") );
   messageWidget->setReadOnly( true );
   messageWidget->setUndoRedoEnabled( false );
   messageWidget->document()->setDefaultStyleSheet( "p {margin:0px;}" );

   this->setWidget( mainWidget );

   // create timer for updating messages
   timer = new QTimer( this );
   timer->setSingleShot( true );
   timer->setInterval( 100 ); // 100ms
   connect( timer, SIGNAL( timeout() ), this, SLOT( processPendingMessages() ) );

   // call showLevelChanged after initializing all the widgets
   if( showLevel != 2 )
      showLevelChanged( showLevelChooser->itemText( showLevel ) );
}


/** The method regenerates the content of messageWidget
 *  from message list in Log (lock/unlockMessageList())
 *  filtered by the selected message level.
 *
 *  The method must be called from gui thread only. */
void LogWindow::updateMessages()
{
   // is scrollbar at the bottom?
   bool keepAtBottom = isScrolledDown();

   // empty message widget
   messageWidget->clear();
   QString newText;

   {
      // take the lock (always get messages list lock before pending messages lock)
      const Log::MessageList &list = Log::lockMessageList();

      // stop timer and empty pending messages
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(pendingMessagesMutex);
      pendingMessages.clear();
      timer->stop();

      // append messages to the message widget
      appendMessages( list.begin(), list.end() );
      Log::unlockMessageList();
   }

   // scroll to bottom
   if( keepAtBottom )  makeScrolledDown();

   // reset update request flag
   updateMessagesScheduled = false;
}


/** The method appends messages to the messageWidget.
 *
 *  It must be called from gui thread only. */
void LogWindow::appendMessages( Log::MessageList::const_iterator start, Log::MessageList::const_iterator end )
{
   QString text;

   while( start != end ) {

      // ignore messages bellow the current showLevel
      if( start->severity <= showLevel ) {

         // append message to text
         text += message2html( *start );
      }

      // next message in the list
      start++;

   }

   // put the text to the message widget
   if( !text.isEmpty() )
      messageWidget->append( text );
}


/** The method sends updateMessagesSignal() that causes updateMessages() method to be invoked.
 *
 *  It must be called from gui thread only. */
void LogWindow::invalidateMessages()
{
   if( !updateMessagesScheduled ) {

      updateMessagesScheduled = true;
      emit updateMessagesSignal();

   }
}


/** Adds message to the log window. The message is put to the pending messages list first
 *  and applied after the internal timer expires. The timer is usually set to 100ms.
 *  This avoids excesive redraws of the log window that puts heavy loads on the system.
 *
 *  The method can be called from any thread.
 *
 *  @param text Text.
 */
void LogWindow::message( const Log::MessageRec &msg )
{
   // ignore messages bellow the current showLevel
   if( msg.severity > showLevel )
      return;

   // append message to pending queue
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(pendingMessagesMutex);
      pendingMessages.push_back( msg );
   }

   emit pendingMessagesSignal();
}


/** The slot called from pendingMessagesSignal(). It starts the timer that will
 *  put pending messages to the log window after the expiry.
 *
 *  The purpose of the slot is that it is called from gui thread while the
 *  pendingMessagesSignal() can be emitted from any thread. */
void LogWindow::startPendingMessagesTimer()
{
   // start timer that will trigger message widget update
   if( !timer->isActive() )
      timer->start();
}


/** Processes pending messages by putting them to the messageWidget.
 *
 *  The method must be called from gui thread. */
void LogWindow::processPendingMessages()
{
   // is scrollbar at the bottom?
   bool keepAtBottom = isScrolledDown();

   {
      // append messages to the message widget
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(pendingMessagesMutex);
      appendMessages( pendingMessages.begin(), pendingMessages.end() );
      pendingMessages.clear();
   }

   // scroll to bottom
   if( keepAtBottom )  makeScrolledDown();
}


bool LogWindow::isScrolledDown() const
{
   return messageWidget->verticalScrollBar()->value() == messageWidget->verticalScrollBar()->maximum();
}


void LogWindow::makeScrolledDown()
{
   messageWidget->verticalScrollBar()->setValue( messageWidget->verticalScrollBar()->maximum() );
}


void LogWindow::printSomethingCB()
{
   OSGWidget *osgWidget = dynamic_cast< OSGWidget* >( Lexolights::mainWindow()->getGLWidget() );
   if( !osgWidget ) {
      Log::warn() << "LogWindow warning: can not print info as OpenGL widget is not osg OSGWidget type." << std::endl;
      return;
   }

   switch( chooseSomething->currentIndex() ) {
      case 0: osgWidget->printOpenGLVersion();
              break;
      case 1: osgWidget->printOpenGLExtensions();
              break;
      case 2: osgWidget->printOpenGLLimits();
              break;
      case 3: osgWidget->printGLSLLimits();
              break;
      case 4: osgWidget->printGraphicsDriverInfo();
              break;
      case 5: osgWidget->printVideoMemoryInfo();
              break;
      case 6: osgWidget->printScreenInfo();
              break;
      case 7: Log::always() << SysInfo::getLibInfo() << Log::endm;
              break;
      case 8: {
                 osg::Camera *camera = Lexolights::viewer()->getSceneWithCamera();
                 if( !camera )
                    Log::always() << "Camera is NULL." << Log::endm;
                 else {
                    osg::Vec3d eye, center, up;
                    double fovy, tmp, zNear, zFar;
                    camera->getViewMatrixAsLookAt( eye, center, up );
                       camera->getProjectionMatrixAsPerspective( fovy, tmp, zNear, zFar );
                    Log::always() << "Camera view data:\n"
                                     "   Position:  " << eye << "\n"
                                     "   Direction: " << (center-eye) << "\n"
                                     "   Up vector: " << up << "\n"
                                     "   FOV (in vertical direction): " << fovy << "\n"
                                     "   zNear,zFar: " << zNear << "," << zFar << Log::endm;
                 }
              }
              break;
   };
}


void LogWindow::showLevelChanged( const QString& text )
{
   osg::NotifySeverity newShowLevel;
   if( text == "FATAL" )       newShowLevel = osg::FATAL;
   else if( text == "WARN" )   newShowLevel = osg::WARN;
   else if( text == "NOTICE" ) newShowLevel = osg::NOTICE;
   else if( text == "INFO" )   newShowLevel = osg::INFO;
   else if( text == "DEBUG_INFO" ) newShowLevel = osg::DEBUG_INFO;
   else { assert( false && "Unknown showLevel in LogWindow." ); return; }

   setShowLevel( newShowLevel );
}


/**
 * Cenverts message to html text that is expected to be shown in message widget of LogWindow.
 * Conversion includes time stamp, highlighting tags, etc..
 */
QString LogWindow::message2html( const Log::MessageRec &msg )
{
   // remove terminating \n as QTextEdit automatically breaks messages
   // into something like paragraphs
   QString t( msg.text );
   if( t.endsWith( '\n' ) )
      t.chop( 1 );

   // replace < and >
   t.replace( '<', "&lt;" );
   t.replace( '>', "&gt;" );

   // replace endl
   t.replace( '\n', "<br>" );

   // use non-breaking spaces (nbsp) as
   // standard spaces merge together in html
   t.replace( "  ", " &nbsp;" );

   // highlighting
   QString highlighting1;
   QString highlighting2;
   getHighlightTag( msg.severity, highlighting1, highlighting2 );

   // create string and return it
   return ( QString( "<p>" ) + highlighting1 + "[%1] %2" + highlighting2 + "</p>" )
                     .arg( msg.time, 6, 'f', 3 )
                     .arg( t );
}


/**
 * Set filter for messages.
 * level parameter gives the minimum notify level
 * for the messages that will be shown.
 */
void LogWindow::setShowLevel( osg::NotifySeverity level )
{
   if( level == showLevel )
      return;

   // set level
   showLevel = level;

   // invalidate message window
   invalidateMessages();

   // update showLevelChooser
   if( sender() != showLevelChooser )
   {
      QString text;
      switch( showLevel ) {
         case osg::FATAL:      text = "FATAL";  break;
         case osg::WARN:       text = "WARN";   break;
         case osg::NOTICE:     text = "NOTICE"; break;
         case osg::INFO:       text = "INFO";   break;
         case osg::DEBUG_INFO: text = "DEBUG_INFO"; break;
         default:
            assert( false && "Unknown showLevel." );
            return;
      }
      int i = showLevelChooser->findText( text );
      if( i != -1 )
         showLevelChooser->setCurrentIndex( i );
   }
}


void LogWindow::showEvent( QShowEvent* event )
{
   invalidateMessages();
}
