#include "Logger.h"
#include "Parser.h"

#include "helper.h"


void timeout_handler(int signum) {

    global_metrics.execution_status = "TIMEOUT";

    global_metrics.print_json_metrics();

    std::_Exit(signum);

}



void final_cleanup_hook() {

    

        global_metrics.print_json_metrics();

    

}

std::map<int,int> dep_to_id;
bool exisFailed=false;

int main(int argc, char* argv[]){



    signal(SIGTERM, timeout_handler);

    signal(SIGSEGV, timeout_handler);

    signal(SIGABRT, timeout_handler);

    std::atexit(final_cleanup_hook);
    global_metrics.start_timestamp = std::chrono::high_resolution_clock::now();
    Abc_Start();
    // globalLogger.setOutputFile("./main2_test.log");
    globalLogger.log(LogLevel::INFO, "Starting the program...");
    Parser* fileParser = new Parser(argc, argv);


    globalLogger.log(LogLevel::INFO, "Parsing the file...");

    Dqbf* origDqbf = fileParser->ParseDqbf();


    global_metrics.benchmark_name = argv[1];
    global_metrics.count_a = origDqbf->GetNumInputs();
    global_metrics.count_e = origDqbf->GetExistentials().size();
    global_metrics.count_d = origDqbf->GetDepVars().size(); - global_metrics.count_e;
    
    global_metrics.d_vars = std::vector<int>(global_metrics.count_d,0);
    global_metrics.individual_dep_set_sizes = std::vector<int>(global_metrics.count_d,0);
    global_metrics.individual_kissat_times = std::vector<double>(global_metrics.count_d,0.0);
    global_metrics.is_trivial_a = std::vector<bool>(global_metrics.count_d,false);
    global_metrics.is_trivial_b = std::vector<bool>(global_metrics.count_d,false);
    global_metrics.exis_eliminations = std::vector<int>(global_metrics.count_d,0);


    auto tmpdvars = origDqbf->GetDepVars();
    int tmpid_var = 0;
    for(auto e:tmpdvars){
        dep_to_id[e]=tmpid_var;
        global_metrics.d_vars[tmpid_var] = e;
        auto tmpdepset = origDqbf->GetDependencySet(e);
        global_metrics.individual_dep_set_sizes[tmpid_var] = tmpdepset.size();
        tmpid_var++;
    }


    globalLogger.log(LogLevel::INFO,"Generating Local Specs...");
    std::vector<KissatWrapper*> localInitializations = generateLocalSpecs(origDqbf);

    global_metrics.last_checkpoint = "BVE_START";
    for(auto kw:localInitializations){

        auto start_time = std::chrono::high_resolution_clock::now();
        // global_metrics.d_vars.push_back(kw->getOutputVar());
        globalLogger.log(LogLevel::INFO, fmt::format("Performing Quantifier Elimination for id: {}", kw->getOutputVar()));
        kw->eliminateExistentialVars();
        kw->eliminateUniversalVars();

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_time = end_time - start_time;
        global_metrics.individual_kissat_times[dep_to_id[kw->getOutputVar()]] = elapsed_time.count();
	int depsetsize = origDqbf->GetDependencySet(kw->getOutputVar()).size();
	// global_metrics.individual_dep_set_sizes[dep_to_id[kw->getOutputVar()]] = depsetsize;
    }

    global_metrics.last_checkpoint = "BVE_DONE";

    if(exisFailed){
        global_metrics.execution_status = "EXIS_QUANT_FAILED";
        exit(1);
    }


    globalLogger.setOutputFile("./statistics/eliminationStatistics.csv");
    int numY = origDqbf->GetDepVars().size();
    for(auto kw:localInitializations){
        globalLogger.log(LogLevel::STATS, fmt::format("{},{},{},{}", argv[1],kw->getOutputVar(), numY-1, kw->getEliminatedVars().size()));
    }

    globalLogger.closeOutputFile();


    // exit(1);




    std::map<int, AigWrapper*> outputToAig;
    for(auto kw:localInitializations){
        std::cout<<"Generating AIG for id: "<<kw->getOutputVar()<<std::endl;
        outputToAig[kw->getOutputVar()] = new AigWrapper(kw);
    }

    AigWrapper* finalFormula = new AigWrapper(origDqbf);
    // finalFormula->ShowAig();
    // finalFormula->substituteInputs(origDqbf->GetExistentials(),fileParser->argv[2], fileParser->argv[3]);
    AigWrapper* unsatCoreFormula = new AigWrapper(finalFormula);
    int numNewInputs = origDqbf->GetDepVars().size();
    std::cout << numNewInputs<<std::endl;
    // exit(1);
    // numNewInputs+= origDqbf->GetExistentials().size();
    finalFormula->addInputs(numNewInputs);
    unsatCoreFormula->addInputs(numNewInputs);
    finalFormula->negateOutput();
    globalLogger.log(LogLevel::INFO, "Final Formula:");
    // finalFormula->ShowAig();
    int hCount = 1;


    // exit(1);

    std::map<int, int> exToHMapping;

    for(auto p:outputToAig){
        p.second->addInputs(numNewInputs);
        p.second->generateDef(p.first, origDqbf->GetNumInputs() + hCount);


        Aig_Man_t* tMan = ABC_NAMESPACE::Abc_NtkToDar(varToBasisMap[p.first].first,0,0);
        if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst0(tMan) 
            && Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
                // printf("A_i is const 0 for id: %d\n",id);
                global_metrics.is_trivial_a[dep_to_id[p.first]] = true;
            }
            else{
                global_metrics.is_trivial_a[dep_to_id[p.first]] = false;
            }

        Aig_ManStop(tMan);
        tMan = ABC_NAMESPACE::Abc_NtkToDar(varToBasisMap[p.first].second,0,0);

            std::filesystem::path bp(global_metrics.benchmark_name);
        std::string pure_name = bp.stem().string();
            std::string a_path = "./experiment/basis_a/app2/"+pure_name+".aig";
            std::string b_path = "./experiment/basis_b/app2/"+pure_name+".aig";
        Io_WriteAiger(varToBasisMap[p.first].first,(char*)a_path.c_str(),0,1,0);
        Io_WriteAiger(varToBasisMap[p.first].second,(char*)b_path.c_str(),0,1,0);
        if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst1(tMan) 
            && Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
                // printf("A_i is const 0 for id: %d\n",id);
                global_metrics.is_trivial_b[dep_to_id[p.first]] = true;
            }
            else{
                global_metrics.is_trivial_b[dep_to_id[p.first]] = false;
            }
            Aig_ManStop(tMan);
        globalLogger.log(LogLevel::INFO, fmt::format("Generating Def for id: {}", p.first));
        // p.second->ShowAig();
        exToHMapping[p.first] = origDqbf->GetNumInputs() + hCount;
        hCount++;
    }

    for(auto p:exToHMapping){
        globalLogger.log(LogLevel::INFO, fmt::format("Ex: {} -> H: {}", p.first, p.second));
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
    // finalFormula->ShowAig();
    std::cout<<finalFormula->GetNumOutputs()<<std::endl;
    // finalFormula->ShowAig();

    CadicalWrapper* solverWrapper = new CadicalWrapper(finalFormula);
    CadicalWrapper* unsatCoreWrapper = new CadicalWrapper(unsatCoreFormula);
    CadicalWrapper* constraintWrapper = new CadicalWrapper();


    global_metrics.last_checkpoint = "CEGIS_START";
    auto start_time = std::chrono::high_resolution_clock::now();


    int res = cegis(origDqbf, solverWrapper, unsatCoreWrapper, constraintWrapper, exToHMapping);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;
    global_metrics.total_cegis_time = elapsed_time.count();
    global_metrics.last_checkpoint = "CEGIS_DONE";
    // global_metrics.execution_status = (res == 0) ? "SUCCESS" : "UNSAT";
    // global_metrics.print_json_metrics();
    
    if(res==1){
        globalLogger.log(LogLevel::INFO, "No Solution Exists.");
        global_metrics.execution_status = "UNSATISFIABLE";
        // global_metrics.print_json_metrics();
    }
    else{
        globalLogger.log(LogLevel::INFO, "Solution Exists.");
        global_metrics.execution_status = "SATISFIABLE";
        // global_metrics.print_json_metrics();
    }

    Abc_Stop();



    return 0;
}
