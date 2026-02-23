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
        std::cout<<"Generating AIG for id: "<<kw->getOutputVar()<<std::endl;
        outputToAig[kw->getOutputVar()] = new AigWrapper(kw);
    }

    AigWrapper* finalFormula = new AigWrapper(origDqbf);
    // finalFormula->ShowAig();
    AigWrapper* unsatCoreFormula = new AigWrapper(origDqbf);
    int numNewInputs = origDqbf->GetDepVars().size();
    finalFormula->addInputs(numNewInputs);
    unsatCoreFormula->addInputs(numNewInputs);
    finalFormula->negateOutput();
    globalLogger.log(LogLevel::INFO, "Final Formula:");
    // finalFormula->ShowAig();
    int hCount = 1;

    std::map<int, int> exToHMapping;

    for(auto p:outputToAig){
        p.second->addInputs(numNewInputs);
        p.second->generateDef(p.first, origDqbf->GetNumInputs() + hCount);
        globalLogger.log(LogLevel::INFO, fmt::format("Generating Def for id: {}", p.first));
        // p.second->ShowAig();
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
    finalFormula->ShowAig();

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