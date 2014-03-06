//$Id$
// Original: AliHLTMerger.cxx,v 1.16 2005/06/14 10:55:21 cvetan 

/**************************************************************************
 * This file is property of and copyright by the ALICE HLT Project        * 
 * ALICE Experiment at CERN, All rights reserved.                         *
 *                                                                        *
 * Primary Authors: Uli Frankenfeld, maintained by                          *
 *                  Matthias Richter <Matthias.Richter@ift.uib.no>        *
 *                  for The ALICE HLT Project.                            *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/** @file   AliHLTTPCMerger.cxx
    @author Uli Frankenfeld, maintained by Matthias Richter
    @date   
    @brief  The HLT TPC merger base class
*/

#include "AliHLTTPCLogging.h"
#include "AliHLTTPCMerger.h"
#include "AliHLTTPCTrack.h"
#include "AliHLTTPCTrackSegmentData.h"
#include "AliHLTTPCTransform.h"
#include "AliHLTTPCTrackArray.h"

#ifdef use_root //use root ntuple for slow merge
#include <TNtuple.h>
#include <TTree.h>
#include <TFile.h>
#endif

#if __GNUC__ >= 3
using namespace std;
#endif

ClassImp(AliHLTTPCMerger)

AliHLTTPCMerger::AliHLTTPCMerger()
  :
  fCurrentTracks(0),
  fSlice(0), // TODO its not quite clear were this member is set
  fVertex(NULL),
  f2Global(false), // TODO: initialization was missing at all, assuming false for the moment
  fMaxY(0.0),
  fMaxZ(0.0),
  fMaxKappa(0.0),
  fMaxPsi(0.0),
  fMaxTgl(0.0),
  fTrackType(0),
  fInTrack(NULL),
  fNIn(0),
  fOutTrack(NULL)
{
  //Default constructor
  fInTrack=0;
  fOutTrack=0;
  fCurrentTracks=0;
  fNIn=0;
}

AliHLTTPCMerger::~AliHLTTPCMerger()
{
  //Destructor
  DeleteArray();
}

void AliHLTTPCMerger::InitMerger(Int_t ntrackarrays,const Char_t *tracktype)
{
  //Used to setup all arrays
  
  if(strcmp(tracktype,"AliHLTTPCTrack")==0) fTrackType='t';
  else if(strcmp(tracktype,"AliHLTTPCConfMapTrack")==0) fTrackType='c';
#ifdef INCLUDE_TPC_HOUGH
  else if(strcmp(tracktype,"AliHLTTPCHoughTrack")==0) fTrackType='h';
#endif
  else
    LOG(AliHLTTPCLog::kError,"AliHLTTPCMerger::AliHLTTPCMerger","Track types")
      <<"Unknown tracktype"<<ENDLOG;
  SetArray(ntrackarrays);
  fCurrentTracks=0;

}

void AliHLTTPCMerger::DeleteArray()
{
  //delete arrays
  for(Int_t i=0; i<fNIn;i++)
    {
      if(!fInTrack[i]) continue;
      delete fInTrack[i];
      fInTrack[i]=0;
    }
  if(fInTrack)
    delete[] fInTrack;
  if(fOutTrack)
    delete fOutTrack;
  fInTrack=0;
  fOutTrack=0;
}

void AliHLTTPCMerger::SetArray(Int_t nin)
{ 
  //set arrays
  DeleteArray();//Make sure arrays are cleaned 
  
  fNIn = nin;
  fInTrack = new AliHLTTPCTrackArray*[fNIn];
  for(Int_t i=0; i<fNIn;i++)
    {
#ifdef INCLUDE_TPC_HOUGH
      if(fTrackType=='h')
	fInTrack[i] = new AliHLTTPCTrackArray("AliHLTTPCHoughTrack");
      else
#endif
	fInTrack[i] = new AliHLTTPCTrackArray("AliHLTTPCTrack");
      
    }
#ifdef INCLUDE_TPC_HOUGH
  if(fTrackType=='h')
    fOutTrack= new AliHLTTPCTrackArray("AliHLTTPCHoughTrack");
  else
#endif
    fOutTrack= new AliHLTTPCTrackArray("AliHLTTPCTrack");
}

