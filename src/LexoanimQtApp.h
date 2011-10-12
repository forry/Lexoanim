#ifndef LEXOANIM_QT_APP
#define LEXOANIM_QT_APP

#include "Lexolights.h"

class LexoanimQtApp : public Lexolights
{
   Q_OBJECT

public:
   LexoanimQtApp( int &argc, char* argv[], bool initialize = true );
   virtual ~LexoanimQtApp();

   // initialize Lexolights object
   virtual void init();
};
#endif //LEXOANIM_QT_APP