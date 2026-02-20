#ifndef AIG_WRAPPER_H
#define AIG_WRAPPER_H

#include "Dqbf.h"

#define clause ABC_clause
extern "C" {
#include "misc/util/abc_global.h"
#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "base/cmd/cmd.h"
#include "base/abc/abc.h"
#include "misc/nm/nmInt.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "sat/bsat/satSolver.h"
#include "sat/bsat/satSolver2.h"
#include "opt/mfs/mfs.h"
#include "opt/mfs/mfsInt.h"
#include "bool/kit/kit.h"
#include "bdd/cudd/cuddInt.h"
#include "bdd/cudd/cudd.h"
}
#undef clause




namespace ABC_NAMESPACE{
extern "C" {
	Aig_Man_t * Abc_NtkToDar(Abc_Ntk_t * pNtk, int fExors, int fRegisters);
	Abc_Ntk_t * Abc_NtkFromAigPhase(Aig_Man_t * pMan);
	Vec_Ptr_t * Io_FileReadCnf( char * pFileName, int fMulti );
	void Aig_ManAppend( Aig_Man_t * pBase, Aig_Man_t * pNew );
	Abc_Ntk_t * Abc_NtkDarToCnf( Abc_Ntk_t * pNtk, char * pFileName, int fFastAlgo, int fChangePol, int fVerbose );
	
}
}



extern std::map<int, std::pair<Abc_Ntk_t*, Abc_Ntk_t*>> varToBasisMap;

class AigWrapper {
public:
    // AigWrapper();
    ~AigWrapper();
    AigWrapper(Dqbf* dqbf);
    AigWrapper(KissatWrapper* kw);

    Aig_Man_t* getManager();

    void merge(AigWrapper* aw); //returns the outputs of new AIG

    void generateDef(int outputVar, int hVar);

    int getNumInputs();

    void addInputs(int numInputs);
    void negateOutput();

    Abc_Ntk_t* getNtk();

private:
    Aig_Man_t* manager;
};



#endif // "AIG_WRAPPER_H"