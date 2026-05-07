#ifndef HELPER_H
#define HELPER_H

// #include "Dqbf.h"

// #include "AigWrapper.h"
#include "CadicalWrapper.h"
#include "nnf.h"
// #include "cadical.hpp"


extern bool didManthan;
extern int numOrigInputs;

std::vector<KissatWrapper*> generateLocalSpecs(Dqbf* origDqbf);


// extern AigWrapper* skolemAig;

int cegis(Dqbf* origDqbf, CadicalWrapper* solverWrapper, CadicalWrapper* unsatCoreWrapper, CadicalWrapper* constraintWrapper, std::map<int,int> exToHMapping);

int verify(AigWrapper* finalFormula, Dqbf* origDqbf, std::vector<std::pair<int, AigWrapper*>>& tseitinSKolems, char* argv[]);

void getBDD(AigWrapper* formula, DdManager* &ddMan, DdNode* &FddNode, Abc_Ntk_t* &pNtk);

AigWrapper* quantify(Abc_Ntk_t* pNtk, DdManager* ddMan, DdNode* FddNode, std::vector<int> &varsToEliminate);

std::vector<std::pair<int, AigWrapper*>> getTseitinSkolems(Aig_Man_t* SAig, std::vector<int> existentialVarsToEliminate);

// AigWrapper* getMonotonicCircuit(AigWrapper* formula);

#endif // "HELPER_H"