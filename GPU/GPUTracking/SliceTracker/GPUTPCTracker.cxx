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

/// \file GPUTPCTracker.cxx
/// \author Sergey Gorbunov, Ivan Kisel, David Rohr

#include "GPUTPCTracker.h"
#include "GPUTPCRow.h"
#include "GPUTPCTrack.h"
#include "GPUCommonMath.h"

#include "GPUTPCClusterData.h"
#include "GPUTPCSliceOutput.h"
#include "GPUTPCTrackletConstructor.h"
#include "GPUTPCTrackLinearisation.h"

#include "GPUTPCTrackParam.h"

#if !defined(GPUCA_GPUCODE)
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "GPUReconstruction.h"
#endif

using namespace GPUCA_NAMESPACE::gpu;

#if !defined(GPUCA_GPUCODE)

GPUTPCTracker::GPUTPCTracker()
  : GPUProcessor(), mLinkTmpMemory(nullptr), mISlice(-1), mData(), mNMaxStartHits(0), mNMaxTracklets(0), mNMaxTracks(0), mNMaxTrackHits(0), mMemoryResLinksScratch(-1), mMemoryResScratchHost(-1), mMemoryResCommon(-1), mMemoryResTracklets(-1), mMemoryResOutput(-1), mRowStartHitCountOffset(nullptr), mTrackletTmpStartHits(nullptr), mGPUTrackletTemp(nullptr), mGPUParametersConst(), mCommonMem(nullptr), mTrackletStartHits(nullptr), mTracklets(nullptr), mTrackletRowHits(nullptr), mTracks(nullptr), mTrackHits(nullptr), mOutput(nullptr), mOutputMemory(nullptr)
{
}

GPUTPCTracker::~GPUTPCTracker()
{
  if (mOutputMemory) {
    free(mOutputMemory);
  }
}

// ----------------------------------------------------------------------------------
void GPUTPCTracker::SetSlice(int iSlice) { mISlice = iSlice; }
void GPUTPCTracker::InitializeProcessor()
{
  if (mISlice < 0) {
    throw std::runtime_error("Slice not set");
  }
  InitializeRows(mCAParam);
  SetupCommonMemory();
}

void* GPUTPCTracker::SetPointersDataInput(void* mem)
{
  return mData.SetPointersInput(mem, mRec->GetRecoStepsGPU() & GPUReconstruction::RecoStep::TPCMerging);
}

void* GPUTPCTracker::SetPointersDataScratch(void* mem)
{
  return mData.SetPointersScratch(mem);
}

void* GPUTPCTracker::SetPointersDataRows(void* mem)
{
  return mData.SetPointersRows(mem);
}

void* GPUTPCTracker::SetPointersScratch(void* mem)
{
  computePointerWithAlignment(mem, mTrackletStartHits, mNMaxStartHits);
  if (mRec->GetDeviceProcessingSettings().memoryAllocationStrategy != GPUMemoryResource::ALLOCATION_INDIVIDUAL) {
    mem = SetPointersTracklets(mem);
  }
  if (mRec->IsGPU()) {
    computePointerWithAlignment(mem, mTrackletTmpStartHits, GPUCA_ROW_COUNT * GPUCA_MAX_ROWSTARTHITS);
    computePointerWithAlignment(mem, mRowStartHitCountOffset, GPUCA_ROW_COUNT);
  }
  return mem;
}

void* GPUTPCTracker::SetPointersScratchHost(void* mem)
{
  computePointerWithAlignment(mem, mLinkTmpMemory, mRec->Res(mMemoryResLinksScratch).Size());
  mem = mData.SetPointersScratchHost(mem, mRec->GetRecoStepsGPU() & GPUReconstruction::RecoStep::TPCMerging);
  return mem;
}

void* GPUTPCTracker::SetPointersCommon(void* mem)
{
  computePointerWithAlignment(mem, mCommonMem, 1);
  return mem;
}

