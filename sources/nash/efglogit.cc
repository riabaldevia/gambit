//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Trace a branch of the agent logit QRE correspondence
//
// This file is part of Gambit
// Copyright (c) 2002, The Gambit Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#include <math.h>
#include "base/gnullstatus.h"
#include "math/gmath.h"
#include "math/gsmatrix.h"
#include "efglogit.h"

//=========================================================================
//             QRE Correspondence Computation via Homotopy
//=========================================================================

//
// The following code implements a homotopy approach to computing
// the logistic QRE correspondence.  This implementation is a basic
// Euler-Newton approach with adaptive step size, based upon the
// ideas and codes presented in Allgower and Georg's
// _Numerical Continuation Methods_.
//
// Many of these functions are duplicated in efgqre.cc and nfgqre.cc.
// This should be fixed in future!
//

inline double sqr(double x) { return x*x; }

static void Givens(gbtMatrix<double> &b, gbtMatrix<double> &q,
		   double &c1, double &c2, int l1, int l2, int l3)
{
  if (fabs(c1) + fabs(c2) == 0.0) {
    return;
  }

  double sn;
  if (fabs(c2) >= fabs(c1)) {
    sn = sqrt(1.0 + sqr(c1/c2)) * fabs(c2);
  }
  else {
    sn = sqrt(1.0 + sqr(c2/c1)) * fabs(c1);
  }
  double s1 = c1/sn;
  double s2 = c2/sn;

  for (int k = 1; k <= q.NumColumns(); k++) {
    double sv1 = q(l1, k);
    double sv2 = q(l2, k);
    q(l1, k) = s1 * sv1 + s2 * sv2;
    q(l2, k) = -s2 * sv1 + s1 * sv2;
  }

  for (int k = l3; k <= b.NumColumns(); k++) {
    double sv1 = b(l1, k);
    double sv2 = b(l2, k);
    b(l1, k) = s1 * sv1 + s2 * sv2;
    b(l2, k) = -s2 * sv1 + s1 * sv2;
  }

  c1 = sn;
  c2 = 0.0;
}

static void QRDecomp(gbtMatrix<double> &b, gbtMatrix<double> &q)
{
  q.MakeIdent();
  for (int m = 1; m <= b.NumColumns(); m++) {
    for (int k = m + 1; k <= b.NumRows(); k++) {
      Givens(b, q, b(m, m), b(k, m), m, k, m + 1);
    }
  }
}

static void NewtonStep(gbtMatrix<double> &q, gbtMatrix<double> &b,
		       gbtVector<double> &u, gbtVector<double> &y,
		       double &d)
{
  for (int k = 1; k <= b.NumColumns(); k++) {
    for (int l = 1; l <= k - 1; l++) {
      y[k] -= b(l, k) * y[l];
    }
    y[k] /= b(k, k);
  }

  d = 0.0;
  for (int k = 1; k <= b.NumRows(); k++) {
    double s = 0.0;
    for (int l = 1; l <= b.NumColumns(); l++) {
      s += q(l, k) * y[l];
    }
    u[k] -= s;
    d += s * s;
  }
  d = sqrt(d);
}

static void QreLHS(const gbtEfgSupport &p_support,
		   const gbtVector<double> &p_point,
		   gbtVector<double> &p_lhs)
{
  gbtBehavProfile<double> profile = p_support->NewBehavProfile(0.0);
  for (int i = 1; i <= profile->BehavProfileLength(); i++) {
    profile[i] = p_point[i];
  }
  double lambda = p_point[p_point.Length()];

  p_lhs = 0.0;
  int rowno = 0;

  for (int pl = 1; pl <= p_support->NumPlayers(); pl++) {
    gbtGamePlayer player = p_support->GetPlayer(pl);
    for (int iset = 1; iset <= player->NumInfosets(); iset++) {
      rowno++;
      for (int act = 1; act <= p_support->NumActions(pl, iset); act++) {
	p_lhs[rowno] += profile(pl, iset, act);
      }
      p_lhs[rowno] -= 1.0;

      for (int act = 2; act <= p_support->NumActions(pl, iset); act++) {
	p_lhs[++rowno] = log(profile(pl, iset, act) / profile(pl, iset, 1));
	p_lhs[rowno] -= (lambda *
			 (profile->GetActionValue(p_support->GetAction(pl, iset, act)) -
			  profile->GetActionValue(p_support->GetAction(pl, iset, 1))));
	p_lhs[rowno] *= profile(pl, iset, 1) * profile(pl, iset, act);
      }
    }
  }
}

