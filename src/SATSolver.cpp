#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/SATSolver.h"
#include "dlvhex2/CDNLSolver.h"
#include "dlvhex2/ClaspSolver.h"
#include "dlvhex2/ProgramCtx.h"

DLVHEX_NAMESPACE_BEGIN

SATSolverPtr SATSolver::getInstance(ProgramCtx& ctx, NogoodSet& ns){

	switch (ctx.config.getOption("GenuineSolver")){
	case 1: case 2:	// internal grounder or Gringo + internal solver
		{
		DBGLOG(DBG, "Instantiating genuine sat solver with internal solver");
		SATSolverPtr ptr = SATSolverPtr(new CDNLSolver(ctx, ns));
		return ptr;
		}
		break;
	case 3: case 4:	// internal grounder or Gringo + clasp
#ifdef HAVE_LIBCLASP
		{
		DBGLOG(DBG, "Instantiating genuine sat solver with clasp");
		SATSolverPtr ptr = SATSolverPtr(new ClaspSolver(ctx, ns));
		return ptr;
		}
#else
		throw GeneralError("No support for clasp compiled into this binary");
#endif // HAVE_LIBCLINGO
		break;
	}
}

DLVHEX_NAMESPACE_END

