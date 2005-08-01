// $Id$

/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Authors: Matthias Richter <Matthias.Richter@ift.uib.no>                *
 *          Timm Steinbeck <timm@kip.uni-heidelberg.de>                   *
 *          Artur Szostak <artursz@iafrica.com>                           *
 *          for The ALICE Off-line Project.                               *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// pure C interface to the AliRoot HLT component handler                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#if __GNUC__== 3
using namespace std;
#endif

#include "AliHLT_C_Component_WrapperInterface.h"
#include "AliHLTComponentHandler.h"
#include "AliHLTComponent.h"
#include <errno.h>

static AliHLTComponentHandler *gComponentHandler_C = NULL;


int AliHLT_C_Component_InitSystem( AliHLTComponentEnvironment* environ )
{
  if ( gComponentHandler_C )
    {
      return EINPROGRESS;
    }
  gComponentHandler_C = new AliHLTComponentHandler();
  if ( !gComponentHandler_C )
    return EFAULT;
  gComponentHandler_C->SetEnvironment( environ );
  return 0;
}

int AliHLT_C_Component_DeinitSystem()
{
  if ( gComponentHandler_C )
    {
      delete gComponentHandler_C;
      gComponentHandler_C = NULL;
    }
  return 0;
}

int AliHLT_C_Component_LoadLibrary( const char* libraryPath )
{
  if ( !gComponentHandler_C )
    return ENXIO;
  return gComponentHandler_C->LoadLibrary( libraryPath );
}

int AliHLT_C_Component_UnloadLibrary( const char* libraryPath )
{
  if ( !gComponentHandler_C )
    return ENXIO;
  return gComponentHandler_C->UnloadLibrary( libraryPath );
}

int AliHLT_C_CreateComponent( const char* componentType, void* environ_param, int argc, const char** argv, AliHLTComponentHandle* handle )
{
  if ( !gComponentHandler_C )
    return ENXIO;
  AliHLTComponent* comp;
  int ret = gComponentHandler_C->CreateComponent( componentType, environ_param, argc, argv, comp );
  *handle = reinterpret_cast<AliHLTComponentHandle>( comp );
  return ret;
}

void AliHLT_C_DestroyComponent( AliHLTComponentHandle handle )
{
  if ( !handle )
    return;
  delete reinterpret_cast<AliHLTComponent*>( handle );
}

int AliHLT_C_ProcessEvent( AliHLTComponentHandle handle, AliHLTComponent_EventData evtData, AliHLTComponent_BlockData* blocks, 
                           AliHLTComponent_TriggerData trigData, AliHLTUInt8_t* outputPtr,
                           AliHLTUInt32_t* size, AliHLTUInt32_t* outputBlockCnt, 
                           AliHLTComponent_BlockData** outputBlocks,
                           AliHLTComponent_EventDoneData** edd )
{
  if ( !handle )
    return ENXIO;
  AliHLTComponent* comp = reinterpret_cast<AliHLTComponent*>( handle );
  return comp->ProcessEvent( evtData, blocks, trigData, outputPtr, size, outputBlockCnt, outputBlocks, edd );
}
