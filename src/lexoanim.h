#ifndef LEXOANIM
#define LEXOANIM

#include <dtABC/application.h>
#include <dtCore/refptr.h>
#include <dtCore/object.h>
//#include <dtCore/orbitmotionmodel.h>
#include "cadworkorbitmotionmodel.h"
#include "cadworkflymotionmodel.h"



class LexoanimApp : public dtABC::Application {
public:
   LexoanimApp(const std::string& configFilename);

   virtual void Config();

   inline dtCore::MotionModel *GetActualMotionModel(){ return mActualMotionModel.get();}
   void SetActualCameraMotionModel(dtCore::MotionModel * mm);

   inline dtCore::CadworkFlyMotionModel *GetFlyMotionModel() { return mCadworkFlyMotionModel.get();}
   inline dtCore::CadworkOrbitMotionModel *GetOrbitMotionModel() { return mCadworkOrbitMotionModel.get();}

protected:
   virtual ~LexoanimApp();
   virtual bool KeyPressed(const dtCore::Keyboard * keyboard, int kc);

private:
   dtCore::RefPtr<dtCore::MotionModel> mActualMotionModel;
   dtCore::RefPtr<dtCore::CadworkFlyMotionModel> mCadworkFlyMotionModel;
   dtCore::RefPtr<dtCore::CadworkOrbitMotionModel> mCadworkOrbitMotionModel;
};

#endif