void GPUTPCTracker::RegisterMemoryAllocation()
{
  mMemoryResLinksScratch = mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersDataScratch, GPUMemoryResource::MEMORY_SCRATCH, "SliceLinks");
  mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersDataInput, GPUMemoryResource::MEMORY_INPUT, "SliceInput");
  mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersScratch, GPUMemoryResource::MEMORY_SCRATCH, "TrackerScratch");
  mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersScratchHost, GPUMemoryResource::MEMORY_SCRATCH_HOST, "TrackerHost");
  mMemoryResCommon = mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersCommon, GPUMemoryResource::MEMORY_PERMANENT, "TrackerCommon");
  mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersDataRows, GPUMemoryResource::MEMORY_PERMANENT, "SliceRows");

  auto type = GPUMemoryResource::MEMORY_OUTPUT;
  if (mRec->GetDeviceProcessingSettings().memoryAllocationStrategy == GPUMemoryResource::ALLOCATION_INDIVIDUAL) { // For individual scheme, we allocate tracklets separately, and change the type for the following allocations to custom
    type = GPUMemoryResource::MEMORY_CUSTOM;
    mMemoryResTracklets = mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersTracklets, type, "TrackerTracklets");
  }
  mMemoryResOutput = mRec->RegisterMemoryAllocation(this, &GPUTPCTracker::SetPointersOutput, type, "TrackerTracks");
}

GPUhd() void* GPUTPCTracker::SetPointersTracklets(void* mem)
{
  computePointerWithAlignment(mem, mTracklets, mNMaxTracklets);
#ifdef GPUCA_EXTERN_ROW_HITS
  computePointerWithAlignment(mem, mTrackletRowHits, mNMaxTracklets * GPUCA_ROW_COUNT);
#endif
  return mem;
}

GPUhd() void* GPUTPCTracker::SetPointersOutput(void* mem)
{
  computePointerWithAlignment(mem, mTracks, mNMaxTracks);
  computePointerWithAlignment(mem, mTrackHits, mNMaxTrackHits);
  return mem;
}

void GPUTPCTracker::SetMaxData()
{
  if (mRec->GetDeviceProcessingSettings().memoryAllocationStrategy == GPUMemoryResource::ALLOCATION_INDIVIDUAL) {
    mNMaxStartHits = mData.NumberOfHits();
  } else {
    mNMaxStartHits = GPUCA_MAX_TRACKLETS;
  }
  mNMaxTracklets = GPUCA_MAX_TRACKLETS;
  mNMaxTracks = GPUCA_MAX_TRACKS;
  mNMaxTrackHits = mData.NumberOfHits() + 1000;
#ifdef GPUCA_SORT_STARTHITS
  if (mRec->IsGPU()) {
    if (mNMaxStartHits > GPUCA_MAX_ROWSTARTHITS * GPUCA_ROW_COUNT) {
      mNMaxStartHits = GPUCA_MAX_ROWSTARTHITS * GPUCA_ROW_COUNT;
    }
  }
#endif
  mData.SetMaxData();
}

void GPUTPCTracker::UpdateMaxData()
{
  mNMaxTracklets = mCommonMem->nTracklets;
  mNMaxTracks = mCommonMem->nTracklets * 2 + 50;
}

void GPUTPCTracker::SetupCommonMemory() { new (mCommonMem) commonMemoryStruct; }

int GPUTPCTracker::ReadEvent()
{
  SetupCommonMemory();

  //* Convert input hits, create grids, etc.
  if (mData.InitFromClusterData()) {
    printf("Error initializing from cluster data\n");
    return 1;
  }
  if (mData.MaxZ() > 300 && !mCAParam->ContinuousTracking) {
    printf("Need to set continuous tracking mode for data outside of the TPC volume!\n");
    return 1;
  }
  return 0;
}

GPUh() int GPUTPCTracker::CheckEmptySlice()
{
  // Check if the Slice is empty, if so set the output apropriate and tell the reconstuct procesdure to terminate
  if (NHitsTotal() < 1) {
    mCommonMem->nTracks = mCommonMem->nTrackHits = 0;
    WriteOutputPrepare();
    mOutput->SetNTracks(0);
    mOutput->SetNTrackClusters(0);
    return 1;
  }
  return 0;
}

GPUh() void GPUTPCTracker::WriteOutputPrepare() { GPUTPCSliceOutput::Allocate(mOutput, mCommonMem->nTracks, mCommonMem->nTrackHits, &mRec->OutputControl(), mOutputMemory); }

template <class T>
static inline bool SortComparison(const T& a, const T& b)
{
  return (a.fSortVal < b.fSortVal);
}

