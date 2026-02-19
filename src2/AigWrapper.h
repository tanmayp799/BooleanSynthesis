#ifndef AIG_WRAPPER_H
#define AIG_WRAPPER_H

#include "Dqbf.h"

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

class AigWrapper {
public:
    AigWrapper();
    ~AigWrapper();
    AigWrapper(Dqbf* dqbf);
    AigWrapper(KissatWrapper* kw);

    Aig_Man_t* getManager();

    std::vector<Aig_Obj_t*> merge(AigWrapper* aw); //returns the outputs of new AIG

    void generateDef(int outputVar, int hVar);

    int getNumInputs();

    void addInputs(int numInputs);
    void negateOutput();

private:
    Aig_Man_t* manager;
};



#endif // "AIG_WRAPPER_H"