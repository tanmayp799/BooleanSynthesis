#include "Logger.h"
#include "Parser.h"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include "helper.h"
// #include "ScopedTimer.h"

std::string g_argv2;
bool didManthan=false;

int numOrigInputs=0;

AigWrapper* callBFSS(std::vector<int>& varsToEliminate, int target_d, std::string verilogFile, AigWrapper* wrapper, std::string suffix){
    MEASURE_TIME(fmt::format("callBFSS execution for target_d={} (suffix: {})", target_d, suffix),target_d, LogLevel::ERROR);
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
    if (argc > 2) {
        g_argv2 = argv[1];
    }
    

    
    MEASURE_TIME("main", -1, LogLevel::ERROR);
    statisticsLogger.setOutputFile(argv[2]);
    Abc_Start();
    // globalLogger.setOutputFile("./main2_test.log");
    globalLogger.log(LogLevel::INFO, "Starting the program...");
    Parser* fileParser = new Parser(argc, argv);

    if(fileParser->argc == 4) didManthan=true;

    globalLogger.log(LogLevel::INFO, "Parsing the file...");

    Dqbf* origDqbf = fileParser->ParseDqbf();

    // globalLogger.log(LogLevel::INFO,"Generating Local Specs...");
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

    numOrigInputs = finalFormula->getNumInputs();
    // AigWrapper* nnfFormula = getMonotonicCircuit(finalFormula);




    // Aig_Man_t* origFormula = finalFormula->getManager();


    // // CadicalWrapper* testwrap = new CadicalWrapper(finalFormula);
    // // testwrap->dump("./testf.txt");
    // // exit(1);
    // if(didManthan){
    //     globalLogger.log(LogLevel::INFO, "Plugging in Skolem functions generated by Manthan.");
    //     Abc_Ntk_t* defNtk = Io_ReadVerilog(fileParser->argv[2],0);
    //     Abc_Ntk_t* defLogicNtk = Abc_NtkToLogic(defNtk);
    //     Abc_Ntk_t* defStrashNtk = Abc_NtkStrash(defLogicNtk,0,1,0);
    //     Abc_NtkDelete(defNtk);
    //     Abc_NtkDelete(defLogicNtk);

    //     defNtk = defStrashNtk;
    //     Aig_Man_t* eDefMan = ABC_NAMESPACE::Abc_NtkToDar(defNtk, 0, 0);
    //     Abc_NtkDelete(defNtk);
    //     while(Aig_ManCiNum(eDefMan) < Aig_ManCiNum(origFormula)){
    //         Aig_ObjCreateCi(eDefMan);
    //     }

    //     std::ifstream f(fileParser->argv[3]);
    //     std::vector<int> inputMapping;
    //     std::vector<int> outputMapping;
    //     std::string str;

    //     if(!f){
    //         globalLogger.log(LogLevel::ERROR, "Failed to open Manthan mapping file.");
    //         exit(1);
    //     }

    //     if(getline(f,str)){
    //         std::stringstream ss(str);
    //         int num;
    //         while(ss>>num){
    //             inputMapping.push_back(num);
    //         }  
    //     }

    //     if(getline(f,str)){
    //         std::stringstream ss(str);
    //         int num;
    //         while(ss>>num){
    //             outputMapping.push_back(num);
    //         }  
    //     }

    //     std::vector<int> ordering(inputMapping.begin(), inputMapping.end());
    //     for(auto n:outputMapping){
    //         ordering.push_back(n);
    //     }

    //     eDefMan = remapInputs(eDefMan, ordering);
    //     assert(outputMapping.size() == Aig_ManCoNum(eDefMan));
    //     int numOut = Aig_ManCoNum(eDefMan);

    //     Abc_Ntk_t* origFormulaNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(origFormula);
    //     Abc_Ntk_t* eDefNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(eDefMan);
    //     Aig_ManStop(origFormula);
    //     Aig_ManStop(eDefMan);
        
    //     Abc_NtkAppend(origFormulaNtk, eDefNtk, 1);
    //     origFormula = ABC_NAMESPACE::Abc_NtkToDar(origFormulaNtk, 0, 0);
    //     Abc_NtkDelete(origFormulaNtk);
    //     Abc_NtkDelete(eDefNtk);

    //     // std::map<int, Aig_Obj_t*> existential_outputDriver_map;
    //     // for(int i=0;i<numOut;i++){
    //     //     existential_outputDriver_map[outputMapping[i]] = Aig_ManCo(origFormula,i+1);
    //     // }


    //     std::vector<int> varIds = outputMapping;
    //     std::vector<Aig_Obj_t*> funcIds;
    //     for(int i=0;i<numOut;i++){
    //         funcIds.push_back(Aig_ManCo(origFormula,i+1));
    //     }

    //     Aig_Obj_t* newDriver = Aig_SubstituteVec(origFormula,Aig_ManCo(origFormula,0),varIds,funcIds);
    //     Aig_ObjCreateCo(origFormula, newDriver);

    //     int numOutToDelete = Aig_ManCoNum(origFormula);
    //     for(int i=0;i<numOutToDelete-1;i++){
    //         Aig_ObjDisconnect(origFormula,Aig_ManCo(origFormula,i));
    //         Aig_ObjConnect(origFormula, Aig_ManCo(origFormula,i),Aig_ManConst0(origFormula), NULL);
    //     }

    //     Aig_ManCoCleanup(origFormula);
    //     Aig_ManCleanup(origFormula);
    //     if(Aig_ManCoNum(origFormula)==0){
    //         Aig_ObjCreateCo(origFormula, Aig_ManConst0(origFormula));
    //     }


    //     finalFormula->SetManager(origFormula);
    //     finalFormula->compress();
    //     // Aig_Obj_t* outObj = Aig_ManCo(origFormula,0);
    // }



    // finalFormula->ShowAig();

    // finalFormula->DumpVerilog("final_formula.v");

    // printf("Dumped normal way, trying frame way\n");
    // sleep(10);
    std::string verilogFile = "./testFolder/final_formula_with_frame.v";
    finalFormula->DumpVerilogWithFrame(verilogFile);




    std::set<int> existentials = origDqbf->GetExistentials();

    std::vector<int> exisVarsToEliminate;
    for(auto e:existentials){
        exisVarsToEliminate.push_back(e-1);
    
    }

    globalLogger.log(LogLevel::INFO, fmt::format("Eliminating existentials: [{}]", fmt::join(exisVarsToEliminate, " ")));


    std::vector<std::pair<int, AigWrapper*>> tseitinSkolems;

    if(!existentials.empty()){

        DdManager* ddMan;
        DdNode* FddNode;
        Abc_Ntk_t* pNtk;

        getBDD(finalFormula, ddMan, FddNode, pNtk);

        
        Nnf_Man nnfNew;
        nnfNew.init(ddMan, FddNode);

        assert(nnfNew.isWDNNF()==true);
        Aig_Man_t* SAig = nnfNew.createAigWithoutClouds();
        

        // exit(1);
        tseitinSkolems = getTseitinSkolems(SAig,exisVarsToEliminate);

        AigWrapper* newFinalFormula = quantify(pNtk, ddMan,FddNode, exisVarsToEliminate);

        // AigWrapper* tmpwrap=new AigWrapper();
        // tmpwrap->SetManager(SAig);
        // tmpwrap->compress();

        // // Aig_ManShow(SAig,0,NULL);
        // // int yy;
        // // std::cin>>yy;
        // tmpwrap->ShowAig();
        // exit(1);
        // AigWrapper* newFinal = finalFormula->quantify(exisVarsToEliminate, 1, tseitinSkolems);
        delete finalFormula;
        finalFormula = newFinalFormula;
    }

    // AigWrapper* origBenchmark = new AigWrapper(finalFormula);

    // origBenchmark->ShowAig();

    // if(!existentials.empty()){

    //     std::vector<int> existentialsToEliminate(existentials.begin(),existentials.end());
    //     std::sort(existentialsToEliminate.begin(), existentialsToEliminate.end());
    //     printf("Calling bfss on existentials\n");
    //     AigWrapper* exisSkolem = callBFSS(existentialsToEliminate, 0, verilogFile, finalFormula, "_e");

    //     finalFormula->substituteSkolem(exisSkolem, 0, "_e");
    //     finalFormula->compress();
    //     // finalFormula->compress();
    //     // finalFormula->ShowAig();
    //     verilogFile = "./testFolder/final_formula_with_frame_modified.v";
    //     finalFormula->DumpVerilogWithFrame(verilogFile);

    // }

    std::set<int> depVars = origDqbf->GetDepVars();
    std::set<int> universals = origDqbf->GetUniversals();
    std::vector<AigWrapper*> finalSkolems;
    
    // for(auto e:existentials){
    //     depVars.insert(e);
    // }
    // finalFormula->ShowAig();
    
    for(int target_d : depVars) {
        std::vector<int> existentialVarsToEliminate;
        
        // 1. Add all 'e' variables to elimination list
        // for(int e : existentials) {
        //     varsToEliminate.push_back(e);
        // }
        
        // 2. Add all 'd' variables EXCEPT the target_d
        for(int d : depVars) {
            if(d != target_d) {
                existentialVarsToEliminate.push_back(d-1);
            }
        }



        std::sort(existentialVarsToEliminate.begin(), existentialVarsToEliminate.end());
        
        // AigWrapper* skolemAig=nullptr;
        // if(!varsToEliminate.empty()){
        //     skolemAig = callBFSS(varsToEliminate, target_d, verilogFile, finalFormula,"_d");
        // }


        // AigWrapper* localSpec = new AigWrapper(finalFormula);
        // if(skolemAig!=nullptr) localSpec->substituteSkolem(skolemAig, target_d, "_d");
        

        // localSpec->negateOutput();

        std::vector<int> universalVarsToEliminate;

        std::set<int> dependency_set = origDqbf->GetDependencySet(target_d);
        for(auto e:universals){
            if(dependency_set.find(e)==dependency_set.end()){
                universalVarsToEliminate.push_back(e-1);
            }
        }
        std::sort(universalVarsToEliminate.begin(), universalVarsToEliminate.end());

        std::vector<int> printExistential;
        for (int e : existentialVarsToEliminate) printExistential.push_back(e + 1);
        std::vector<int> printUniversal;
        for (int e : universalVarsToEliminate) printUniversal.push_back(e + 1);

        globalLogger.log(LogLevel::INFO,fmt::format("Generating localSpec for target_d={}: eliminating existentials [{}] and universals [{}]", 
            target_d, fmt::join(printExistential, " "), fmt::join(printUniversal, " ")));
        AigWrapper* localSpec = finalFormula->getLocalSpec(target_d, existentialVarsToEliminate, universalVarsToEliminate);

        // finalFormula->ShowAig();
        
        // std::string verilogFile2 = "./testFolder/localSpec_"+std::to_string(target_d)+".v";
        // localSpec->DumpVerilogWithFrame(verilogFile2);


        // AigWrapper* universalSkolemAig=nullptr;
        // if(!universalVarsToEliminate.empty()){
        //     universalSkolemAig = callBFSS(universalVarsToEliminate, target_d, verilogFile2, localSpec, "_u");
        // }

        // if(universalSkolemAig!=nullptr) localSpec->substituteSkolem(universalSkolemAig, target_d, "_u");

        // // std
        // localSpec->negateOutput();

        // localSpec->compress();

        printf("Final localSpec\n");
        // localSpec->ShowAig();

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



    // Aig_Man_t* origFormula = finalFormula->getManager();


    // // CadicalWrapper* testwrap = new CadicalWrapper(finalFormula);
    // // testwrap->dump("./testf.txt");
    // // exit(1);
    // if(didManthan){
    //     globalLogger.log(LogLevel::INFO, "Plugging in Skolem functions generated by Manthan.");
    //     Abc_Ntk_t* defNtk = Io_ReadVerilog(fileParser->argv[2],0);
    //     Abc_Ntk_t* defLogicNtk = Abc_NtkToLogic(defNtk);
    //     Abc_Ntk_t* defStrashNtk = Abc_NtkStrash(defLogicNtk,0,1,0);
    //     Abc_NtkDelete(defNtk);
    //     Abc_NtkDelete(defLogicNtk);

    //     defNtk = defStrashNtk;
    //     Aig_Man_t* eDefMan = ABC_NAMESPACE::Abc_NtkToDar(defNtk, 0, 0);
    //     Abc_NtkDelete(defNtk);
    //     while(Aig_ManCiNum(eDefMan) < Aig_ManCiNum(origFormula)){
    //         Aig_ObjCreateCi(eDefMan);
    //     }

    //     std::ifstream f(fileParser->argv[3]);
    //     std::vector<int> inputMapping;
    //     std::vector<int> outputMapping;
    //     std::string str;

    //     if(!f){
    //         globalLogger.log(LogLevel::ERROR, "Failed to open Manthan mapping file.");
    //         exit(1);
    //     }

    //     if(getline(f,str)){
    //         std::stringstream ss(str);
    //         int num;
    //         while(ss>>num){
    //             inputMapping.push_back(num);
    //         }  
    //     }

    //     if(getline(f,str)){
    //         std::stringstream ss(str);
    //         int num;
    //         while(ss>>num){
    //             outputMapping.push_back(num);
    //         }  
    //     }

    //     std::vector<int> ordering(inputMapping.begin(), inputMapping.end());
    //     for(auto n:outputMapping){
    //         ordering.push_back(n);
    //     }

    //     eDefMan = remapInputs(eDefMan, ordering);
    //     assert(outputMapping.size() == Aig_ManCoNum(eDefMan));
    //     int numOut = Aig_ManCoNum(eDefMan);

    //     Abc_Ntk_t* origFormulaNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(origFormula);
    //     Abc_Ntk_t* eDefNtk = ABC_NAMESPACE::Abc_NtkFromAigPhase(eDefMan);
    //     Aig_ManStop(origFormula);
    //     Aig_ManStop(eDefMan);
        
    //     Abc_NtkAppend(origFormulaNtk, eDefNtk, 1);
    //     origFormula = ABC_NAMESPACE::Abc_NtkToDar(origFormulaNtk, 0, 0);
    //     Abc_NtkDelete(origFormulaNtk);
    //     Abc_NtkDelete(eDefNtk);

    //     // std::map<int, Aig_Obj_t*> existential_outputDriver_map;
    //     // for(int i=0;i<numOut;i++){
    //     //     existential_outputDriver_map[outputMapping[i]] = Aig_ManCo(origFormula,i+1);
    //     // }


    //     std::vector<int> varIds = outputMapping;
    //     std::vector<Aig_Obj_t*> funcIds;
    //     for(int i=0;i<numOut;i++){
    //         funcIds.push_back(Aig_ManCo(origFormula,i+1));
    //     }

    //     Aig_Obj_t* newDriver = Aig_SubstituteVec(origFormula,Aig_ManCo(origFormula,0),varIds,funcIds);
    //     Aig_ObjCreateCo(origFormula, newDriver);

    //     int numOutToDelete = Aig_ManCoNum(origFormula);
    //     for(int i=0;i<numOutToDelete-1;i++){
    //         Aig_ObjDisconnect(origFormula,Aig_ManCo(origFormula,i));
    //         Aig_ObjConnect(origFormula, Aig_ManCo(origFormula,i),Aig_ManConst0(origFormula), NULL);
    //     }

    //     Aig_ManCoCleanup(origFormula);
    //     Aig_ManCleanup(origFormula);
    //     if(Aig_ManCoNum(origFormula)==0){
    //         Aig_ObjCreateCo(origFormula, Aig_ManConst0(origFormula));
    //     }


    //     finalFormula->SetManager(origFormula);
    //     finalFormula->compress();
    //     // Aig_Obj_t* outObj = Aig_ManCo(origFormula,0);
    // }


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
    // AigWrapper* origBenchmark = new AigWrapper(finalFormula); 
    AigWrapper* unsatCoreFormula = new AigWrapper(finalFormula);
    int numNewInputs = depVars.size();
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
        // finalFormula->ShowAig();
        exToHMapping[p.first] = origDqbf->GetNumInputs() + hCount;
        hCount++;
    }

    // for(auto p:exToHMapping){
    //     globalLogger.log(LogLevel::INFO, fmt::format("Ex: {} -> H: {}", p.first, p.second));
    // }

    // // AigWrapper* skolemFunctions = new AigWrapper();
    // // skolemFunctions->addInputs(origDqbf->GetNumInputs()+numNewInputs);

    for(auto p:outputToAig){
        // globalLogger.log(LogLevel::INFO, fmt::format("Merging AIG for id: {}", p.first));
        // finalFormula->ShowAig();

        // globalLogger.log(LogLevel::INFO, "Showing definiton");
        // p.second->compress();
        // p.second->ShowAig();

        finalFormula->merge(p.second);
        
        // skolemFunctions->merge(p.second);
        // globalLogger.log(LogLevel::INFO, "After merge");
        // finalFormula->ShowAig();
        unsatCoreFormula->merge(p.second);
    }
    // finalFormula->ShowAig();
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

    int res;
    if(!depVars.empty()) res = cegis(origDqbf, solverWrapper, unsatCoreWrapper, constraintWrapper, exToHMapping);
    else res = 0;
    if(res==1){
        globalLogger.log(LogLevel::INFO, "No Solution Exists.");
        exit(20);

    }
    else{
        globalLogger.log(LogLevel::INFO, "Solution Exists.");
    }

    // origBenchmark->ShowAig();
    AigWrapper* origBenchmark = new AigWrapper(origDqbf);
    int res2 =verify(origBenchmark, origDqbf, tseitinSkolems, fileParser->argv);

    if(res2==20){
        globalLogger.log(LogLevel::INFO, "Verification complete: Check Passed!");
        // dumpAigerSkolems(tseitinSkolems);
    }
    else{
        globalLogger.log(LogLevel::INFO, "Verification complete: Check Failed!");
        
    }

    Abc_Stop();


    if(res) return 1;
    return 0;
}