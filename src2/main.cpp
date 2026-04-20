#include "Logger.h"
#include "Parser.h"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include "helper.h"
#include "ScopedTimer.h"



AigWrapper* callBFSS(std::vector<int>& varsToEliminate, int target_d, std::string verilogFile, AigWrapper* wrapper, std::string suffix){
    MEASURE_TIME(fmt::format("callBFSS execution for target_d={} (suffix: {})", target_d, suffix),LogLevel::ERROR);
    globalLogger.log(LogLevel::ERROR,fmt::format("VarsToEliminate: {}",fmt::join(varsToEliminate," ")));
    std::string elimFileName = "./testFolder/elim_target_d" + std::to_string(target_d) + suffix + ".txt";
        std::ofstream outElim(elimFileName);
        if (outElim.is_open()) {
            Abc_Ntk_t* pNtkLogic = ABC_NAMESPACE::Abc_NtkFromAigPhase(wrapper->getManager());
            Abc_NtkShortNames(pNtkLogic);
            
            for (int var : varsToEliminate) {
                Abc_Obj_t* pPi = Abc_NtkPi(pNtkLogic, var - 1);
                outElim << Abc_ObjName(pPi) << "\n";
            }
            Abc_NtkDelete(pNtkLogic);
            
            outElim.close();
            globalLogger.log(LogLevel::INFO, fmt::format("Dumped elimination list for d_{} to {}", target_d, elimFileName));
        } else {
            globalLogger.log(LogLevel::ERROR, "Failed to open " + elimFileName);
        }

        // Generate paths for the output files
        std::string orderFileName = "./testFolder/order_target_d" + std::to_string(target_d) + suffix + ".txt";
        std::string skolemFileName = "./testFolder/skolem_target_d" + std::to_string(target_d) + suffix + ".v";

        // 2. Generate the variable ordering
        std::string genOrderCmd = "./bin/genVarOrder " + verilogFile + " " + elimFileName + " > " + orderFileName;
        globalLogger.log(LogLevel::INFO, "Executing: " + genOrderCmd);
        int retOrder = std::system(genOrderCmd.c_str());
        if (retOrder != 0) {
            globalLogger.log(LogLevel::ERROR, "genVarOrder failed for d_" + std::to_string(target_d));
        }

        // 3. Run bfss to generate the Skolem functions
        std::string bfssCmd = "./bin/bfss -ae -b " + verilogFile + " -v " + orderFileName + " -o " + skolemFileName;
        globalLogger.log(LogLevel::INFO, "Executing: " + bfssCmd);
        int retBfss = std::system(bfssCmd.c_str());
        if (retBfss != 0) {
            globalLogger.log(LogLevel::ERROR, "bfss failed for d_" + std::to_string(target_d));
        }

        // 4. Load the generated Skolem function back into an AIG
        AigWrapper* skolemAig = new AigWrapper(skolemFileName);
        // skolemAig->ShowAig();

        globalLogger.log(LogLevel::INFO, fmt::format("Loaded Skolem function for d_{} with {} inputs.", target_d, Aig_ManCiNum(skolemAig->getManager())));
        
        int numInitInputs = wrapper->getNumInputs();
        int numParamInputs = skolemAig->getNumInputs() - (numInitInputs-varsToEliminate.size());
        for(int i=0;i<numParamInputs;i++){
            skolemAig->substituteConst(skolemAig->getNumInputs() - i,0);
        }
        // skolemAig->ShowAig();


        return skolemAig;
}