GPUh() void GPUTPCTracker::WriteOutput()
{
  mOutput->SetNTracks(0);
  mOutput->SetNLocalTracks(0);
  mOutput->SetNTrackClusters(0);

  if (mCommonMem->nTracks == 0) {
    return;
  }
  if (mCommonMem->nTracks > GPUCA_MAX_SLICE_NTRACK) {
    printf("Maximum number of tracks exceeded, cannot store\n");
    return;
  }

  int nStoredHits = 0;
  int nStoredTracks = 0;
  int nStoredLocalTracks = 0;

  GPUTPCSliceOutTrack* out = mOutput->FirstTrack();

  trackSortData* trackOrder = new trackSortData[mCommonMem->nTracks];
  for (unsigned int i = 0; i < mCommonMem->nTracks; i++) {
    trackOrder[i].fTtrack = i;
    trackOrder[i].fSortVal = mTracks[trackOrder[i].fTtrack].NHits() / 1000.f + mTracks[trackOrder[i].fTtrack].Param().GetZ() * 100.f + mTracks[trackOrder[i].fTtrack].Param().GetY();
  }
  std::sort(trackOrder, trackOrder + mCommonMem->nLocalTracks, SortComparison<trackSortData>);
  std::sort(trackOrder + mCommonMem->nLocalTracks, trackOrder + mCommonMem->nTracks, SortComparison<trackSortData>);

  for (unsigned int iTrTmp = 0; iTrTmp < mCommonMem->nTracks; iTrTmp++) {
    const int iTr = trackOrder[iTrTmp].fTtrack;
    GPUTPCTrack& iTrack = mTracks[iTr];

    out->SetParam(iTrack.Param());
    out->SetLocalTrackId(iTrack.LocalTrackId());
    int nClu = 0;
    int iID = iTrack.FirstHitID();

    for (int ith = 0; ith < iTrack.NHits(); ith++) {
      const GPUTPCHitId& ic = mTrackHits[iID + ith];
      int iRow = ic.RowIndex();
      int ih = ic.HitIndex();

      const GPUTPCRow& row = mData.Row(iRow);
#ifdef GPUCA_ARRAY_BOUNDS_CHECKS
      if (ih >= row.NHits() || ih < 0) {
        printf("Array out of bounds access (Sector Row) (Hit %d / %d - NumC %d): Sector %d Row %d Index %d\n", ith, iTrack.NHits(), NHitsTotal(), mISlice, iRow, ih);
        fflush(stdout);
        continue;
      }
#endif
      int clusterIndex = mData.ClusterDataIndex(row, ih);

#ifdef GPUCA_ARRAY_BOUNDS_CHECKS
      if (clusterIndex >= NHitsTotal() || clusterIndex < 0) {
        printf("Array out of bounds access (Cluster Data) (Hit %d / %d - NumC %d): Sector %d Row %d Hit %d, Clusterdata Index %d\n", ith, iTrack.NHits(), NHitsTotal(), mISlice, iRow, ih, clusterIndex);
        fflush(stdout);
        continue;
      }
#endif

      float origX = mData.ClusterData()[clusterIndex].x;
      float origY = mData.ClusterData()[clusterIndex].y;
      float origZ = mData.ClusterData()[clusterIndex].z;
      int id = mData.ClusterData()[clusterIndex].id;
      unsigned char flags = mData.ClusterData()[clusterIndex].flags;
      unsigned short amp = mData.ClusterData()[clusterIndex].amp;
      GPUTPCSliceOutCluster c;
      c.Set(id, iRow, flags, amp, origX, origY, origZ);
#ifdef GPUCA_TPC_RAW_PROPAGATE_PAD_ROW_TIME
      c.pad = mData.ClusterData()[clusterIndex].pad;
      c.time = mData.ClusterData()[clusterIndex].time;
#endif
      out->SetCluster(nClu, c);
      nClu++;
    }

    nStoredTracks++;
    if (iTr < mCommonMem->nLocalTracks) {
      nStoredLocalTracks++;
    }
    nStoredHits += nClu;
    out->SetNClusters(nClu);
    out = out->NextTrack();
  }
  delete[] trackOrder;

  mOutput->SetNTracks(nStoredTracks);
  mOutput->SetNLocalTracks(nStoredLocalTracks);
  mOutput->SetNTrackClusters(nStoredHits);
  if (mCAParam->debugLevel >= 3) {
    printf("Slice %d, Output: Tracks %d, local tracks %d, hits %d\n", mISlice, nStoredTracks, nStoredLocalTracks, nStoredHits);
  }
}