void AliHLTTPCMerger::Reset()
{ 
  //reset
  for(Int_t i=0; i<fNIn;i++)
    {
      fInTrack[i]->Reset();
    }
  fOutTrack->Reset();
}

void AliHLTTPCMerger::FillTracks(Int_t ntracks, AliHLTTPCTrackSegmentData* tr)
{
  //Read tracks from shared memory (or memory)
  AliHLTTPCTrackArray *destination = GetInTracks(fCurrentTracks);
  if(Is2Global())
    destination->FillTracksChecked(tr, ntracks, 0, fSlice);
  else
    destination->FillTracksChecked(tr, ntracks, 0);
}

void AliHLTTPCMerger::AddAllTracks()
{ 
  //add all tracks
  for(Int_t i=0; i<GetNIn();i++)
    {
      AliHLTTPCTrackArray *in = GetInTracks(i);
      AliHLTTPCTrackArray *out = GetOutTracks();
      out->AddTracks(in);
    }
}

void AliHLTTPCMerger::SortGlobalTracks(AliHLTTPCTrack **tracks, Int_t ntrack)
{ 
  //sort global tracks
  AliHLTTPCTrack **tmp = new AliHLTTPCTrack*[ntrack]; 
  for(Int_t i=0;i<ntrack;i++) tmp[i] = tracks[i];
  Int_t *t = new Int_t[ntrack];
  for(Int_t i=0;i<ntrack;i++) t[i]=-1;
  
  for(Int_t j=0;j<ntrack;j++)
    {
      Double_t minr=300;
      Int_t    mini=0;
      for(Int_t i=0;i<ntrack;i++)
	{
	  if(!tracks[i]) continue;
	  Double_t rr=pow(tracks[i]->GetFirstPointX(),2)+pow(tracks[i]->GetFirstPointY(),2);
	  Double_t r=sqrt(rr);
	  if(r<minr){
	    minr=r;
	    mini=i;
	  }
	}
      t[j]=mini;
      tracks[mini]=0;
    }
  for(Int_t i=0;i<ntrack;i++) tracks[i] = tmp[t[i]];
  delete[] t;
  delete[] tmp;
}

void AliHLTTPCMerger::SortTracks(AliHLTTPCTrack **tracks, Int_t ntrack) const
{ 
  //sort tracks
  AliHLTTPCTrack **tmp = new  AliHLTTPCTrack*[ntrack];
  for(Int_t i=0;i<ntrack;i++) tmp[i] = tracks[i];
  Int_t *t = new Int_t[ntrack];
  for(Int_t i=0;i<ntrack;i++) t[i]=-1;
  
  for(Int_t j=0;j<ntrack;j++)
    {
      Double_t minx=300; 
      Int_t    mini=0;
      for(Int_t i=0;i<ntrack;i++)
	{
	  if(!tracks[i]) continue;
	  if(tracks[i]->GetFirstPointX()<minx)
	    {
	      minx=tracks[i]->GetFirstPointX();
	      mini=i;
	    }     
	}
      t[j]=mini;  
      tracks[mini]=0;
    }
  for(Int_t i=0;i<ntrack;i++) tracks[i] = tmp[t[i]];
  delete[] t;
  delete[] tmp;
}

void AliHLTTPCMerger::AddTrack(AliHLTTPCTrackArray *mergedtrack,AliHLTTPCTrack *track)
{ 
  // add tracks
  AliHLTTPCTrack *t[1];
  t[0] = track;
  MultiMerge(mergedtrack,t,1);
}

