#include "helper.h"

int cegis(Dqbf* origDqbf, CadicalWrapper* solverWrapper, CadicalWrapper* unsatCoreWrapper, CadicalWrapper* constraintWrapper, std::map<int,int> exToHMapping){

    int numInputs = origDqbf->GetNumInputs() + origDqbf->GetDepVars().size();
    
    std::set<int> deps = origDqbf->GetDepVars();

    for(auto id:deps){
        int hId = exToHMapping[id];

        solverWrapper->setDefaultValue(hId, 1);
    }

    

}