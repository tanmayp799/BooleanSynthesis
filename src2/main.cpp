#include "Logger.h"
#include "Parser.h"

#include "helper.h"

int main(int argc, char* argv[]){

    Abc_Start();
    globalLogger.setOutputFile("./main2_test.log");
    globalLogger.log(LogLevel::INFO, "Starting the program...");
    Parser* fileParser = new Parser(argc, argv);


    globalLogger.log(LogLevel::INFO, "Parsing the file...");

    Dqbf* origDqbf = fileParser->ParseDqbf();

    globalLogger.log(LogLevel::INFO,"Generating Local Specs...");
    std::vector<KissatWrapper*> localInitializations = generateLocalSpecs(origDqbf);


    for(auto kw:localInitializations){
        globalLogger.log(LogLevel::INFO, fmt::format("Performing Quantifier Elimination for id: {}", kw->getOutputVar()));
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

    // AigWrapper* skolemFunctions = new AigWrapper();
    // skolemFunctions->addInputs(origDqbf->GetNumInputs()+numNewInputs);

    for(auto p:outputToAig){
        finalFormula->merge(p.second);
        // skolemFunctions->merge(p.second);
        unsatCoreFormula->merge(p.second);
    }

    for(auto p:outputToAig){
        delete p.second;
    }

    CadicalWrapper* solverWrapper = new CadicalWrapper(finalFormula);
    CadicalWrapper* unsatCoreWrapper = new CadicalWrapper(unsatCoreFormula);
    CadicalWrapper* constraintWrapper = new CadicalWrapper();

    int res = cegis(origDqbf, solverWrapper, unsatCoreWrapper, constraintWrapper, exToHMapping);

    if(res==1){
        globalLogger.log(LogLevel::INFO, "No Solution Exists.");
    }
    else{
        globalLogger.log(LogLevel::INFO, "Solution Exists.");
    }

    Abc_Stop();



    return 0;
}