AliHLTTPCTrack * AliHLTTPCMerger::MergeTracks(AliHLTTPCTrackArray *mergedtrack,AliHLTTPCTrack *t0,AliHLTTPCTrack *t1)
{ 
  //merge tracks
  AliHLTTPCTrack *t[2];
  t[0] = t0; 
  t[1] = t1;
  SortTracks(t,2);
  return MultiMerge(mergedtrack,t,2);
}

AliHLTTPCTrack * AliHLTTPCMerger::MultiMerge(AliHLTTPCTrackArray *mergedtracks,AliHLTTPCTrack **tracks, Int_t ntrack)
{
  //multi merge the tracks
  //check npoints
  Int_t nps = 0;
  for(Int_t i=0;i<ntrack;i++)
    {
      nps+=tracks[i]->GetNHits();
    }
  if(nps>AliHLTTPCTransform::GetNRows())
    {
      LOG(AliHLTTPCLog::kWarning,"AliHLTTPCMerger::MultiMerge","Adding Points")
	<<AliHLTTPCLog::kDec<<"Too many Points: "<<nps<<ENDLOG;
      return 0;
    }
  
  //create new track
  AliHLTTPCTrack *newtrack = mergedtracks->NextTrack();
  //copy points
  UInt_t * nn = new UInt_t[AliHLTTPCTransform::GetNRows()];
  nps = 0;
  
  //  for(Int_t i=0;i<ntrack;i++){
  for(Int_t i=ntrack-1;i>=0;i--)
    {
      memcpy(&nn[nps],tracks[i]->GetHitNumbers(),tracks[i]->GetNHits()*sizeof(UInt_t));
      nps+=tracks[i]->GetNHits();
    }
  AliHLTTPCTrack *tpf=tracks[0];
  AliHLTTPCTrack *tpl=tracks[ntrack-1];
  AliHLTTPCTrack *best = tpf;
  if(tpf->GetNHits()<tpl->GetNHits() && Is2Global())
    best = tpl;//Best means = most points and therefore best fit (in global case)
  
  newtrack->SetNHits(nps);
  newtrack->SetHits(nps,nn);
  newtrack->SetFirstPoint(tpf->GetFirstPointX(),tpf->GetFirstPointY(),tpf->GetFirstPointZ());
  newtrack->SetLastPoint(tpl->GetLastPointX(),tpl->GetLastPointY(),tpl->GetLastPointZ());
  newtrack->SetPt(best->GetPt());
  newtrack->SetPsi(best->GetPsi());
  newtrack->SetTgl(best->GetTgl());
  newtrack->SetCharge(tpf->GetCharge());
  delete [] nn;
  return newtrack;
}

void* AliHLTTPCMerger::GetNtuple(char *varlist) const
{ 
  //get ntuple
#ifdef use_root
  TNtuple* nt = new TNtuple("ntuple","ntuple",varlist);
  return (void*) nt;
#else
  return 0;
#endif
}

void* AliHLTTPCMerger::GetNtuple() const
{ 
  //get ntuple
#ifdef use_root
  TNtuple* nt = new TNtuple("ntuple","ntuple",
			    "dx:dy:dz:dk:dpsi:dtgl:dq:disx:disy:disz:dis:n0:n1:diff:drx:dry:drz");
  return (void*) nt;
#else
  return 0;
#endif
}

Bool_t AliHLTTPCMerger::WriteNtuple(char *filename, void* nt) const
{ 
  //write ntuple
#ifdef use_root
  TNtuple *ntuple=(TNtuple *) nt;
  TFile *f = new TFile(filename,"RECREATE");
  ntuple->Write();
  f->Close();
  delete ntuple; 
  return kTRUE; 
#else
  return kFALSE;
#endif
}

