#ifndef ALIPHOSV1_H
#define ALIPHOSV1_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

//_________________________________________________________________________
// Implementation version v1 of PHOS Manager class 
// Layout EMC + CPV  has name IHEP
//*--                  
//*-- Author: Yves Schutz (SUBATECH)

// --- ROOT system ---
class TClonesArray ;
class TLorentzVector ;
class TFile;

// --- AliRoot header files ---
#include "AliPHOSv0.h"
class AliPHOSQAIntCheckable ; 
class AliPHOSQAFloatCheckable ; 

class AliPHOSv1 : public AliPHOSv0 {

public:

  AliPHOSv1(void) ;
  AliPHOSv1(const char *name, const char *title="") ;
  AliPHOSv1(AliPHOSv1 & phos) : AliPHOSv0(phos) {
    phos.Copy(*this) ; 
  }
  virtual ~AliPHOSv1(void) ;

  virtual void   Copy(AliPHOSv1 & phos) ; 
  virtual void   AddHit( Int_t shunt, Int_t primary, Int_t id, Float_t *hits) ; 
  virtual void   FinishEvent() ;
  virtual void   FinishPrimary() ;
  virtual Int_t  IsVersion(void) const {
    // Gives the version number 
    return 1 ; 
  }

  virtual void   StepManager(void) ;                              
  virtual const TString Version(void)const { return TString("v1") ;  }

  AliPHOSv1 & operator = (const AliPHOSv1 & /*rvalue*/)  {
    // assignement operator requested by coding convention but not needed
    Fatal("operator =", "not implemented") ;
    return *this ; 
  }

  void       CPVDigitize (TLorentzVector p, Float_t *xy, TClonesArray *digits) ;
  Float_t    CPVPadResponseFunction(Float_t qhit, Float_t zg, Float_t xg) ;
  Double_t   CPVCumulPadResponse(Double_t x, Double_t y) ;

  //Variables conserning light yeild and APD efficiency
  Float_t GetLightYieldMean()         const { return  fLightYieldMean ;}
  Float_t GetLightYieldAttenuation()  const { return  fLightYieldAttenuation ;}
  Float_t GetRecalibrationFactor()    const { return  fRecalibrationFactor ;}
  Float_t GetAPDGain()                const { return  fAPDGain ;}
  Float_t GetIntrinsicPINEfficiency() const { return  fIntrinsicPINEfficiency ;}
  Float_t GetElectronsPerGeV()        const { return  fElectronsPerGeV ;}

  void    SetLightYieldMean(Float_t LightYieldMean) 
                                   {fLightYieldMean = LightYieldMean;}
  void    SetLightYieldAttenuation(Float_t LightYieldAttenuation)
                                   {fLightYieldAttenuation = LightYieldAttenuation;}
  void    SetIntrinsicPINEfficiency(Float_t IntrinsicPINEfficiency) 
                                   {fIntrinsicPINEfficiency = IntrinsicPINEfficiency;}
  void    SetRecalibrationFactor(Float_t RecalibrationFactor) 
                                   {fRecalibrationFactor = RecalibrationFactor;}
  void    SetElectronsPerGeV(Float_t ElectronsPerGeV) 
                                   {fElectronsPerGeV = ElectronsPerGeV;}
  void    SetAPDGain(Float_t APDGain)   {fAPDGain = APDGain;}

protected:

  AliPHOSQAIntCheckable   * fQAHitsMul ;  // QA Hits Multiplicity checkable
  TClonesArray * fQAHitsMulB ;            // QA Hits Multiplicity / Block checkable
  AliPHOSQAFloatCheckable * fQATotEner ;  // QA Total Energy checkable
  TClonesArray * fQATotEnerB ;            // QA Total Energy / Block checkable

  Float_t fLightYieldMean ;         // Mean lightyield in the PbOW4 xtal per GeV (Poisson distribution)
  Float_t fIntrinsicPINEfficiency ; // Photo efficiency of the PIN diode   
  Float_t fLightYieldAttenuation ;  // Attenuation of the light through the crystal
  Float_t fRecalibrationFactor ;    // Recalibration factor
  Float_t fElectronsPerGeV ;        // Number of electrons per GeV created in the PIN by a ionizing particle
  Float_t fAPDGain ;                // APD Gain
  Float_t fLightFactor ;            //! a calculated factor
  Float_t fAPDFactor ;              //! a calculated factor

  ClassDef(AliPHOSv1,1)  // Implementation of PHOS manager class for layout EMC+PPSD

};

#endif // AliPHOSV1_H