GPUh() int GPUTPCTracker::PerformGlobalTrackingRun(GPUTPCTracker& sliceNeighbour, int iTrack, int rowIndex, float angle, int direction)
{
  /*for (int j = 0;j < mTracks[j].NHits();j++)
  {
    printf("Hit %3d: Row %3d: X %3.7lf Y %3.7lf\n", j, mTrackHits[mTracks[iTrack].FirstHitID() + j].RowIndex(), Row(mTrackHits[mTracks[iTrack].FirstHitID() + j].RowIndex()).X(),
    (float) Data().HitDataY(Row(mTrackHits[mTracks[iTrack].FirstHitID() + j].RowIndex()), mTrackHits[mTracks[iTrack].FirstHitID() + j].HitIndex()) * Row(mTrackHits[mTracks[iTrack].FirstHitID() + j].RowIndex()).HstepY() + Row(mTrackHits[mTracks[iTrack].FirstHitID() + j].RowIndex()).Grid().YMin());
  }*/

  if (sliceNeighbour.mCommonMem->nTracklets == 0) {
    return (0);
  }

  GPUTPCTrackParam tParam;
  tParam.InitParam();
  tParam.SetCov(0, 0.05);
  tParam.SetCov(2, 0.05);
  tParam.SetCov(5, 0.001);
  tParam.SetCov(9, 0.001);
  tParam.SetCov(14, 0.05);
  tParam.SetParam(mTracks[iTrack].Param());

  // printf("Parameters X %f Y %f Z %f SinPhi %f DzDs %f QPt %f SignCosPhi %f\n", tParam.X(), tParam.Y(), tParam.Z(), tParam.SinPhi(), tParam.DzDs(), tParam.QPt(), tParam.SignCosPhi());
  if (!tParam.Rotate(angle, GPUCA_MAX_SIN_PHI)) {
    return (0);
  }
  // printf("Rotated X %f Y %f Z %f SinPhi %f DzDs %f QPt %f SignCosPhi %f\n", tParam.X(), tParam.Y(), tParam.Z(), tParam.SinPhi(), tParam.DzDs(), tParam.QPt(), tParam.SignCosPhi());

  int maxRowGap = 10;
  GPUTPCTrackLinearisation t0(tParam);
  do {
    rowIndex += direction;
    if (!tParam.TransportToX(sliceNeighbour.Row(rowIndex).X(), t0, mCAParam->ConstBz, GPUCA_MAX_SIN_PHI)) {
      return (0); // Reuse t0 linearization until we are in the next sector
    }
    // printf("Transported X %f Y %f Z %f SinPhi %f DzDs %f QPt %f SignCosPhi %f (MaxY %f)\n", tParam.X(), tParam.Y(), tParam.Z(), tParam.SinPhi(), tParam.DzDs(), tParam.QPt(), tParam.SignCosPhi(), sliceNeighbour.Row(rowIndex).MaxY());
    if (--maxRowGap == 0) {
      return (0);
    }
  } while (fabsf(tParam.Y()) > sliceNeighbour.Row(rowIndex).MaxY());

  float err2Y, err2Z;
  GetErrors2(rowIndex, tParam.Z(), tParam.SinPhi(), tParam.DzDs(), err2Y, err2Z);
  if (tParam.GetCov(0) < err2Y) {
    tParam.SetCov(0, err2Y);
  }
  if (tParam.GetCov(2) < err2Z) {
    tParam.SetCov(2, err2Z);
  }

  int nHits = GPUTPCTrackletConstructor::GPUTPCTrackletConstructorGlobalTracking(sliceNeighbour, tParam, rowIndex, direction, 0);
  if (nHits >= GPUCA_GLOBAL_TRACKING_MIN_HITS) {
    // printf("%d hits found\n", nHits);
    unsigned int hitId = CAMath::AtomicAdd(&sliceNeighbour.mCommonMem->nTrackHits, nHits);
    if ((hitId + nHits) >= mNMaxTrackHits) {
      mCommonMem->kernelError = GPUCA_ERROR_GLOBAL_TRACKING_TRACK_HIT_OVERFLOW;
      CAMath::AtomicExch(NTrackHits(), mNMaxTrackHits);
      return (0);
    }
    unsigned int trackId = CAMath::AtomicAdd(&sliceNeighbour.mCommonMem->nTracks, 1);
    if (direction == 1) {
      int i = 0;
      while (i < nHits) {
#ifdef GPUCA_EXTERN_ROW_HITS
        const calink rowHit = sliceNeighbour.TrackletRowHits()[rowIndex * *sliceNeighbour.NTracklets()];
#else
        const calink rowHit = sliceNeighbour.Tracklet(0).RowHit(rowIndex);
#endif
        if (rowHit != CALINK_INVAL) {
          // printf("New track: entry %d, row %d, hitindex %d\n", i, rowIndex, sliceNeighbour.mTrackletRowHits[rowIndex * sliceNeighbour.mCommonMem->nTracklets]);
          sliceNeighbour.mTrackHits[hitId + i].Set(rowIndex, rowHit);
          // if (i == 0) tParam.TransportToX(sliceNeighbour.Row(rowIndex).X(), mCAParam->ConstBz(), GPUCA_MAX_SIN_PHI); //Use transport with new linearisation, we have changed the track in between - NOT needed, fitting will always start at outer end of global track!
          i++;
        }
        rowIndex++;
      }
    } else {
      int i = nHits - 1;
      while (i >= 0) {
#ifdef GPUCA_EXTERN_ROW_HITS
        const calink rowHit = sliceNeighbour.TrackletRowHits()[rowIndex * *sliceNeighbour.NTracklets()];
#else
        const calink rowHit = sliceNeighbour.Tracklet(0).RowHit(rowIndex);
#endif
        if (rowHit != CALINK_INVAL) {
          // printf("New track: entry %d, row %d, hitindex %d\n", i, rowIndex, sliceNeighbour.mTrackletRowHits[rowIndex * sliceNeighbour.mCommonMem->nTracklets]);
          sliceNeighbour.mTrackHits[hitId + i].Set(rowIndex, rowHit);
          i--;
        }
        rowIndex--;
      }
    }
    GPUTPCTrack& track = sliceNeighbour.mTracks[trackId];
    track.SetAlive(1);
    track.SetParam(tParam.GetParam());
    track.SetNHits(nHits);
    track.SetFirstHitID(hitId);
    track.SetLocalTrackId((mISlice << 24) | mTracks[iTrack].LocalTrackId());
  }

  return (nHits >= GPUCA_GLOBAL_TRACKING_MIN_HITS);
}