void AliHLTTPCMerger::FillNtuple(void *nt,AliHLTTPCTrack *innertrack,AliHLTTPCTrack *outertrack)
{ 
  //fill ntuple
  Float_t data[17];
  if(outertrack->IsPoint()&&innertrack->IsPoint())
    {
      data[0] =Float_t(innertrack->GetPointX()-outertrack->GetPointX());
      data[1] =Float_t(innertrack->GetPointY()-outertrack->GetPointY());
      data[2] =Float_t(innertrack->GetPointZ()-outertrack->GetPointZ());
      data[3] =Float_t(innertrack->GetKappa()-outertrack->GetKappa());
      Double_t psi= innertrack->GetPointPsi() - outertrack->GetPointPsi();
      if(psi>AliHLTTPCTransform::Pi()) psi-=AliHLTTPCTransform::TwoPi();
      else if(psi<-AliHLTTPCTransform::Pi()) psi+=AliHLTTPCTransform::TwoPi();
      data[4] =Float_t(psi);
      data[5] =Float_t(innertrack->GetTgl()-outertrack->GetTgl());
      data[6] =Float_t(innertrack->GetCharge()-outertrack->GetCharge());
      data[7] =Float_t(innertrack->GetLastPointX()-outertrack->GetFirstPointX());
      data[8] =Float_t(innertrack->GetLastPointY()-outertrack->GetFirstPointY());
      data[9] =Float_t(innertrack->GetLastPointZ()-outertrack->GetFirstPointZ());
      data[10] =sqrt(pow(data[7],2)+pow(data[8],2)+pow(data[9],2));
      data[11]= outertrack->GetNHits();
      data[12]= innertrack->GetNHits();
      data[13] = Float_t(TrackDiff(innertrack,outertrack));
      data[14]=0;
      data[15]=0;
      data[16]=0;
#ifdef use_root
      TNtuple *ntuple = (TNtuple *) nt;
      ntuple->Fill(data);
#endif
    }
}

void AliHLTTPCMerger::FillNtuple(void *nt,Float_t *data) const
{ 
  //fill ntuple
#ifdef use_root
  TNtuple *ntuple = (TNtuple *) nt;
  ntuple->Fill(data);
#endif
}

Double_t AliHLTTPCMerger::GetAngle(Double_t a1,Double_t a2)
{ 
  //get angle
  Double_t da = a1 - a2 + 4*AliHLTTPCTransform::Pi();
  da = fmod(da,AliHLTTPCTransform::TwoPi());
  if(da>AliHLTTPCTransform::Pi()) da = AliHLTTPCTransform::TwoPi()-da;
  return da;
}

void AliHLTTPCMerger::SetParameter(Double_t maxy, Double_t maxz, Double_t maxkappa, Double_t maxpsi, Double_t maxtgl)
{ 
  //set parameters for merger
  fMaxY = maxy;
  fMaxZ = maxz;
  fMaxKappa = maxkappa;
  fMaxPsi = maxpsi;
  fMaxTgl = maxtgl;
}

Bool_t AliHLTTPCMerger::IsTrack(AliHLTTPCTrack *innertrack,AliHLTTPCTrack *outertrack)
{
  //is track to be merged
  if(innertrack->GetCharge()!=outertrack->GetCharge()) return kFALSE;
  if( (!innertrack->IsPoint()) || (!outertrack->IsPoint()) )  return kFALSE; 
  if(innertrack->GetNHits()+outertrack->GetNHits()>AliHLTTPCTransform::GetNRows()) return kFALSE;
  
  if(fabs(innertrack->GetPointY()-outertrack->GetPointY()) >fMaxY) return kFALSE;
  if(fabs(innertrack->GetPointZ()-outertrack->GetPointZ()) >fMaxZ) return kFALSE;
  if(fabs(innertrack->GetKappa()-outertrack->GetKappa())   >fMaxKappa) return kFALSE;
  if(GetAngle(innertrack->GetPointPsi(),outertrack->GetPointPsi()) >fMaxPsi) return kFALSE;
  if(fabs(innertrack->GetTgl()-outertrack->GetTgl()) >fMaxTgl) return kFALSE;
  //if no rejection up to this point: merge!!
  return kTRUE;
}

