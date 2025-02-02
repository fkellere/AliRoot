//**************************************************************************\
//* This file is property of and copyright by the ALICE Project            *\
//* ALICE Experiment at CERN, All rights reserved.                         *\
//*                                                                        *\
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *\
//*                  for The ALICE HLT Project.                            *\
//*                                                                        *\
//* Permission to use, copy, modify and distribute this software and its   *\
//* documentation strictly for non-commercial purposes is hereby granted   *\
//* without fee, provided that the above copyright notice appears in all   *\
//* copies and that both the copyright notice and this permission notice   *\
//* appear in the supporting documentation. The authors make no claims     *\
//* about the suitability of this software for any purpose. It is          *\
//* provided "as is" without express or implied warranty.                  *\
//**************************************************************************

/// \file GPUDisplayBackendGlfw.h
/// \author David Rohr

#ifndef GPUDISPLAYBACKENDGlfw_H
#define GPUDISPLAYBACKENDGlfw_H

#include "GPUDisplayBackend.h"
#include <pthread.h>

struct GLFWwindow;

namespace GPUCA_NAMESPACE
{
namespace gpu
{
class GPUDisplayBackendGlfw : public GPUDisplayBackend
{
 public:
  GPUDisplayBackendGlfw() = default;
  ~GPUDisplayBackendGlfw() override = default;

  int StartDisplay() override;
  void DisplayExit() override;
  void SwitchFullscreen(bool set) override;
  void ToggleMaximized(bool set) override;
  void SetVSync(bool enable) override;
  void OpenGLPrint(const char* s, float x, float y, float r, float g, float b, float a, bool fromBotton = true) override;
  bool EnableSendKey() override;

 private:
  int OpenGLMain() override;
  static void DisplayLoop();

  static void error_callback(int error, const char* description);
  static void key_callback(GLFWwindow* mWindow, int key, int scancode, int action, int mods);
  static void mouseButton_callback(GLFWwindow* mWindow, int button, int action, int mods);
  static void scroll_callback(GLFWwindow* mWindow, double x, double y);
  static void cursorPos_callback(GLFWwindow* mWindow, double x, double y);
  static void resize_callback(GLFWwindow* mWindow, int width, int height);
  static int GetKey(int key);
  static void GetKey(int keyin, int scancode, int mods, int& keyOut, int& keyPressOut);

  GLFWwindow* mWindow;

  volatile bool mGlfwRunning = false;
  pthread_mutex_t mSemLockExit = PTHREAD_MUTEX_INITIALIZER;
  int mWindowX = 0;
  int mWindowY = 0;
  int mWindowWidth = INIT_WIDTH;
  int mWindowHeight = INIT_HEIGHT;
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif
