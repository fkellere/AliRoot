// XEmacs -*-C++-*-

#ifndef ALIHLTTPCCATRACKERCOMPONENT_H
#define ALIHLTTPCCATRACKERCOMPONENT_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* AliHLTTPCCATrackerComponent
 */

#include "AliHLTProcessor.h"

class AliHLTTPCCATracker;
class AliHLTTPCVertex;

/**
 * @class AliHLTTPCCATrackerComponent
 * The Cellular Automaton tracker component.
 */
class AliHLTTPCCATrackerComponent : public AliHLTProcessor
    {
    public:
      /** standard constructor */
      AliHLTTPCCATrackerComponent();
      /** not a valid copy constructor, defined according to effective C++ style */
      AliHLTTPCCATrackerComponent(const AliHLTTPCCATrackerComponent&);
      /** not a valid assignment op, but defined according to effective C++ style */
      AliHLTTPCCATrackerComponent& operator=(const AliHLTTPCCATrackerComponent&);
      /** standard destructor */
      virtual ~AliHLTTPCCATrackerComponent();
      
      // Public functions to implement AliHLTComponent's interface.
      // These functions are required for the registration process
      
      /** @see component interface @ref AliHLTComponent::GetComponentID */
      const char* GetComponentID();
      
      /** @see component interface @ref AliHLTComponent::GetInputDataTypes */
      void GetInputDataTypes( vector<AliHLTComponentDataType>& list);

      /** @see component interface @ref AliHLTComponent::GetOutputDataType */
      AliHLTComponentDataType GetOutputDataType();

      /** @see component interface @ref AliHLTComponent::GetOutputDataSize */
      virtual void GetOutputDataSize( unsigned long& constBase, double& inputMultiplier );

      /** @see component interface @ref AliHLTComponent::Spawn */
      AliHLTComponent* Spawn();

    protected:

	// Protected functions to implement AliHLTComponent's interface.
	// These functions provide initialization as well as the actual processing
	// capabilities of the component. 

      /** @see component interface @ref AliHLTComponent::DoInit */
	int DoInit( int argc, const char** argv );

      /** @see component interface @ref AliHLTComponent::DoDeinit */
	int DoDeinit();

      /** @see component interface @ref AliHLTProcessor::DoEvent */
	int DoEvent( const AliHLTComponentEventData& evtData, const AliHLTComponentBlockData* blocks, 
		     AliHLTComponentTriggerData& trigData, AliHLTUInt8_t* outputPtr, 
		     AliHLTUInt32_t& size, vector<AliHLTComponentBlockData>& outputBlocks );
	
    private:

      /** the tracker object */
      AliHLTTPCCATracker* fTracker;                                //! transient
      /** the virtexer object */
      AliHLTTPCVertex* fVertex;                                    //! transient

      /** magnetic field */
      Double_t fBField;                                            // see above

	ClassDef(AliHLTTPCCATrackerComponent, 0)

    };
#endif
