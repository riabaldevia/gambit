//
// FILE: nfgsolng.cc -- definition of the class dealing with the GUI part of
//                      the normal form solutions.
//
// $Id$
//

#include "wx.h"
#include "wx_form.h"
#include "wxmisc.h"
#include "wxstatus.h"
#include "glist.h"
#include "gpoly.h"
#include "nfgsolng.h"
#include "nfgconst.h"
#include "gfunc.h"

// sections in the defaults file(s)
#define     SOLN_SECT               "Soln-Defaults"

//=========================================================================
//                     NfgSolutionG: Member functions
//=========================================================================

NfgSolutionG::NfgSolutionG(const Nfg &p_nfg, const NFSupport &p_support,
			   NfgShowInterface *p_parent)
  : nf(p_nfg), sup(p_support), parent(p_parent)
{ }

//=========================================================================
//                     Derived classes, by algorithm
//=========================================================================

//-----------
// EnumPure
//-----------

#include "nfgpure.h"
#include "purenprm.h"

NfgEnumPureG::NfgEnumPureG(const Nfg &p_nfg, const NFSupport &p_support,
			   NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgEnumPureG::Solve(void) const
{
  wxStatus status(parent->Frame(), "EnumPure Algorithm");
  
  PureNashParamsSettings PNS;
  int stopAfter;
  PNS.GetParams(stopAfter);
  try {
    FindPureNash(sup, stopAfter, status, solns);
  }
  catch (gSignalBreak &) { }
  return solns;
}

bool NfgEnumPureG::SolveSetup(void) const
{
  PureNashSolveParamsDialog PNPD(parent->Frame());
  return (PNPD.Completed() == wxOK);
}

//------------
// EnumMixed
//------------

#include "enum.h"
#include "enumprm.h"

NfgEnumG::NfgEnumG(const Nfg &p_nfg, const NFSupport &p_support,
		   NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgEnumG::Solve(void) const
{
  EnumParamsSettings EPS;
  wxEnumStatus status(parent->Frame());
  EnumParams P(status);
  EPS.GetParams(P);
  long npivots;
  double time;
  Enum(sup, P, solns, npivots, time);
  return solns;
}

bool NfgEnumG::SolveSetup(void) const
{
  EnumSolveParamsDialog ESPD(parent->Frame());
  return (ESPD.Completed() == wxOK);
}

//------------
// LcpSolve
//------------

#include "lemke.h"
#include "lemkeprm.h"

NfgLemkeG::NfgLemkeG(const Nfg &p_nfg, const NFSupport &p_support,
		     NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgLemkeG::Solve(void) const
{
  if (nf.NumPlayers() != 2) {
    wxMessageBox("LCP algorithm only works on 2 player games.",
		 "Algorithm Error");
    return gList<MixedSolution>();
  }

  wxStatus status(parent->Frame(), "LCP Algorithm");
  LemkeParamsSettings LPS;
  LemkeParams P(status);
  LPS.GetParams(P);
  int npivots;
  double time;
  Lemke(sup, P, solns, npivots, time);
  return solns;
}

bool NfgLemkeG::SolveSetup(void) const
{
  LemkeSolveParamsDialog LSPD(parent->Frame());
  return (LSPD.Completed() == wxOK);
}

//----------
// LpSolve
//----------

#include "nfgcsum.h"
#include "csumprm.h"

NfgZSumG::NfgZSumG(const Nfg &p_nfg, const NFSupport &p_support,
		   NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgZSumG::Solve(void) const
{
  if (nf.NumPlayers() > 2 || !IsConstSum(nf)) {
    wxMessageBox("Only valid for two-person zero-sum games");
    return solns;
  }

  wxStatus status(parent->Frame(), "LP Algorithm");
  status << "Progress not implemented\n" << "Cancel button disabled\n";
  LPParamsSettings LPPS;
  ZSumParams P;
  LPPS.GetParams(P);
  int npivots;
  double time;
  ZSum(sup, P, solns, npivots, time);
  return solns;
}

bool NfgZSumG::SolveSetup(void) const
{
  LPSolveParamsDialog LPD(parent->Frame());
  return (LPD.Completed() == wxOK);
}

//--------
// Liap
//--------

#include "nliap.h"
#include "liapprm.h"

NfgLiapG::NfgLiapG(const Nfg &p_nfg, const NFSupport &p_support,
		   NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgLiapG::Solve(void) const
{
  wxStatus status(parent->Frame(), "Liap Algorithm");
  LiapParamsSettings LPS;
  NFLiapParams P(status);
  LPS.GetParams(P);
  MixedProfile<gNumber> start(parent->CreateStartProfile(LPS.StartOption()));
  long nevals, nits;
  Liap(nf, P, start, solns, nevals, nits);
  return solns;
}

bool NfgLiapG::SolveSetup(void) const
{
  LiapSolveParamsDialog LSPD(parent->Frame());
  return (LSPD.Completed() == wxOK);
}

//----------
// Simpdiv
//----------

#include "simpdiv.h"
#include "simpprm.h"

NfgSimpdivG::NfgSimpdivG(const Nfg &p_nfg, const NFSupport &p_support,
			 NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgSimpdivG::Solve(void) const
{
  SimpdivParamsSettings SPS;
  wxStatus status(parent->Frame(), "Simpdiv Algorithm");
  SimpdivParams P(status);
  SPS.GetParams(P);
  int nevals, niters;
  double time;
  Simpdiv(sup, P, solns, nevals, niters, time);
  return solns;
}

bool NfgSimpdivG::SolveSetup(void) const
{
  SimpdivSolveParamsDialog SDPD(parent->Frame());
  return (SDPD.Completed() == wxOK);
}

//----------
// Gobit
//----------

#include "ngobit.h"
#include "gobitprm.h"

NfgGobitG::NfgGobitG(const Nfg &p_nfg, const NFSupport &p_support,
		     NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgGobitG::Solve(void) const
{
  GobitParamsSettings GSPD(parent->Filename());
  wxStatus *status = new wxStatus(parent->Frame(), "QRE Algorithm");
  NFGobitParams P(*status);
  GSPD.GetParams(P);

  MixedProfile<gNumber> start(parent->CreateStartProfile(GSPD.StartOption()));

  long nevals, nits;
  Gobit(nf, P, start, solns, nevals, nits);

  delete status;
  GSPD.RunPxi();
  return solns;
}

bool NfgGobitG::SolveSetup(void) const
{
  GobitSolveParamsDialog GSPD(parent->Frame(), parent->Filename());
  return (GSPD.Completed() == wxOK);
}

//-------------
// GobitGrid
//-------------

#include "grid.h"
#include "gridprm.h"

NfgGobitAllG::NfgGobitAllG(const Nfg &p_nfg, const NFSupport &p_support,
			   NfgShowInterface *p_parent)
  : NfgSolutionG(p_nfg, p_support, p_parent)
{ }

gList<MixedSolution> NfgGobitAllG::Solve(void) const
{
  GridParamsSettings GSPD(parent->Filename());
  wxStatus *status = new wxStatus(parent->Frame(), "QRE Grid Solve");
  GridParams P(*status);
  GSPD.GetParams(P);
  GridSolve(sup, P, solns);
  delete status;
  GSPD.RunPxi();
  return solns;
}

bool NfgGobitAllG::SolveSetup(void) const
{
  GridSolveParamsDialog GSPD(parent->Frame(), parent->Filename());
  return (GSPD.Completed() == wxOK);
}



