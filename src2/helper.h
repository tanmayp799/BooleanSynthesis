#ifndef HELPER_H
#define HELPER_H

// #include "Dqbf.h"

// #include "AigWrapper.h"
#include "CadicalWrapper.h"
// #include "cadical.hpp"

std::vector<KissatWrapper*> generateLocalSpecs(Dqbf* origDqbf);


int cegis(Dqbf* origDqbf, CadicalWrapper* solverWrapper, CadicalWrapper* unsatCoreWrapper, CadicalWrapper* constraintWrapper, std::map<int,int> exToHMapping);

#endif // "HELPER_H"