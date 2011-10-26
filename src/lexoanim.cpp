#include "lexoanim.h"

#include <dtCore/deltawin.h>
#include <dtCore/transform.h>
#include <osgViewer/View>
#include <osgGA/GUIEventAdapter>
#include "lighting/ShadowVolume.h"

LexoanimApp::LexoanimApp(const std::string& configFilename)
   : dtABC::Application(configFilename)
{}

LexoanimApp::~LexoanimApp(){}

void LexoanimApp::Config(){
   // Call the parent class, in case something important is happening there
   Application::Config();

   osgShadow::ShadowVolume::setupCamera(GetCamera()->GetOSGCamera());

   GetWindow()->SetWindowTitle("Lexoanim");
   mCadworkOrbitMotionModel = new dtCore::CadworkOrbitMotionModel(GetKeyboard(), GetMouse(),GetScene());
   //mCadworkOrbitMotionModel->SetTarget(GetCamera());
   mCadworkOrbitMotionModel->SetEnabled(false);

   mCadworkFlyMotionModel = new dtCore::CadworkFlyMotionModel(GetKeyboard(), GetMouse(), dtCore::CadworkFlyMotionModel::OPTION_REQUIRE_MOUSE_DOWN |  dtCore::CadworkFlyMotionModel::OPTION_USE_SIMTIME_FOR_SPEED/* | dtCore::CadworkFlyMotionModel::OPTION_RESET_MOUSE_CURSOR*/);
   mCadworkFlyMotionModel->SetMaximumFlySpeed(10.0);
   mCadworkFlyMotionModel->SetMaximumTurnSpeed(90.0);
   //mCadworkFlyMotionModel->SetTarget(GetCamera());
   mCadworkFlyMotionModel->SetEnabled(false);

   SetActualCameraMotionModel(mCadworkOrbitMotionModel);

   /*mTerrainObject = new dtCore::Object("Model");
   mTerrainObject->LoadFile("F:/grafika/osg/model loading/2011-08-04 Mirror light intensity/mirror.iv");
   AddDrawable(mTerrainObject.get());*/
   //mActualMotionModel->SetTarget(GetCamera());
}

void LexoanimApp::SetActualCameraMotionModel(dtCore::MotionModel * mm)
{ 
   if(!mm)
      return;
   CadworkMotionModelInterface *actualCMMI = dynamic_cast<CadworkMotionModelInterface *> (mActualMotionModel.get());
   CadworkMotionModelInterface *newCMMI = dynamic_cast<CadworkMotionModelInterface *> (mm);
   if(newCMMI && actualCMMI)
   {
      newCMMI->CMMI_SetDistance(actualCMMI->CMMI_GetDistance());
   }

   if(mActualMotionModel) // different Motins models needs diferent cals
   {
      mActualMotionModel->SetEnabled(false);
      dtCore::CadworkOrbitMotionModel *orbit = dynamic_cast<dtCore::CadworkOrbitMotionModel *>(mActualMotionModel.get());
      if(orbit)
      {
         orbit->SetTarget(NULL);
      }
      else
         mActualMotionModel->SetTarget(NULL);
   }
   mActualMotionModel = mm;
   dtCore::CadworkOrbitMotionModel *orbit = dynamic_cast<dtCore::CadworkOrbitMotionModel *>(mm);
   if(orbit)
   {
      orbit->SetTarget(GetCamera());
   }
   else
   {
      mActualMotionModel->SetTarget(GetCamera());
   }
   mActualMotionModel->SetEnabled(true);
   /*if(mm == mCadworkFlyMotionModel)
   {
      mCadworkFlyMotionModel->ShowCursor(true);
   }
   else
   {
      mCadworkFlyMotionModel->ShowCursor(true);
   }*/
}

bool LexoanimApp::KeyPressed(const dtCore::Keyboard * keyboard, int kc)
{
   switch(kc)
   {
   case osgGA::GUIEventAdapter::KEY_F2:
      SetNextStatisticsType();
      break;
   }
   return false;
}