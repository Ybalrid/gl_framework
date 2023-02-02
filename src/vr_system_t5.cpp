#include "vr_system_t5.hpp"

vr_system_t5::vr_system_t5()
{
  std::cout << "Initialized TiltFive based vr_system implementation\n";

  caps = caps_hmd_3dof | caps_hmd_6dof | caps_hand_controllers;
}

vr_system_t5::~vr_system_t5() { }

bool vr_system_t5::initialize(sdl::Window& window)
{
  std::cout << "Initializing TiltFive based VR system\n";

  T5_Result err = T5_SUCCESS;

  err = t5CreateContext(&t5ctx, &clientInfo, nullptr);
  if(err)
  {
    std::cout << "Failed to create TiltFive context." << t5GetResultMessage(err) << std::endl;
    return false;
  }

  //Get all gameboard sizes we could support
  //TODO checkerror
  err = t5GetGameboardSize(t5ctx, kT5_GameboardType_XE_Raised, &gameboardSizeXERaised);
  err = t5GetGameboardSize(t5ctx, kT5_GameboardType_XE, &gameboardSizeXE);
  err = t5GetGameboardSize(t5ctx, kT5_GameboardType_LE, &gameboardSizeLE);


  //Get t5 service
  {
    bool waiting = false;
    for(;;)//TODO this comes from the example, I am not a fan
    {
      char serviceVersion[T5_MAX_STRING_PARAM_LEN];
      size_t bufferSize = T5_MAX_STRING_PARAM_LEN;
      err               = t5GetSystemUtf8Param(t5ctx, kT5_ParamSys_UTF8_Service_Version, serviceVersion, &bufferSize);

      if(!err)
      {
        std::cout << "Tilt Five service version : " << serviceVersion << std::endl;
        break; //service found.
      }

      if(T5_ERROR_NO_SERVICE == err)
      {
        if(!waiting)
        {
          std::cout << "Waiting for TiltFive service ... \n";
          //TODO print "waiting for service ..." 
          waiting = true;
        }
      }
      else
      {
        const auto errorMessage = t5GetResultMessage(err);
        std::cout << "Failed to obtain TiltFive service version : " << errorMessage << std::endl;
        return false;
      }
    }
  }

  //Get glasses1
  {
    
  }



  return false;
}

void vr_system_t5::build_camera_node_system() { }

void vr_system_t5::wait_until_next_frame() { }

void vr_system_t5::update_tracking() { }

void vr_system_t5::submit_frame_to_vr_system() { }

bool vr_system_t5::must_vflip() const { return false; }

void vr_system_t5::update_mr_camera() { }