GPUh() void GPUTPCTracker::PerformGlobalTracking(GPUTPCTracker& sliceLeft, GPUTPCTracker& sliceRight)
{
  int ul = 0, ur = 0, ll = 0, lr = 0;

  int nTrkLeft = sliceLeft.mCommonMem->nTracklets, nTrkRight = sliceRight.mCommonMem->nTracklets;
  sliceLeft.mCommonMem->nTracklets = sliceRight.mCommonMem->nTracklets = 1;
  GPUTPCTracklet *trkLeft = sliceLeft.mTracklets, *trkRight = sliceRight.mTracklets;
  sliceLeft.mTracklets = sliceRight.mTracklets = new GPUTPCTracklet;
#ifdef GPUCA_EXTERN_ROW_HITS
  calink *lnkLeft = sliceLeft.mTrackletRowHits, *lnkRight = sliceRight.mTrackletRowHits;
  sliceLeft.mTrackletRowHits = sliceRight.mTrackletRowHits = new calink[GPUCA_ROW_COUNT];
#endif

  for (int i = 0; i < mCommonMem->nLocalTracks; i++) {
    {
      const int tmpHit = mTracks[i].FirstHitID();
      if (mTrackHits[tmpHit].RowIndex() >= GPUCA_GLOBAL_TRACKING_MIN_ROWS && mTrackHits[tmpHit].RowIndex() < GPUCA_GLOBAL_TRACKING_RANGE) {
        int rowIndex = mTrackHits[tmpHit].RowIndex();
        const GPUTPCRow& row = Row(rowIndex);
        float Y = (float)Data().HitDataY(row, mTrackHits[tmpHit].HitIndex()) * row.HstepY() + row.Grid().YMin();
        if (sliceLeft.NHitsTotal() < 1) {
        } else if (sliceLeft.mCommonMem->nTracks >= sliceLeft.mNMaxTracks) {
          mCommonMem->kernelError = GPUCA_ERROR_GLOBAL_TRACKING_TRACK_OVERFLOW;
          return;
        } else if (Y < -row.MaxY() * GPUCA_GLOBAL_TRACKING_Y_RANGE_LOWER_LEFT) {
          // printf("Track %d, lower row %d, left border (%f of %f)\n", i, mTrackHits[tmpHit].RowIndex(), Y, -row.MaxY());
          ll += PerformGlobalTrackingRun(sliceLeft, i, rowIndex, -mCAParam->DAlpha, -1);
        }
        if (sliceRight.NHitsTotal() < 1) {
        } else if (sliceRight.mCommonMem->nTracks >= sliceRight.mNMaxTracks) {
          mCommonMem->kernelError = GPUCA_ERROR_GLOBAL_TRACKING_TRACK_OVERFLOW;
          return;
        } else if (Y > row.MaxY() * GPUCA_GLOBAL_TRACKING_Y_RANGE_LOWER_RIGHT) {
          // printf("Track %d, lower row %d, right border (%f of %f)\n", i, mTrackHits[tmpHit].RowIndex(), Y, row.MaxY());
          lr += PerformGlobalTrackingRun(sliceRight, i, rowIndex, mCAParam->DAlpha, -1);
        }
      }
    }

    {
      const int tmpHit = mTracks[i].FirstHitID() + mTracks[i].NHits() - 1;
      if (mTrackHits[tmpHit].RowIndex() < GPUCA_ROW_COUNT - GPUCA_GLOBAL_TRACKING_MIN_ROWS && mTrackHits[tmpHit].RowIndex() >= GPUCA_ROW_COUNT - GPUCA_GLOBAL_TRACKING_RANGE) {
        int rowIndex = mTrackHits[tmpHit].RowIndex();
        const GPUTPCRow& row = Row(rowIndex);
        float Y = (float)Data().HitDataY(row, mTrackHits[tmpHit].HitIndex()) * row.HstepY() + row.Grid().YMin();
        if (sliceLeft.NHitsTotal() < 1) {
        } else if (sliceLeft.mCommonMem->nTracks >= sliceLeft.mNMaxTracks) {
          mCommonMem->kernelError = GPUCA_ERROR_GLOBAL_TRACKING_TRACK_OVERFLOW;
          return;
        } else if (Y < -row.MaxY() * GPUCA_GLOBAL_TRACKING_Y_RANGE_UPPER_LEFT) {
          // printf("Track %d, upper row %d, left border (%f of %f)\n", i, mTrackHits[tmpHit].RowIndex(), Y, -row.MaxY());
          ul += PerformGlobalTrackingRun(sliceLeft, i, rowIndex, -mCAParam->DAlpha, 1);
        }
        if (sliceRight.NHitsTotal() < 1) {
        } else if (sliceRight.mCommonMem->nTracks >= sliceRight.mNMaxTracks) {
          mCommonMem->kernelError = GPUCA_ERROR_GLOBAL_TRACKING_TRACK_OVERFLOW;
          return;
        } else if (Y > row.MaxY() * GPUCA_GLOBAL_TRACKING_Y_RANGE_UPPER_RIGHT) {
          // printf("Track %d, upper row %d, right border (%f of %f)\n", i, mTrackHits[tmpHit].RowIndex(), Y, row.MaxY());
          ur += PerformGlobalTrackingRun(sliceRight, i, rowIndex, mCAParam->DAlpha, 1);
        }
      }
    }
  }

  sliceLeft.mCommonMem->nTracklets = nTrkLeft;
  sliceRight.mCommonMem->nTracklets = nTrkRight;
  delete sliceLeft.mTracklets;
  sliceLeft.mTracklets = trkLeft;
  sliceRight.mTracklets = trkRight;
#ifdef GPUCA_EXTERN_ROW_HITS
  delete[] sliceLeft.mTrackletRowHits;
  sliceLeft.mTrackletRowHits = lnkLeft;
  sliceRight.mTrackletRowHits = lnkRight;
#endif
  // printf("Global Tracking Result: Slide %2d: LL %3d LR %3d UL %3d UR %3d\n", mCAParam->ISlice(), ll, lr, ul, ur);
}

#endif