static void QreJacobian(const gbtEfgSupport &p_support,
			const gbtVector<double> &p_point,
			gbtMatrix<double> &p_matrix)
{
  gbtBehavProfile<double> profile = p_support->NewBehavProfile(0.0);
  for (int i = 1; i <= profile->BehavProfileLength(); i++) {
    profile[i] = p_point[i];
  }
  double lambda = p_point[p_point.Length()];

  int rowno = 0; 
  for (int pl1 = 1; pl1 <= p_support->NumPlayers(); pl1++) {
    gbtGamePlayer player1 = p_support->GetPlayer(pl1);
    for (int iset1 = 1; iset1 <= player1->NumInfosets(); iset1++) {
      gbtGameInfoset infoset1 = player1->GetInfoset(iset1);
      rowno++;
      // First, do the "sum to one" equation
      int colno = 0;
      for (int pl2 = 1; pl2 <= p_support->NumPlayers(); pl2++) {
	gbtGamePlayer player2 = p_support->GetPlayer(pl2);
	for (int iset2 = 1; iset2 <= player2->NumInfosets(); iset2++) {
	  for (int act2 = 1; act2 <= p_support->NumActions(pl2, iset2); act2++) {
	    colno++;
	    if (pl1 == pl2 && iset1 == iset2) {
	      p_matrix(colno, rowno) = 1.0;
	    }
	    else {
	      p_matrix(colno, rowno) = 0.0;
	    }
	  }
	}
      }
      p_matrix(p_matrix.NumRows(), rowno) = 0.0;
					    
      for (int act1 = 2; act1 <= p_support->NumActions(pl1, iset1); act1++) {
	rowno++;
	int colno = 0;

	for (int pl2 = 1; pl2 <= p_support->NumPlayers(); pl2++) {
	  gbtGamePlayer player2 = p_support->GetPlayer(pl2);
	  for (int iset2 = 1; iset2 <= player2->NumInfosets(); iset2++) {
	    gbtGameInfoset infoset2 = player2->GetInfoset(iset2);

	    for (int act2 = 1; act2 <= p_support->NumActions(pl2, iset2); act2++) {
	      colno++;
	      if (infoset1 == infoset2) {
		if (act2 == 1) {
		  p_matrix(colno, rowno) = -profile(pl1, iset1, act1);
		}
		else if (act1 == act2) {
		  p_matrix(colno, rowno) = profile(pl1, iset1, 1);
		}
		else {
		  p_matrix(colno, rowno) = 0.0;
		}
	      }
	      else {   // infoset1 != infoset2
		if (profile->GetInfosetProb(infoset1) < 1.0e-10) {
		  p_matrix(colno, rowno) = 0;
		}
		else {
		  p_matrix(colno, rowno) = -lambda * profile(pl1, iset1, 1) * profile(pl1, iset1, act1) * (profile->DiffActionValue(p_support->GetAction(pl1, iset1, act1), p_support->GetAction(pl2, iset2, act2)) - profile->DiffActionValue(p_support->GetAction(pl1, iset1, 1), p_support->GetAction(pl2, iset2, act2)));
		}
	      }
	    }
	  }
	}

	p_matrix(p_matrix.NumRows(), rowno) = -profile(pl1, iset1, 1) * profile(pl1, iset1, act1) * (profile->GetActionValue(p_support->GetAction(pl1, iset1, act1)) - profile->GetActionValue(p_support->GetAction(pl1, iset1, 1)));
      }
    }
  }
}

