#ifndef HELPER_H
#define HELPER_H

// #include "Dqbf.h"

// #include "AigWrapper.h"
#include "CadicalWrapper.h"
// #include "cadical.hpp"


extern bool didManthan;

std::vector<KissatWrapper*> generateLocalSpecs(Dqbf* origDqbf);


// extern AigWrapper* skolemAig;

int cegis(Dqbf* origDqbf, CadicalWrapper* solverWrapper, CadicalWrapper* unsatCoreWrapper, CadicalWrapper* constraintWrapper, std::map<int,int> exToHMapping);

int verify(Dqbf* origDqbf, char* argv[]);

#endif // "HELPER_H"