Bool_t AliHLTTPCMerger::IsRTrack(AliHLTTPCTrack *innertrack,AliHLTTPCTrack *outertrack)
{ 
  //same as IsTrack
  return IsTrack(innertrack,outertrack);
}

Double_t AliHLTTPCMerger::TrackDiff(AliHLTTPCTrack *innertrack,AliHLTTPCTrack *outertrack)
{ 
  //return track difference
  Double_t diff =-1;
  Double_t x[4],y[4],z[4],dy[4],dz[4];
  AliHLTTPCTrack *tracks[2]; 
  
  tracks[0] = innertrack;
  tracks[1] = outertrack;
  SortGlobalTracks(tracks,2);
  innertrack = tracks[0]; 
  outertrack = tracks[1];
  
  x[0] = innertrack->GetFirstPointX();
  x[1] = innertrack->GetLastPointX();
  x[2] = outertrack->GetFirstPointX();
  x[3] = outertrack->GetLastPointX();
  
  y[0] = innertrack->GetFirstPointY();
  y[1] = innertrack->GetLastPointY();
  y[2] = outertrack->GetFirstPointY();
  y[3] = outertrack->GetLastPointY();

  z[0] = innertrack->GetFirstPointZ();
  z[1] = innertrack->GetLastPointZ();
  z[2] = outertrack->GetFirstPointZ();
  z[3] = outertrack->GetLastPointZ();

  
  outertrack->CalculatePoint(x[0]);
  if(!outertrack->IsPoint()) return diff;
  dy[0] = fabs(y[0] - outertrack->GetPointY());
  dz[0] = fabs(z[0] - outertrack->GetPointZ());
  
  outertrack->CalculatePoint(x[1]);
  if(!outertrack->IsPoint()) return diff;
  dy[1] = fabs(y[1] - outertrack->GetPointY());
  dz[1] = fabs(z[1] - outertrack->GetPointZ());
  
  innertrack->CalculatePoint(x[2]);
  if(!innertrack->IsPoint()) return diff;
  dy[2] = fabs(y[2] - innertrack->GetPointY());
  dz[2] = fabs(z[2] - innertrack->GetPointZ());
  
  innertrack->CalculatePoint(x[3]);
  if(!innertrack->IsPoint()) return diff;
  dy[3] = fabs(y[3] - innertrack->GetPointY());
  dz[3] = fabs(z[3] - innertrack->GetPointZ());

  diff=0;
  for(Int_t i=0;i<4;i++)
    diff+=sqrt(dy[i]*dy[i]+dz[i]*dz[i]);
  return diff; 
}

void AliHLTTPCMerger::PrintDiff(AliHLTTPCTrack *innertrack,AliHLTTPCTrack *outertrack)
{ 
  // print difference
  if(!innertrack->IsPoint()||!outertrack->IsPoint())
    {
      LOG(AliHLTTPCLog::kInformational,"AliHLTTPCMerger::PrintDiff","No Points")<<ENDLOG;
      //cerr<<"AliHLTTPCMerger::PrintDiff: No Points"<<endl;
      //cerr<<"---------------------------"<<endl;
      return;
    } 
  
  Double_t dx = innertrack->GetPointX()-outertrack->GetPointX();
  Double_t dy = innertrack->GetPointY()-outertrack->GetPointY();
  Double_t dz = innertrack->GetPointZ()-outertrack->GetPointZ();
  Double_t dk = innertrack->GetKappa()-outertrack->GetKappa();
  Double_t dpsi= innertrack->GetPointPsi() - outertrack->GetPointPsi();
  if(dpsi>AliHLTTPCTransform::Pi()) dpsi-=AliHLTTPCTransform::TwoPi();
  else if(dpsi<-AliHLTTPCTransform::Pi())dpsi+=AliHLTTPCTransform::TwoPi();
  //Double_t dpsi = GetAngle(innertrack->GetPointPsi(),outertrack->GetPointPsi());
  Double_t dtgl= innertrack->GetTgl()-outertrack->GetTgl();
  Double_t dq =innertrack->GetCharge()-outertrack->GetCharge();
  
  LOG(AliHLTTPCLog::kInformational,"AliHLTTPCMerger::PrintDiff","Points") <<"dx: "<<dx<<" dy: "<<dy<<" dz: "<<dz<<" dk: "<<dk<<" dpsi: "<<dpsi<<" dtgl: "<<dtgl<<" dq: "<<dq<<ENDLOG;
  //fprintf(stderr,"dx: %4f dy: %4f dz: %4f dk: %4f dpsi: %4f dtgl: %4f dq: %4f\n",dx,dy,dz,dk,dpsi,dtgl,dq);
  //cerr<<"---------------------------"<<endl;
  
}

