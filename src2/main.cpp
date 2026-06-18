#include "Logger.h"
#include "Parser.h"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include "helper.h"
#include <csignal>
// #include "ScopedTimer.h"

std::string g_argv2;
bool didManthan=false;

int numOrigInputs=0;

void timeout_handler(int signum) {

    global_metrics.execution_status = "TIMEOUT";

    global_metrics.print_json_metrics();

    std::_Exit(signum);

}



void final_cleanup_hook() {

    

        global_metrics.print_json_metrics();

    

}







int main(int argc, char* argv[]){
    if (argc > 2) {
        g_argv2 = argv[1];
    }
    

    
    MEASURE_TIME("main", -1, LogLevel::ERROR);


        signal(SIGTERM, timeout_handler);

    signal(SIGSEGV, timeout_handler);

    signal(SIGABRT, timeout_handler);

    std::atexit(final_cleanup_hook);
    global_metrics.start_timestamp = std::chrono::high_resolution_clock::now();

    // statisticsLogger.setOutputFile(argv[2]);
    Abc_Start();
    // globalLogger.setOutputFile("./main2_test.log");
    globalLogger.log(LogLevel::INFO, "Starting the program...");
    Parser* fileParser = new Parser(argc, argv);

    if(fileParser->argc == 4) didManthan=true;

    globalLogger.log(LogLevel::INFO, "Parsing the file...");

    Dqbf* origDqbf = fileParser->ParseDqbf();

    global_metrics.benchmark_name = argv[1];
    global_metrics.count_a = origDqbf->GetUniversals().size();
    global_metrics.count_e = origDqbf->GetExistentials().size();
    global_metrics.count_d = origDqbf->GetDepVars().size();

    global_metrics.d_vars = std::vector<int>(global_metrics.count_d,0);
    global_metrics.individual_dep_set_sizes = std::vector<int>(global_metrics.count_d,0);
    global_metrics.individual_exis_quant_times = std::vector<double>(global_metrics.count_d,0.0);
    global_metrics.individual_univ_quant_times = std::vector<double>(global_metrics.count_d,0.0);
    global_metrics.individual_bdd_gen_times = std::vector<double>(global_metrics.count_d,0.0);
    global_metrics.individual_bdd_sizes = std::vector<long long>(global_metrics.count_d,0);
    global_metrics.individual_aig_sizes = std::vector<long long>(global_metrics.count_d,0);
    global_metrics.is_trivial_a = std::vector<bool>(global_metrics.count_d,true);
    global_metrics.is_trivial_b = std::vector<bool>(global_metrics.count_d,true);




    auto tmpdvars = origDqbf->GetDepVars();
    int tmpidvar=0;
    for(auto e:tmpdvars){
        dep_to_id[e]=tmpidvar;
        global_metrics.d_vars[tmpidvar] = e;
        auto tmpdepset = origDqbf->GetDependencySet(e);
        global_metrics.individual_dep_set_sizes[tmpidvar] = tmpdepset.size();
        tmpidvar++;
    }
    

    std::map<int, AigWrapper*> outputToAig;
    

    AigWrapper* finalFormula = new AigWrapper(origDqbf);

    numOrigInputs = finalFormula->getNumInputs();
    
    // finalFormula->ShowAig();


    std::set<int> existentials = origDqbf->GetExistentials();

    std::vector<int> exisVarsToEliminate;
    for(auto e:existentials){
        exisVarsToEliminate.push_back(e-1);
    
    }

    globalLogger.log(LogLevel::INFO, fmt::format("Eliminating existentials: [{}]", fmt::join(exisVarsToEliminate, " ")));


    std::vector<std::pair<int, AigWrapper*>> tseitinSkolems;

    
    DdManager* ddMan;
    DdNode* FddNode;
    Abc_Ntk_t* pNtk;
    
    global_metrics.last_checkpoint = "getBDD_START";

    auto getbdd_start_time = std::chrono::high_resolution_clock::now();


    getBDD(finalFormula, ddMan, FddNode, pNtk);

    auto getbdd_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> getbdd_elapsed = getbdd_end_time - getbdd_start_time;
    global_metrics.getBDD_time = getbdd_elapsed.count();

    global_metrics.last_checkpoint = "getBDD_END";



    globalLogger.log(LogLevel::INFO, "Generated BDD");
    Nnf_Man nnfNew;
    nnfNew.init(ddMan, FddNode);
    
    assert(nnfNew.isWDNNF()==true);
    Aig_Man_t* SAig = nnfNew.createAigWithoutClouds();
    
    


    if(!existentials.empty()){
        // exit(1);
        global_metrics.last_checkpoint = "TSEITIN_ELIMINATE_START";
        
        auto tseitin_start_time = std::chrono::high_resolution_clock::now();



        tseitinSkolems = getTseitinSkolems(SAig,exisVarsToEliminate);


        globalLogger.log(LogLevel::INFO, "Generated Skolem functions for tseitin Variables");


        // AigWrapper* newFinalFormula = quantify(pNtk, ddMan,FddNode, exisVarsToEliminate);
        quantify2(SAig, exisVarsToEliminate);
        globalLogger.log(LogLevel::INFO, "Quantified Existentials");

        // AigWrapper* tmpwrap=new AigWrapper();
        // tmpwrap->SetManager(SAig);
        // tmpwrap->compress();

        // // Aig_ManShow(SAig,0,NULL);
        // // int yy;
        // // std::cin>>yy;
        // tmpwrap->ShowAig();
        // exit(1);
        // AigWrapper* newFinal = finalFormula->quantify(exisVarsToEliminate, 1, tseitinSkolems);
        // delete finalFormula;
        auto tseitin_end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> tseitin_elapsed = tseitin_end_time - tseitin_start_time;
        global_metrics.tseitin_elimination_time = tseitin_elapsed.count();
        
        global_metrics.last_checkpoint = "TSEITIN_ELIMINATE_END";
    
    }
    finalFormula->SetManager(SAig);

    // finalFormula->ShowAig();

    std::set<int> depVars = origDqbf->GetDepVars();
    std::set<int> universals = origDqbf->GetUniversals();
    std::vector<AigWrapper*> finalSkolems;
    
    // for(auto e:existentials){
    //     depVars.insert(e);
    // }
    // finalFormula->ShowAig();

    globalLogger.log(LogLevel::INFO, "Generating Local Specifications");
    
    global_metrics.last_checkpoint = "GET_LOCAL_SPEC_START";
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

       
        
        outputToAig[target_d] = localSpec;

        finalSkolems.push_back(localSpec);
    
    }

    global_metrics.last_checkpoint = "GET_LOCAL_SPEC_END";

    Aig_Man_t* finalMan=finalFormula->getManager();
    getMonoAig(finalMan);

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
    globalLogger.log(LogLevel::INFO, fmt::format("Generating definitions"));

    for(auto p:outputToAig){
        p.second->addInputs(numNewInputs);
        globalLogger.log(LogLevel::INFO, fmt::format("Generating Def for id: {}", p.first));
        p.second->generateDef(p.first, origDqbf->GetNumInputs() + hCount);
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
    globalLogger.log(LogLevel::INFO, fmt::format("Creating DELTA AND NEG_PHI"));

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
    globalLogger.log(LogLevel::INFO, fmt::format("Starting CEGIS..."));


    global_metrics.last_checkpoint = "CEGIS_START";

    auto cegis_start_time = std::chrono::high_resolution_clock::now();

    if(!depVars.empty()) res = cegis(origDqbf, solverWrapper, unsatCoreWrapper, constraintWrapper, exToHMapping);
    else res = 0;

    auto cegis_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> cegis_elapsed = cegis_end_time - cegis_start_time;
    global_metrics.total_cegis_time = cegis_elapsed.count();

    global_metrics.last_checkpoint = "CEGIS_END";

    if(res==1){
        globalLogger.log(LogLevel::INFO, "No Solution Exists.");
        global_metrics.execution_status = "UNSATISFIABLE";
        exit(20);

    }
    else{
        globalLogger.log(LogLevel::INFO, "Solution Exists.");
        global_metrics.execution_status = "SATISFIABLE";
    }


    exit(1);
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