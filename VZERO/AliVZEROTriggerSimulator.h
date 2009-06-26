#ifndef ALIVZEROTRIGGERSIMULATOR_H
#define ALIVZEROTRIGGERSIMULATOR_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights
 * reserved. 
 *
 * See cxx source for full Copyright notice                               
 */

#include <TObject.h>

#include "AliVZEROTriggerData.h"

class AliVZEROLogicalSignal;
class TTree;
class TClonesArray;

class AliVZEROTriggerSimulator : public TObject {
public:
	AliVZEROTriggerSimulator();
	AliVZEROTriggerSimulator(TTree * digitsTree, TClonesArray* digits);
	~AliVZEROTriggerSimulator();
	AliVZEROTriggerSimulator(const AliVZEROTriggerSimulator &triggerSim);
	AliVZEROTriggerSimulator& operator= (const AliVZEROTriggerSimulator &triggerSim);
	
	AliVZEROTriggerData * GetTriggerData() const {return fTriggerData;};
	
	Bool_t GetBBAandBBC() const		{return (fTriggerWord & 0x1);};
	Bool_t GetBBAorBBC() const		{return (fTriggerWord>>1 & 0x1);};
	Bool_t GetBGAandBBC() const		{return (fTriggerWord>>2 & 0x1);};
	Bool_t GetBGA() const			{return (fTriggerWord>>3 & 0x1);};
	Bool_t GetBGCandBBA() const		{return (fTriggerWord>>4 & 0x1);};
	Bool_t GetBGC() const			{return (fTriggerWord>>5 & 0x1);};
	Bool_t GetCTA1andCTC1() const	{return (fTriggerWord>>6 & 0x1);};
	Bool_t GetCTA1orCTC1() const	{return (fTriggerWord>>7 & 0x1);};
	Bool_t GetCTA2andCTC2() const	{return (fTriggerWord>>8 & 0x1);};
	Bool_t GetCTA2orCTC2() const	{return (fTriggerWord>>9 & 0x1);};
	Bool_t GetMTAandMTC() const		{return (fTriggerWord>>10 & 0x1);};
	Bool_t GetMTAorMTC() const		{return (fTriggerWord>>11 & 0x1);};
	Bool_t GetBBA() const			{return (fTriggerWord>>12 & 0x1);};
	Bool_t GetBBC() const			{return (fTriggerWord>>13 & 0x1);};
	Bool_t GetBGAorBGC() const		{return (fTriggerWord>>14 & 0x1);};
	Bool_t GetBeamGas() const		{return (fTriggerWord>>15 & 0x1);};
	
	void SetBBAandBBC()		{ (fTriggerWord += 0x1);};
	void SetBBAorBBC()		{ (fTriggerWord += 0x1<<1);};
	void SetBGAandBBC()		{ (fTriggerWord += 0x1<<2);};
	void SetBGA()			{ (fTriggerWord += 0x1<<3);};
	void SetBGCandBBA()		{ (fTriggerWord += 0x1<<4);};
	void SetBGC()			{ (fTriggerWord += 0x1<<5);};
	void SetCTA1andCTC1()	{ (fTriggerWord += 0x1<<6);};
	void SetCTA1orCTC1()	{ (fTriggerWord += 0x1<<7);};
	void SetCTA2andCTC2()	{ (fTriggerWord += 0x1<<8);};
	void SetCTA2orCTC2()	{ (fTriggerWord += 0x1<<9);};
	void SetMTAandMTC()		{ (fTriggerWord += 0x1<<10);};
	void SetMTAorMTC()		{ (fTriggerWord += 0x1<<11);};	
	void SetBBA()			{ (fTriggerWord += 0x1<<12);};
	void SetBBC()			{ (fTriggerWord += 0x1<<13);};
	void SetBGAorBGC()		{ (fTriggerWord += 0x1<<14);};
	void SetBeamGas()		{ (fTriggerWord += 0x1<<15);};
	
	void Run();
	
private:
	// Private methods
	AliVZEROTriggerData * LoadTriggerData() const ;
	void GenerateBBWindows();
	void GenerateBGWindows();
	
	// Members
	AliVZEROLogicalSignal * fBBGate[AliVZEROTriggerData::kNCIUBoards];
	AliVZEROLogicalSignal * fBBLatch[AliVZEROTriggerData::kNCIUBoards];
	AliVZEROLogicalSignal * fBBReset[AliVZEROTriggerData::kNCIUBoards];
	
	AliVZEROLogicalSignal * fBGGate[AliVZEROTriggerData::kNCIUBoards];
	AliVZEROLogicalSignal * fBGLatch[AliVZEROTriggerData::kNCIUBoards];
	AliVZEROLogicalSignal * fBGReset[AliVZEROTriggerData::kNCIUBoards];

	AliVZEROTriggerData *fTriggerData; // Object holding the trigger configuration parameters
	
	TTree* fDigitsTree;
	TClonesArray* fDigits;
	
	Bool_t fBBFlags[64];
	Bool_t fBGFlags[64];
	Int_t  fCharges[64];
	
	UShort_t fTriggerWord;
		
	ClassDef( AliVZEROTriggerSimulator, 1 )  

};


#endif // ALIVZEROTRIGGERSIMULATOR_H