void AliHLTTPCMerger::Print()
{
  // print some infos
  for(Int_t i=0; i<fNIn; i++)
    {
      AliHLTTPCTrackArray *ttt= GetInTracks(i);
      for(Int_t j =0;j<ttt->GetNTracks();j++)
	{
	  AliHLTTPCTrack *track=ttt->GetCheckedTrack(j);
	  if(!track) continue;
	  track->CalculateHelix();
	  //      Double_t angle = atan2(track->GetLastPointY(),track->GetLastPointX());
	  //      if(angle<0) angle+=AliHLTTPCTransform::Pi();
	  if(track->CalculatePoint(135))
	    //      if(!track->CalculateEdgePoint(angle)) cerr<<"**************"<<endl;     
	    //      if(track->CalculatePoint(track->GetLastPointX()))
	    //      if(track->CalculatePoint(0))
	    {
	      //      PrintTrack(track);
	      //      track->CalculateReferencePoint(AliHLTTPCTransform::Pi()/180.);
	      track->CalculateReferencePoint(0.001);
	      Float_t dx=(float)track->GetPointX()-track->GetPointX();
	      Float_t dy=(float)track->GetPointY()-track->GetPointY();
	      Float_t dz=(float)track->GetPointZ()-track->GetPointZ();
	      LOG(AliHLTTPCLog::kInformational,"AliHLTTPCMerger::Print","RefPoint") <<"npt: "<<track->GetNHits()<<" dx: "<<dx<<" dy: "<<dy<<" dz: "<<dz<<ENDLOG;
	      
	      //fprintf(stderr,"npt: %3d dx: %8.5f dy: %8.5f dz: %8.5f\n",track->GetNHits(),dx,dy,dz);
	      //cerr<<"---------------------------"<<endl;
	    }
	}  
    }
}

void AliHLTTPCMerger::PrintTrack(AliHLTTPCTrack *track)
{ 
  //print track info
  fprintf(stderr,"npt: %3d pt: %.2f psi: %.2f tgl: %5.2f q: %2d\n",
	  track->GetNHits(),track->GetPt(),track->GetPsi(),
	  track->GetTgl(),track->GetCharge());
  fprintf(stderr,
	  "x1: %6.2f y1: %6.2f z1: %6.2f xl: %6.2f yl: %6.2f zl: %6.2f\n",
	  track->GetFirstPointX(),track->GetFirstPointY(),track->GetFirstPointZ(),
	  track->GetLastPointX(),track->GetLastPointY(),track->GetLastPointZ());
  if(track->IsPoint())
    {
      fprintf(stderr,
	      "R: %.2f Xc: %.2f Yc: %.2f Xp: %.2f Yp: %.2f Zp: %.2f Psip: %.2f\n",
	      track->GetRadius(),track->GetCenterX(),track->GetCenterY(),
	      track->GetPointX(),track->GetPointY(),track->GetPointZ(),
	      track->GetPointPsi());
    }
}