static void TracePath(const gbtBehavProfile<double> &p_start,
		      double p_startLambda, double p_maxLambda, double p_omega,
		      gbtStatus &p_status,
		      gbtBehavNashSet &p_solutions)
{
  const int c_maxIters = 5000;     // maximum number of iterations
  const double c_tol = 1.0e-4;     // tolerance for corrector iteration
  const double c_maxDecel = 1.1;   // maximal deceleration factor
  const double c_maxDist = 0.4;    // maximal distance to curve
  const double c_maxContr = 0.6;   // maximal contraction rate in corrector
  const double c_eta = 0.1;        // perturbation to avoid cancellation
                                   // in calculating contraction rate
  double h = .03;                  // initial stepsize
  const double c_hmin = 1.0e-5;    // minimal stepsize

  gbtVector<double> x(p_start->BehavProfileLength() + 1);
  gbtVector<double> u(p_start->BehavProfileLength() + 1);
  for (int i = 1; i <= p_start->BehavProfileLength(); i++) {
    x[i] = p_start[i];
  }
  x[x.Length()] = p_startLambda;
  gbtVector<double> t(p_start->BehavProfileLength() + 1);
  gbtVector<double> y(p_start->BehavProfileLength());

  gbtMatrix<double> b(p_start->BehavProfileLength() + 1,
		      p_start->BehavProfileLength());
  gbtSquareMatrix<double> q(p_start->BehavProfileLength() + 1);
  QreJacobian(p_start->GetSupport(), x, b);
  QRDecomp(b, q);
  q.GetRow(q.NumRows(), t);
  
  int niters = 0;

  for (int i = 1; i < x.Length(); i++) {
    if (x[i] < 1.0e-10) {
      // Drop this strategy from the support, then recursively call
      // to continue tracing
      gbtEfgSupport newSupport(p_start->GetSupport());
      int index = 1;
      for (int pl = 1; pl <= newSupport->NumPlayers(); pl++) {
	gbtGamePlayer player = newSupport->GetPlayer(pl);
	for (int iset = 1; iset <= player->NumInfosets(); iset++) {
	  for (int act = 1; act <= newSupport->NumActions(pl, iset); act++) {
	    if (index++ == i) {
	      newSupport->RemoveAction(newSupport->GetAction(pl, iset, act));
	    }
	  }
	}
      }
      
      gbtBehavProfile<double> newProfile = newSupport->NewBehavProfile(0.0);
      for (int j = 1; j <= newProfile->BehavProfileLength(); j++) {
	if (j < i) {
	  newProfile[j] = x[j];
	}
	else if (j >= i) {
	  newProfile[j] = x[j+1];
	}
      }

      TracePath(newProfile, x[x.Length()], p_maxLambda, p_omega,
		p_status, p_solutions);
      return;
    }
  }

  while (x[x.Length()] >= 0.0 && x[x.Length()] < p_maxLambda) {
    p_status.Get();
    if (niters > c_maxIters) {
      // Give up
      return;
    }

    if (niters++ % 25 == 0) {
      p_status.SetProgress(x[x.Length()] / p_maxLambda,
			   gbtText("Lambda = ") + ToText(x[x.Length()]));
    }

    bool accept = true;

    if (fabs(h) <= c_hmin) {
      return;
    }

    // Predictor step
    for (int k = 1; k <= x.Length(); k++) {
      u[k] = x[k] + h * p_omega * t[k];
      if (k < x.Length() && u[k] < 0.0) {
	accept = false;
	break;
      }
    }

    if (!accept) {
      h *= 0.5;
      continue;
    }

    double decel = 1.0 / c_maxDecel;  // initialize deceleration factor
    QreJacobian(p_start->GetSupport(), u, b);
    QRDecomp(b, q);

    int iter = 1;
    double disto = 0.0;
    while (true) {
      double dist;

      QreLHS(p_start->GetSupport(), u, y);
      NewtonStep(q, b, u, y, dist); 
      if (dist >= c_maxDist) {
	accept = false;
	break;
      }
      for (int i = 1; i < u.Length(); i++) {
	if (u[i] < 0.0) {
	  // don't go negative
	  accept = false;
	  break;
	}
      }
      if (!accept) {
	break;
      }
      
      decel = gmax(decel, sqrt(dist / c_maxDist) * c_maxDecel);
      if (iter >= 2) {
	double contr = dist / (disto + c_tol * c_eta);
	if (contr > c_maxContr) {
	  accept = false;
	  break;
	}
	decel = gmax(decel, sqrt(contr / c_maxContr) * c_maxDecel);
      }

      if (dist <= c_tol) {
	// Success; break out of iteration
	break;
      }
      disto = dist;
      iter++;
    }

    if (!accept) {
      h /= c_maxDecel;   // PC not accepted; change stepsize and retry
      if (fabs(h) <= c_hmin) {
	return;
      }

      continue;
    }

    // Determine new stepsize
    if (decel > c_maxDecel) {
      decel = c_maxDecel;
    }
    h = fabs(h / decel);

    // PC step was successful; update and iterate
    for (int i = 1; i < x.Length(); i++) {
      if (u[i] < 1.0e-10) {
	// Drop this strategy from the support, then recursively call
	// to continue tracing
	gbtEfgSupport newSupport(p_start->GetSupport());
	int index = 1;
	for (int pl = 1; pl <= newSupport->NumPlayers(); pl++) {
	  gbtGamePlayer player = newSupport->GetPlayer(pl);
	  for (int iset = 1; iset <= player->NumInfosets(); iset++) {
	    for (int act = 1; act <= newSupport->NumActions(pl, iset); act++) {
	      if (index++ == i) {
		newSupport->RemoveAction(newSupport->GetAction(pl, iset, act));
	      }
	    }
	  }
	}

	gbtBehavProfile<double> newProfile = newSupport->NewBehavProfile(0.0);
	for (int j = 1; j <= newProfile->BehavProfileLength(); j++) {
	  if (j < i) {
	    newProfile[j] = u[j];
	  }
	  else if (j >= i) {
	    newProfile[j] = u[j+1];
	  }
	}

	TracePath(newProfile, u[u.Length()], p_maxLambda, p_omega,
		  p_status, p_solutions);
	return;
      }
      else {
	x[i] = u[i];
      }
    }

    x[x.Length()] = u[u.Length()];

    gbtBehavProfile<double> foo(p_start);
    for (int i = 1; i <= foo->BehavProfileLength(); i++) {
      foo[i] = x[i];
    }
    p_solutions.Append(foo->NewBehavProfile(gbtNumber(0)));
    
    gbtVector<double> newT(t);
    q.GetRow(q.NumRows(), newT);  // new tangent
    if (t * newT < 0.0) {
      // Bifurcation detected; for now, just "jump over" and continue,
      // taking into account the change in orientation of the curve.
      // Someday, we need to do more here! :)
      p_omega = -p_omega;
    }
    t = newT;
  }
}

gbtEfgNashLogit::gbtEfgNashLogit(void)
  : m_maxLam(30.0), m_stepSize(0.0001), m_fullGraph(false)
{ }

gbtBehavNashSet gbtEfgNashLogit::Solve(const gbtEfgSupport &p_support,
				       gbtStatus &p_status)
{
  gbtBehavNashSet solutions;
  gbtBehavProfile<double> start = p_support->NewBehavProfile(0.0);

  try {
    TracePath(start, 0.0, m_maxLam, 1.0, p_status, solutions);
  }
  catch (...) { }

  if (!m_fullGraph) { 
    while (solutions.Length() > 1) {
      solutions.Remove(1);
    }
  }

  return solutions;
}