int main(int argc, char* argv[]){

    Abc_Start();
    globalLogger.setOutputFile("./main2_test.log");
    globalLogger.log(LogLevel::INFO, "Starting the program...");
    Parser* fileParser = new Parser(argc, argv);


    globalLogger.log(LogLevel::INFO, "Parsing the file...");

    Dqbf* origDqbf = fileParser->ParseDqbf();

    globalLogger.log(LogLevel::INFO,"Generating Local Specs...");
    // std::vector<KissatWrapper*> localInitializations = generateLocalSpecs(origDqbf);


    // for(auto kw:localInitializations){
    //     globalLogger.log(LogLevel::INFO, fmt::format("Performing Quantifier Elimination for id: {}", kw->getOutputVar()));
    //     kw->eliminateExistentialVars();
    //     // kw->eliminateUniversalVars();
    // }


    // globalLogger.setOutputFile("./statistics/eliminationStatistics.csv");
    // int numY = origDqbf->GetDepVars().size()+origDqbf->GetExistentials().size();
    // for(auto kw:localInitializations){
    //     globalLogger.log(LogLevel::STATS, fmt::format("{},{},{},{}", argv[1],kw->getOutputVar(), numY-1, kw->getEliminatedVars().size()));
    // }

    // globalLogger.closeOutputFile();


    // exit(1);




    std::map<int, AigWrapper*> outputToAig;
    // for(auto kw:localInitializations){
    //     std::cout<<"Generating AIG for id: "<<kw->getOutputVar()<<std::endl;
    //     outputToAig[kw->getOutputVar()] = new AigWrapper(kw);
    // }

    AigWrapper* finalFormula = new AigWrapper(origDqbf);
    // finalFormula->ShowAig();

    // finalFormula->DumpVerilog("final_formula.v");

    // printf("Dumped normal way, trying frame way\n");
    // sleep(10);
    std::string verilogFile = "./testFolder/final_formula_with_frame.v";
    finalFormula->DumpVerilogWithFrame(verilogFile);




    std::set<int> existentials = origDqbf->GetExistentials();

    if(!existentials.empty()){

        std::vector<int> existentialsToEliminate(existentials.begin(),existentials.end());
        std::sort(existentialsToEliminate.begin(), existentialsToEliminate.end());
        printf("Calling bfss on existentials\n");
        AigWrapper* exisSkolem = callBFSS(existentialsToEliminate, 0, verilogFile, finalFormula, "_e");

        finalFormula->substituteSkolem(exisSkolem, 0, "_e");
        finalFormula->compress();
        // finalFormula->compress();
        // finalFormula->ShowAig();
        verilogFile = "./testFolder/final_formula_with_frame_modified.v";
        finalFormula->DumpVerilogWithFrame(verilogFile);

    }

    std::set<int> depVars = origDqbf->GetDepVars();
    std::set<int> universals = origDqbf->GetUniversals();
    std::vector<AigWrapper*> finalSkolems;
    for(int target_d : depVars) {
        std::vector<int> varsToEliminate;
        
        // 1. Add all 'e' variables to elimination list
        // for(int e : existentials) {
        //     varsToEliminate.push_back(e);
        // }
        
        // 2. Add all 'd' variables EXCEPT the target_d
        for(int d : depVars) {
            if(d != target_d) {
                varsToEliminate.push_back(d);
            }
        }
        std::sort(varsToEliminate.begin(), varsToEliminate.end());
        
        AigWrapper* skolemAig=nullptr;
        if(!varsToEliminate.empty()){
            skolemAig = callBFSS(varsToEliminate, target_d, verilogFile, finalFormula,"_d");
        }


        AigWrapper* localSpec = new AigWrapper(finalFormula);
        if(skolemAig!=nullptr) localSpec->substituteSkolem(skolemAig, target_d, "_d");
        

        localSpec->negateOutput();

        std::vector<int> universalVarsToEliminate;

        std::set<int> dependency_set = origDqbf->GetDependencySet(target_d);
        for(auto e:universals){
            if(dependency_set.find(e)==dependency_set.end()){
                universalVarsToEliminate.push_back(e);
            }
        }
        std::sort(universalVarsToEliminate.begin(), universalVarsToEliminate.end());
        
        std::string verilogFile2 = "./testFolder/localSpec_"+std::to_string(target_d)+".v";
        localSpec->DumpVerilogWithFrame(verilogFile2);


        AigWrapper* universalSkolemAig=nullptr;
        if(!universalVarsToEliminate.empty()){
            universalSkolemAig = callBFSS(universalVarsToEliminate, target_d, verilogFile2, localSpec, "_u");
        }

        if(universalSkolemAig!=nullptr) localSpec->substituteSkolem(universalSkolemAig, target_d, "_u");

        // std
        localSpec->negateOutput();

        localSpec->compress();

        printf("Final localSpec\n");
        localSpec->ShowAig();

        // AigWrapper* const0sub= new AigWrapper(localSpec);
        // AigWrapper* const1sub = localSpec;

        // const1sub->substituteConst(target_d,1);
        
        // printf("Const1sub\n");
        // const1sub->ShowAig();

        
        
        // const0sub->substituteConst(target_d,0);
        // const0sub->negateOutput();
        // printf("Const0sub\n");
        // const0sub->ShowAig();

        // const1sub->merge(const0sub);

        // printf("Final skolem\n");
        // localSpec->ShowAig();
        
        outputToAig[target_d] = localSpec;

        finalSkolems.push_back(localSpec);
    
    }

    // finalSub(finalFormula, finalSkolems,depVars);
    // printf("Should be true\n");
    // finalFormula->ShowAig();

    // Abc_Ntk_t* finalNtk = finalFormula->getNtk();
    // for(auto sk:finalSkolems){
    //     Abc_Ntk_t* skNtk = sk->getNtk();
    //     Abc_NtkAppend(finalNtk, skNtk, 1);
    //     Abc_NtkDelete(skNtk);
    // }

    // Aig_Man_t* finalMan = ABC_NAMESPACE::Abc_NtkToDar(finalNtk, 0, 0);
    // Abc_NtkDelete(finalNtk);

    // std::vector<int> varIds(depVars.begin(), depVars.end());
    // std::vector<Aig_Obj_t*> funcIds;
    // int numOuts=Aig_ManCoNum(finalMan);
    // for(int i=1;i<numOuts;i++){
    //     funcIds.push_back(Aig_ManCo(finalMan,i));
    // }

    // Aig_Obj_t* newDriver = Aig_SubstituteVec(finalMan,Aig_ManCo(finalMan,0),varIds,funcIds);
    // Aig_ObjCreateCo(finalMan, newDriver);

    // exit(1);

    // // finalFormula->substituteInputs(origDqbf->GetExistentials(),fileParser->argv[2], fileParser->argv[3]);
    AigWrapper* unsatCoreFormula = new AigWrapper(finalFormula);
    int numNewInputs = origDqbf->GetDepVars().size();
    // numNewInputs+= origDqbf->GetExistentials().size();
    finalFormula->addInputs(numNewInputs);
    unsatCoreFormula->addInputs(numNewInputs);
    finalFormula->negateOutput();
    // globalLogger.log(LogLevel::INFO, "Final Formula:");
    // finalFormula->ShowAig();
    int hCount = 1;


    // exit(1);

    std::map<int, int> exToHMapping;

    for(auto p:outputToAig){
        p.second->addInputs(numNewInputs);
        p.second->generateDef(p.first, origDqbf->GetNumInputs() + hCount);
        globalLogger.log(LogLevel::INFO, fmt::format("Generating Def for id: {}", p.first));
        // p.second->ShowAig();
        exToHMapping[p.first] = origDqbf->GetNumInputs() + hCount;
        hCount++;
    }

    // for(auto p:exToHMapping){
    //     globalLogger.log(LogLevel::INFO, fmt::format("Ex: {} -> H: {}", p.first, p.second));
    // }

    // // AigWrapper* skolemFunctions = new AigWrapper();
    // // skolemFunctions->addInputs(origDqbf->GetNumInputs()+numNewInputs);

    for(auto p:outputToAig){
        finalFormula->merge(p.second);
        // skolemFunctions->merge(p.second);
        unsatCoreFormula->merge(p.second);
    }

    // for(auto p:outputToAig){
    //     delete p.second;
    // }
    // finalFormula->ShowAig();
    // std::cout<<finalFormula->GetNumOutputs()<<std::endl;
    // // finalFormula->ShowAig();
    // finalFormula->ShowAig();

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


    if(res) return 1;
    return 0;
}