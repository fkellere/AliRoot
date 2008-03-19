#ifndef AliTPCCALIBTRACKSCUTS_H
#define AliTPCCALIBTRACKSCUTS_H

/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

//////////////////////////////////////////////////////
//                                                  //
//     Class to specify cuts for track analysis     //
//     with AliTPCcalibTracks                       //
//                                                  //
//////////////////////////////////////////////////////



#include <TNamed.h>
#include <TObjString.h>

class TChain;

using namespace std;

class AliTPCcalibTracksCuts: public TNamed {

public:
//    AliTPCcalibTracksCuts(Int_t minClusters = 20, Float_t minRatio = 0.4, Float_t max1pt = 0.5,
//       Float_t edgeXZCutNoise = 0.13, Float_t edgeThetaCutNoise = 0.018, char* outputFileName = "Output.root");
   AliTPCcalibTracksCuts(Int_t minClusters, Float_t minRatio, Float_t max1pt,
      Float_t edgeXZCutNoise, Float_t edgeThetaCutNoise, char* outputFileName = "Output.root");
   AliTPCcalibTracksCuts(AliTPCcalibTracksCuts *cuts);
   AliTPCcalibTracksCuts();
   virtual ~AliTPCcalibTracksCuts();
   static void AddCuts(TChain * chain, char* ctype, char* outputFileName = "Output.root");
   static void AddCuts(TChain * chain, Int_t minClusters, Float_t minRatio, Float_t max1pt,
      Float_t edgeXZCutNoise, Float_t edgeThetaCutNoise, char* outputFileName = "Output.root");
   void SetMinClusters(Int_t minClusters){fMinClusters = minClusters;}
   void SetMinRatio(Float_t minRatio){fMinRatio = minRatio;}
   void SetMax1pt(Float_t max1pt){fMax1pt = max1pt;}
   void SetEdgeXYCutNoise(Float_t edgeCutNoise){fEdgeYXCutNoise = edgeCutNoise;}
   void SetEdgeThetaCutNoise(Float_t edgeCutNoise){fEdgeThetaCutNoise = edgeCutNoise;}
   void SetOuputFileNmae(char *fileName) {fOutputFileName = fileName;}
   Int_t   GetMinClusters() const {return fMinClusters;}
   Float_t GetMinRatio() const {return fMinRatio;}
   Float_t GetMax1pt() const {return fMax1pt;}
   Float_t GetEdgeYXCutNoise() const {return fEdgeYXCutNoise;}
   Float_t GetEdgeThetaCutNoise() const {return fEdgeThetaCutNoise;}
   const char* GetOutputFileName() {return fOutputFileName.String().Data();}
   virtual void Print(Option_t* option = "") const;
   
private:
   Int_t   fMinClusters;         // number of clusters
   Float_t fMinRatio;            // kMinRratio = 0.4
   Float_t fMax1pt;              // kMax1pt = 0.5
   Float_t fEdgeYXCutNoise;      // kEdgeYXCutNoise = 0.13
   Float_t fEdgeThetaCutNoise;   // kEdgeThetaCutNoise = 0.018
   TObjString fOutputFileName;  // filename of outputfile ('Output.root')

protected:         
   ClassDef(AliTPCcalibTracksCuts,1)
};


#endif
