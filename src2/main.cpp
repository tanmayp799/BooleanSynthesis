#include "Logger.h"
#include "Parser.h"

#include "helper.h"

Logger* globalLogger = new Logger();


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
    int numNewInputs = origDqbf->GetDepVars().size();
    finalFormula->addInputs(numNewInputs);
    finalFormula->negateOutput();
    int hCount = 1;
    for(auto p:outputToAig){
        p.second->addInputs(numNewInputs);
        p.second->generateDef(p.first, origDqbf->GetNumInputs() + hCount);
        hCount++;
    }

    AigWrapper* skolemFunctions = new AigWrapper();
    skolemFunctions->addInputs(origDqbf->GetNumInputs()+numNewInputs);

    for(auto p:outputToAig){
        finalFormula->merge(p.second);
        skolemFunctions->merge(p.second);
    }

    for(auto p:outputToAig){
        delete p.second;
    }



    return 0;
}