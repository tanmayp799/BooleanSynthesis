#include "Logger.h"
#include "Parser.h"

#include "helper.h"

Logger globalLogger;


int main(int argc, char* argv[]){


    Parser* fileParser = new Parser(argc, argv);
    Dqbf* origDqbf = fileParser->ParseDqbf();

    std::vector<KissatWrapper*> localInitializations = generateLocalSpecs(origDqbf);

    for(auto kw:localInitializations){
        kw->eliminateExistentialVars();
        kw->eliminateUniversalVars();
    }

    std::map<int, AigWrapper*> outputToAig;
    for(auto kw:localInitializations){
        outputToAig[kw->getOutputVar()] = new AigWrapper(kw);
    }

    AigWrapper* finalFormula = new AigWrapper(origDqbf);
    AigWrapper* unsatCoreFormula = new AigWrapper(origDqbf);
    int numNewInputs = origDqbf->GetDepVars().size();
    finalFormula->addInputs(numNewInputs);
    unsatCoreFormula->addInputs(numNewInputs);
    finalFormula->negateOutput();
    int hCount = 1;

    std::map<int, int> exToHMapping;

    for(auto p:outputToAig){
        p.second->addInputs(numNewInputs);
        p.second->generateDef(p.first, origDqbf->GetNumInputs() + hCount);
        exToHMapping[p.first] = origDqbf->GetNumInputs() + hCount;
        hCount++;
    }

    AigWrapper* skolemFunctions = new AigWrapper();
    skolemFunctions->addInputs(origDqbf->GetNumInputs()+numNewInputs);

    for(auto p:outputToAig){
        finalFormula->merge(p.second);
        skolemFunctions->merge(p.second);
        unsatCoreFormula->merge(p.second);
    }

    for(auto p:outputToAig){
        delete p.second;
    }

    CadicalWrapper* solverWrapper = new CadicalWrapper(finalFormula);
    CadicalWrapper* unsatCoreWrapper = new CadicalWrapper(unsatCoreFormula);
    CadicalWrapper* constraintWrapper = new CadicalWrapper();

    cegis(origDqbf, solverWrapper, unsatCoreWrapper, constraintWrapper, exToHMapping);







    return 0;
}