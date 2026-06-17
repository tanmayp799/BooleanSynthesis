
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
#include "helper.h"

using namespace std;

////////////////////////////////////////////////////////////////////////
///                           GLOBALS                                ///
////////////////////////////////////////////////////////////////////////

// int signal;

vector<int> varsSInv;
vector<int> varsXF, varsXS;
vector<int> varsYF, varsYS; // to be eliminated
int numOrigInputs = 0, numX = 0, numY = 0;
vector<string> varsNameX, varsNameY;
Abc_Frame_t *pAbc = NULL;
sat_solver *m_pSat = NULL;
Cnf_Dat_t *m_FCnf = NULL;
lit m_f = 0;
double sat_solving_time = 0;
double verify_sat_solving_time = 0;
double reverse_sub_time = 0;
chrono_steady_time helper_time_measure_start = TIME_NOW;
chrono_steady_time main_time_start = TIME_NOW;
int numTrue=0, numFalse=0, numBoth=0;
// Abc_Frame_t *pAbc = NULL;

// int numX, numY;
map<int, vector<int>> dependencies;

// ExperimentalMetrics global_metrics;


void timeout_handler(int signum) {
    global_metrics.execution_status = "TIMEOUT";
    global_metrics.print_json_metrics();
    std::_Exit(signum);
}

void final_cleanup_hook() {
    
        global_metrics.print_json_metrics();
    
}

void callManthan(){
    // global_metrics.last_checkpoint = "MANTHAN_START";


    auto start_time = std::chrono::high_resolution_clock::now();


    std::filesystem::path p(global_metrics.benchmark_name);
    string rawname=p.filename().string();

    string inpFile = "../benchmark_tests/dqbf20/"+rawname;
    string command = "cd ./manthan_test && ./test.sh " + inpFile;
    cout<<command<<endl;
    int return_code = std::system(command.c_str());
    // int xx;
    // cin>>xx;


    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;
    global_metrics.manthan_time = elapsed_time.count();


    if(return_code !=0){
        global_metrics.execution_status = "ERROR_MANTHAN_FAILED";

        // global_metrics.print_json_metrics();
        exit(1);
    }
    return;
}

int main(int argc, char *argv[])
{
    // extern Abc_Ntk_t * Abc_NtkDarToCnf( Abc_Ntk_t * pNtk, char * pFileName, int fFastAlgo, int fChangePol, int fVerbose );


    global_metrics.start_timestamp = std::chrono::high_resolution_clock::now();
    signal(SIGTERM, timeout_handler);
    signal(SIGSEGV, timeout_handler);
    signal(SIGABRT, timeout_handler);
    std::atexit(final_cleanup_hook);


    global_metrics.benchmark_name = argv[1];

    int mySIG;
    Abc_Start();
    srand(time(0));
    CaDiCaL::Solver solver;
    solver.set("incremental",1);
    // exit(1);

    

    // ensure phi.dqdimacs is taken as input
    bool didManthan=false;
    // cout<<argc<<endl;
    // exit(1);
    // if (argc == 4)  
    // {
    //     didManthan=true;
    // }

    if(argc!=2){
        cerr<<"Usage: ./bin/main <DQDIMACS FILE>"<<endl;
        return 1;
    }


   
    
    main_time_start = TIME_NOW;

    string inpPath = argv[1];
    cout<<inpPath<<endl;
    FS::path phiPath(inpPath);
    DQCNF *phiCNF = new DQCNF(phiPath.string());
    cout << "Read the file\n";
    phiCNF->unateCheck();
    // phiCNF->preprocess();

    auto stat_exs = phiCNF->get_existentials();
    auto stat_univs = phiCNF->get_universals();
    auto stat_deps = phiCNF->get_deps();

    global_metrics.count_a = stat_univs.size();
    global_metrics.count_e = stat_exs.size();
    global_metrics.count_d = stat_deps.size();
    global_metrics.last_checkpoint = "INIT";

    for(auto id:stat_deps){
        global_metrics.d_vars.push_back(id);
        auto depset = phiCNF->get_dependencySet(id);
        global_metrics.individual_dep_set_sizes.push_back(depset.size());
    }

    global_metrics.individual_projection_times.resize(global_metrics.count_d,0.0);
    global_metrics.is_trivial_a.resize(global_metrics.count_d, false);
    global_metrics.is_trivial_b.resize(global_metrics.count_d, false);

    Aig_Man_t *phi_Man = phiCNF->genAIGMan();

    auto tmp_exis = phiCNF->get_existentials();
    if(!tmp_exis.empty()){
        didManthan=true;
        global_metrics.last_checkpoint = "MANTHAN_START";
        callManthan();
        global_metrics.last_checkpoint = "MANTHAN_END";
    }

    map<int, int> dep_id_to_idx; // Maps variable ID to its 0-indexed array position
    int current_idx = 0;
    for (auto id : stat_deps) {
        dep_id_to_idx[id] = current_idx;
        global_metrics.individual_dep_set_sizes[current_idx] = phiCNF->get_dependencySet(id).size();
        current_idx++;
    }

    
    numOrigInputs = Aig_ManCiNum(phi_Man);

    // Aig_ManShow(phi_Man,0,NULL);

    // cout<<numOrigInputs<<endl;
    // exit(1);
    cout << "Generated Manager for PHI\n";
    Abc_Ntk_t *phi_Ntk = Abc_NtkFromAigPhase(phi_Man);

    // auto exs = phiCNF->get_existentials();
    auto deps = phiCNF->get_deps();
    // deps = phiCNF->get_existentials();
    TIME_MEASURE_START
    cout << "Generating DQCNF and Aig_Man_t Objects for Projected PHI_i" << endl;
    map<int, DQCNF *> projectedPhis;
    map<int, Aig_Man_t *> projectedMans;

    string stat_filename = "/home/coolboy19/Desktop/RnD/BooleanSynthesis/projection_stat.csv";
    FILE* stat_file = fopen(stat_filename.c_str(),"a");
    global_metrics.last_checkpoint = "PROJECTION_START";
    // fprintf(stat_file, "tc_name, numPos, numNeg, numBoth\n");
    for (auto id : deps)
    {

        auto proj_start = std::chrono::high_resolution_clock::now();
        

        DQCNF *tCNF = phiCNF->getProjection(id);
        projectedPhis[id] = tCNF;
        Aig_Man_t *tMan = tCNF->genAIGMan();

        cout << "Generated DQCNF and Aig_Man_t Objects for Projected PHI_" << id << endl;
        tMan = compressAig(tMan);
        projectedMans[id] = tMan;

        auto proj_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proj_duration = proj_end - proj_start;
        global_metrics.individual_projection_times[dep_id_to_idx[id]] = proj_duration.count();

        




        if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst1(tMan) 
            && Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
                printf("Phi_i is const 0 for id: %d\n",id);
            }

        
    }
    global_metrics.last_checkpoint = "PROJECTION_END";
    // cout<<"DONE. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;

    // std::ostringstream oss;
    // oss << phiPath.filename().string() << ", " << numTrue << ", " << numFalse << ", " << numBoth << "\n";
    // std::string result = oss.str(); // result: "example, 1, 2, 3\n"
    // // string output_stat = std::format("{}, {}, {}, {}\n", phiPath.filename().string(), numTrue, numFalse, numBoth);
    // fprintf(stat_file,result.c_str());
    // fclose(stat_file);
    // exit(1);
    cout<<"DONE. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;


    TIME_MEASURE_START
    cout << "Generating PHI_i(0) and PHI_i(1)...\n";
    map<int, Aig_Man_t *> phi_1_Man;
    map<int, Aig_Man_t *> phi_0_Man;
    for (auto p : projectedMans)
    {
        Aig_Man_t *tMan = Aig_ManDupOrdered(p.second);
        cout << "Generating Phi(1) and Phi(0) for EX_VAR: " << p.first << endl;

        // get the new output driver
        Aig_Obj_t *newOut = Aig_SubstituteConst(tMan, Aig_ManCo(tMan, 0), p.first, 1);
        // create new output
        Aig_ObjCreateCo(tMan, newOut);

        // remove old output and cleanup the network to remove dangling nodes
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 0));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, 0), Aig_ManConst0(tMan), NULL);
        Aig_ManCoCleanup(tMan);
        Aig_ManCleanup(tMan);

        if (Aig_ManCoNum(tMan) == 0)
        {
            Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
        }
        // cout<<"Printing Phi_1\n";
        // Aig_ManShow(tMan,0,NULL);
        // int q;
        // cin>>q;
        

        phi_1_Man[p.first] = tMan;

        // Aig_ManShow(tMan,0,NULL);
        // cin>>mySIG;

        tMan = Aig_ManDupOrdered(p.second);
        // get the new output driver
        newOut = Aig_SubstituteConst(tMan, Aig_ManCo(tMan, 0), p.first, 0);
        // create new output
        Aig_ObjCreateCo(tMan, newOut);

        // remove old output and cleanup the network to remove dangling nodes
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 0));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, 0), Aig_ManConst0(tMan), NULL);
        Aig_ManCoCleanup(tMan);
        Aig_ManCleanup(tMan);

        if (Aig_ManCoNum(tMan) == 0)
        {
            Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
        }
        // cout<<"Printing Phi_0\n";
        // Aig_ManShow(tMan,0,NULL);
        // // int q;
        // cin>>q;
       

        phi_0_Man[p.first] = tMan;
        Aig_ManStop(p.second);
    }
    cout<<"DONE. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
    // cout << "****   DONE!   *****\n";


    TIME_MEASURE_START
    cout << "Generating Basis Functions...\n";

    map<int, Aig_Man_t *> A_Man;
    map<int, Aig_Man_t *> B_Man;
    map<int, Abc_Ntk_t *> A_Ntk;
    map<int, Abc_Ntk_t *> B_Ntk;

    global_metrics.last_checkpoint = "BASIS_START";

    for (auto id : deps)
    {

        cout << "Generating Basis for EX_VAR: " << id << endl;

        Aig_Man_t *phi_1 = phi_1_Man[id];
        Aig_Man_t *phi_0 = phi_0_Man[id];

        // get the networks
        Abc_Ntk_t *phi_1_Ntk = Abc_NtkFromAigPhase(phi_1);
        Abc_Ntk_t *phi_0_Ntk = Abc_NtkFromAigPhase(phi_0);

        // append networks
        Abc_NtkAppend(phi_0_Ntk, phi_1_Ntk, 1);

        // get the manager for A
        Aig_Man_t *tMan = Abc_NtkToDar(phi_0_Ntk, 0, 0);

        // create output driver for A
        Aig_Obj_t *newNode = Aig_And(tMan, Aig_ManCo(tMan, 1)->pFanin0, Aig_Not(Aig_ManCo(tMan, 0)->pFanin0));

        // Create new Output
        Aig_ObjCreateCo(tMan, newNode);

        // remove old outputs and cleanup the network
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 0));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, 0), Aig_ManConst0(tMan), NULL);
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 1));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, 1), Aig_ManConst0(tMan), NULL);
        Aig_ManCoCleanup(tMan);
        Aig_ManCleanup(tMan);

        if (Aig_ManCoNum(tMan) == 0)
        {
            Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
        }
        // tMan = Aig(tMan);
        tMan = compressAig(tMan);

        A_Man[id] = tMan;
        A_Ntk[id] = Abc_NtkFromAigPhase(tMan);

        if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst1(tMan) 
            && Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
                printf("A_i is const 0 for id: %d\n",id);
                global_metrics.is_trivial_a[dep_id_to_idx[id]] = true;
            }
        if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst1(tMan) 
            && !Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
                printf("A_i is const 1 for id: %d\n",id);
            }

        // if(id==25){

            // cout<<"Printing A_i:\n";
            // Aig_ManShow(tMan,0,NULL);
            // int q;
            // cin>>q;
        // }
        // get the manager for B
        tMan = Abc_NtkToDar(phi_0_Ntk, 0, 0);

        // phi1 OR !phi_0
        Aig_Obj_t *newNode1 = Aig_Or(tMan, Aig_ManCo(tMan, 1)->pFanin0, Aig_Not(Aig_ManCo(tMan, 0)->pFanin0));

        // !phi OR phi0
        Aig_Obj_t *newNode2 = Aig_Or(tMan, Aig_ManCo(tMan, 0)->pFanin0, Aig_Not(Aig_ManCo(tMan, 1)->pFanin0));

        // newNode1 AND newNode2
        Aig_Obj_t *finalNewNode = Aig_And(tMan, newNode1, newNode2);

        // Create new Output
        Aig_ObjCreateCo(tMan, finalNewNode);

        // remove old outputs and cleanup the network
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 0));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, 0), Aig_ManConst0(tMan), NULL);
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, 1));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, 1), Aig_ManConst0(tMan), NULL);
        Aig_ManCoCleanup(tMan);
        Aig_ManCleanup(tMan);

        if (Aig_ManCoNum(tMan) == 0)
        {
            Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
        }
        
        tMan = compressAig(tMan);

        B_Man[id] = tMan;
        B_Ntk[id] = Abc_NtkFromAigPhase(tMan);
        
        if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst1(tMan) 
            && Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
                printf("B_i is const 0 for id: %d\n",id);
            }
        if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst1(tMan) 
            && !Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
                printf("B_i is const 1 for id: %d\n",id);
                global_metrics.is_trivial_b[dep_id_to_idx[id]] = true;
            }

        // if(id==25){

            // cout<<"Printing B_i:\n";
            // Aig_ManShow(tMan,0,NULL);
            // // int q;
            // cin>>q;
        // }
        // if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) == Aig_ManConst1(tMan)){
        //     if(Aig_ObjFaninC0())
        // }

        Aig_ManStop(phi_0_Man[id]);
        Aig_ManStop(phi_1_Man[id]);
    }

    global_metrics.last_checkpoint = "BASIS_END";
    cout<<"DONE. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
    // cout << "****   DONE!   *****\n";

    cout << "Generating AIG Manager for negPHI...\n";
    Aig_Man_t *negPhi_Man = Aig_ManDupOrdered(phi_Man);
    // Aig_ManStop(phi_Man);

    Aig_ManCo(negPhi_Man, 0)->pFanin0 = Aig_Not(Aig_ManCo(negPhi_Man, 0)->pFanin0);


    Aig_Man_t* origFormula = negPhi_Man;
    Aig_Man_t* deltaAndPhi_Man = phi_Man;

    // Aig_ManShow(origFormula,0,NULL);
    // cin>>mySIG;

    if(didManthan){
        cout<<"APPENDING y_i <=> f(a,d)\n";
        // cin>>mySIG;
        Abc_Ntk_t* defNtk = Io_ReadVerilog((char*)"./manthan_test/out_skolem.v",0);
        defNtk = Abc_NtkToLogic(defNtk);
        defNtk = Abc_NtkStrash(defNtk,0,1,0);
        
        Aig_Man_t* eDefMan = Abc_NtkToDar(defNtk,0,0);
        while(Aig_ManCiNum(eDefMan) < numOrigInputs){
            Aig_ObjCreateCi(eDefMan);
        }
        
        ifstream f("./manthan_test/ordering.txt");
        vector<int> inputMapping;
        vector<int> outputMapping;
        string str;
        
        if(!f){
            cerr<<"Error opening file\n";
            return 1;
        }
        
        if(getline(f,str)){
            stringstream ss(str);
            int num;
            while(ss>>num){
                inputMapping.push_back(num);
            }
        }
        
        if(getline(f,str)){
            stringstream ss(str);
            int num;
            while(ss>>num){
                outputMapping.push_back(num);
            }
        }
        
        vector<int> ordering(inputMapping.begin(),inputMapping.end());
        for(auto n:outputMapping){
            ordering.push_back(n);
        }

        for(auto e:ordering){
            cout<<e<<endl;
        }

        
        eDefMan = remapInputs(eDefMan,ordering);
        

        //compose outputs of eDefMan such that inp[OutMap[i]] <=> out[i]
        
        assert(outputMapping.size() == Aig_ManCoNum(eDefMan));
        int numOut = Aig_ManCoNum(eDefMan);
        Aig_Obj_t* newOutput = Aig_ManConst1(eDefMan);
        for(int i=0;i<numOut;i++){
            Aig_Obj_t* y_i = Aig_ManCi(eDefMan, outputMapping[i]-1);
            Aig_Obj_t* fy_i = Aig_ManCo(eDefMan,i)->pFanin0;

            
            Aig_Obj_t* t1 = Aig_Or(eDefMan, Aig_Not(y_i),fy_i);
            Aig_Obj_t* t2 = Aig_Or(eDefMan, y_i, Aig_Not(fy_i));
            Aig_Obj_t* t3 = Aig_And(eDefMan,t1,t2);

            newOutput = Aig_And(eDefMan,newOutput,t3);
        }

        Aig_ObjCreateCo(eDefMan,newOutput);

        for(int i=0; i<Aig_ManCoNum(eDefMan)-1;i++){
            Aig_ObjDisconnect(eDefMan, Aig_ManCo(eDefMan,i));
            Aig_ObjConnect(eDefMan,Aig_ManCo(eDefMan,i),Aig_ManConst0(eDefMan),NULL);
        }
        Aig_ManCoCleanup(eDefMan);
        Aig_ManCleanup(eDefMan);
        
        

        Abc_Ntk_t* origFormulaNtk = Abc_NtkFromAigPhase(origFormula);
        Abc_Ntk_t* eDefNtk = Abc_NtkFromAigPhase(eDefMan);
        
        // Abc_Ntk_t* deltaAndPhi_Ntk = Abc_NtkFromAigPhase(deltaAndPhi_Man);

        assert(Aig_ManCiNum(eDefMan) == Aig_ManCiNum(origFormula));
        // assert(Aig_ManCiNum(eDefMan) == Aig_ManCiNum(deltaAndPhi_Man));

        Abc_NtkAppend(origFormulaNtk, eDefNtk, 1);
        origFormula = Abc_NtkToDar(origFormulaNtk,0,0);
        Aig_Obj_t* o1 = Aig_ManCo(origFormula,0)->pFanin0;
        Aig_Obj_t* o2 = Aig_ManCo(origFormula,1)->pFanin0;
        Aig_Obj_t* o3 = Aig_And(origFormula,o1,o2);

        Aig_ObjCreateCo(origFormula,o3);

        Aig_ObjDisconnect(origFormula,Aig_ManCo(origFormula,0));
        Aig_ObjConnect(origFormula, Aig_ManCo(origFormula,0),Aig_ManConst0(origFormula),NULL);
        Aig_ObjDisconnect(origFormula,Aig_ManCo(origFormula,1));
        Aig_ObjConnect(origFormula, Aig_ManCo(origFormula,1),Aig_ManConst0(origFormula),NULL);

        Aig_ManCoCleanup(origFormula);
        Aig_ManCleanup(origFormula);


        // Abc_NtkAppend(deltaAndPhi_Ntk, eDefNtk, 1);
        // deltaAndPhi_Man = Abc_NtkToDar(deltaAndPhi_Ntk,0,0);
        // Aig_Obj_t* o11 = Aig_ManCo(deltaAndPhi_Man,0)->pFanin0;
        // Aig_Obj_t* o21 = Aig_ManCo(deltaAndPhi_Man,1)->pFanin0;
        // Aig_Obj_t* o31 = Aig_And(deltaAndPhi_Man,o11,o21);

        // Aig_ObjCreateCo(deltaAndPhi_Man,o31);

        // Aig_ObjDisconnect(deltaAndPhi_Man,Aig_ManCo(deltaAndPhi_Man,0));
        // Aig_ObjConnect(deltaAndPhi_Man, Aig_ManCo(deltaAndPhi_Man,0),Aig_ManConst0(deltaAndPhi_Man),NULL);
        // Aig_ObjDisconnect(deltaAndPhi_Man,Aig_ManCo(deltaAndPhi_Man,1));
        // Aig_ObjConnect(deltaAndPhi_Man, Aig_ManCo(deltaAndPhi_Man,1),Aig_ManConst0(deltaAndPhi_Man),NULL);

        // Aig_ManCoCleanup(deltaAndPhi_Man);
        // Aig_ManCleanup(deltaAndPhi_Man);

    }

    // Aig_ManShow(origFormula,0,NULL);
    // cin>>mySIG;

    // Aig_ManShow(deltaAndPhi_Man,0,NULL);
    // cin>>mySIG;
    // exit(1);


    cout << "Creating the formula DELTA (AND) !PHI\n";

    Aig_Man_t *defsMan = Aig_ManStart(0);

    // Append all basis into negPHI
    Abc_Ntk_t *negPHI_Ntk = Abc_NtkFromAigPhase(origFormula);

    // Abc_Ntk_t* phi_ntk = Abc_NtkFromAigPhase(deltaAndPhi_Man);

    Aig_ManStop(origFormula);
    // Aig_ManStop(deltaAndPhi_Man);

    map<int, pair<int, int>> exToOutMapping;
    for (auto id : deps)
    {
        Abc_Ntk_t *ANtk = Abc_NtkFromAigPhase(A_Man[id]);
        Abc_Ntk_t *BNtk = Abc_NtkFromAigPhase(B_Man[id]);

        Abc_NtkAppend(negPHI_Ntk, ANtk, 1);
        Abc_NtkAppend(negPHI_Ntk, BNtk, 1);

        // Abc_NtkAppend(phi_ntk, ANtk, 1);
        // Abc_NtkAppend(phi_ntk, BNtk, 1);

        cout << "Appended Basis for EX_VAR: " << id << endl;

        exToOutMapping[id] = {Abc_NtkCoNum(negPHI_Ntk) - 2, Abc_NtkCoNum(negPHI_Ntk) - 1};
        Aig_ManStop(A_Man[id]);
        Aig_ManStop(B_Man[id]);
    }

    Aig_Man_t *tMan = Abc_NtkToDar(negPHI_Ntk, 0, 0);
    // Aig_Man_t* tMan2 = Abc_NtkToDar(phi_ntk,0,0);
    // compose the outputs and create the final output for delta ^ !phi
    map<int, int> exToHMapping;
    Aig_Obj_t *delta = Aig_ManConst1(tMan);
    // Aig_Obj_t* delta2 = Aig_ManConst1(tMan2);
    for (auto id : deps)
    {
        Aig_Obj_t *outA = Aig_ManCo(tMan, exToOutMapping[id].first)->pFanin0;
        Aig_Obj_t *outB = Aig_ManCo(tMan, exToOutMapping[id].second)->pFanin0;

        // Aig_Obj_t *outA2 = Aig_ManCo(tMan2, exToOutMapping[id].first)->pFanin0;
        // Aig_Obj_t *outB2 = Aig_ManCo(tMan2, exToOutMapping[id].second)->pFanin0;

        Aig_Obj_t *currH = Aig_ObjCreateCi(tMan);
        
        // Aig_Obj_t* currH2 = Aig_ObjCreateCi(tMan2);

        exToHMapping[id] = Aig_ManCiNum(tMan);
        Aig_Obj_t *HAndB = Aig_And(tMan, currH, outB);
        Aig_Obj_t *defin = Aig_Or(tMan, outA, HAndB);
        Aig_Obj_t *yImpDefin = Aig_Or(tMan, Aig_Not(Aig_ManCi(tMan, id - 1)), defin);
        Aig_Obj_t *definImpY = Aig_Or(tMan, Aig_Not(defin), Aig_ManCi(tMan, id - 1));

        Aig_Obj_t *currDelta = Aig_And(tMan, yImpDefin, definImpY);

        delta = Aig_And(tMan,delta,currDelta);

        // Aig_Obj_t *HAndB2 = Aig_And(tMan2, currH2, outB2);
        // Aig_Obj_t *defin2 = Aig_Or(tMan2, outA2, HAndB2);
        // Aig_Obj_t *yImpDefin2 = Aig_Or(tMan2, Aig_Not(Aig_ManCi(tMan2, id - 1)), defin2);
        // Aig_Obj_t *definImpY2 = Aig_Or(tMan2, Aig_Not(defin2), Aig_ManCi(tMan2, id - 1));

        // Aig_Obj_t *currDelta2 = Aig_And(tMan2, yImpDefin2, definImpY2);

        // delta2 = Aig_And(tMan2, delta2, currDelta2);
    }

    
    Aig_Obj_t *outputDriver = Aig_And(tMan, delta, Aig_ManCo(tMan, 0)->pFanin0);
    Aig_Obj_t *newOut = Aig_ObjCreateCo(tMan, outputDriver);
    
    // Aig_ManShow(tMan,0,NULL);
    // cin>>mySIG;

    // Aig_Obj_t *outputDriver2 = Aig_And(tMan2, delta2, Aig_ManCo(tMan2,0)->pFanin0);
    // Aig_Obj_t* newOut2 = Aig_ObjCreateCo(tMan2, outputDriver2);

    int numOuts = Aig_ManCoNum(tMan);
    for (int i = 0; i < numOuts - 1; i++)
    {
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, i));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, i), Aig_ManConst0(tMan), NULL);
    }

    // Aig_ManShow(tMan,0,NULL);
    // cin>>mySIG;

    // int numOuts2 = Aig_ManCoNum(tMan2);
    // for (int i = 0; i < numOuts2 - 1; i++)
    // {
    //     Aig_ObjDisconnect(tMan2, Aig_ManCo(tMan2, i));
    //     Aig_ObjConnect(tMan2, Aig_ManCo(tMan2, i), Aig_ManConst0(tMan2), NULL);
    // }

    // Abc_Ntk_t* tntk = Abc_NtkFromAigPhase(origFormula);
    // exit(1);
    // Aig_Man_t* origFormula = Aig_ManDupOneOutput(tMan,Aig_ManCoNum(tMan)-1,0);
    Aig_ManCoCleanup(tMan);
    Aig_ManCleanup(tMan);

    if (Aig_ManCoNum(tMan) == 0)
    {
        Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
    }

    // Aig_ManCoCleanup(tMan2);
    // Aig_ManCleanup(tMan2);

    // if (Aig_ManCoNum(tMan2) == 0)
    // {
    //     Aig_ObjCreateCo(tMan2, Aig_ManConst0(tMan2));
    // }

    
    Aig_ManCleanup(tMan);

    origFormula = Aig_ManDupSimple(tMan);
    Aig_ManStop(tMan);
    
    Aig_ManCleanup(origFormula);
    // Aig_ManReduceLaches(origFormula,1);
    
    // Aig_ManShow(ori)
    
    // 

    // deltaAndPhi_Man = Aig_ManDupSimple(tMan2);
    // Aig_ManStop(tMan2);
    
    Aig_Obj_t *deltaAndNegPhi = Aig_ManCo(origFormula, 0)->pFanin0;

    map<int, set<set<int>>> cases;


    map<int, Aig_Obj_t *> exToCases; // cases when h=0;

    for (auto id : deps)
    {
        exToCases[id] = Aig_ManConst0(origFormula);
        cases[id] = set<set<int>>();
    }

    // build mu and default case;
    Aig_Obj_t *mu = Aig_ManConst1(origFormula);
    Aig_Obj_t *defaultCase = Aig_ManConst1(origFormula);


    // for (auto id : deps)
    // {
    //     Aig_Obj_t *currCase = exToCases[id];

    //     // case imp ~h = ~case v ~h


    //     Aig_Obj_t *tmp = Aig_Or(origFormula, Aig_Not(currCase), Aig_Not(Aig_ManCi(origFormula,exToHMapping[id]-1)));
    //     mu = Aig_And(origFormula, mu, tmp);

    //     // ~case => h<=>1 = case v h
    //     // tmp = Aig_Or(origFormula, currCase, Aig_ManCi(origFormula,exToHMapping[id]-1));
    //     // defaultCase = Aig_And(origFormula, defaultCase, tmp);
    // }

    Aig_Obj_t *constraint = Aig_ManConst1(origFormula);

    // compose the nodes to create final output
    newOut = Aig_ManConst1(origFormula);
    newOut = Aig_And(origFormula, newOut, deltaAndNegPhi); // this stays constant
    newOut = Aig_And(origFormula, newOut, mu);             // this changes by mu -> mu AND tmpMu
    newOut = Aig_And(origFormula, newOut, defaultCase);    // we build this from exToCases
    newOut = Aig_And(origFormula, newOut, constraint);     // this updates by constraint -> constraint AND newConstraint.

    Aig_ObjCreateCo(origFormula, newOut);
    // Aig_ObjCreateCo(origFormula,mu);
    // Aig_ObjCreateCo(origFormula,defaultCase);
    // Aig_ObjCreateCo(origFormula,constraint);
    // Aig_ObjCreateCo(origFormula,deltaAndNegPhi);

    // Aig_ManShow(origFormula,0,NULL);
    // cin>>mySIG;
    Aig_ObjDisconnect(origFormula, Aig_ManCo(origFormula, 0));
    Aig_ObjConnect(origFormula, Aig_ManCo(origFormula, 0), Aig_ManConst0(origFormula), NULL);
    Aig_ManCoCleanup(origFormula);
    Aig_ManCleanup(origFormula);
    // Aig_ManShow(origFormula,0,NULL);
    // cin>>mySIG;
    // exit(1);

    if (Aig_ManCoNum(origFormula) == 0)
    {
        Aig_ObjCreateCo(origFormula, Aig_ManConst0(origFormula));
    }

    // A manager for constraints to check for sat

    

    Aig_Man_t *constraintMan = Aig_ManDupOrdered(origFormula);



    Aig_ObjDisconnect(constraintMan, Aig_ManCo(constraintMan, 0));
    Aig_ObjConnect(constraintMan, Aig_ManCo(constraintMan, 0), Aig_ManConst0(constraintMan), NULL);

    // Aig_ObjDisconnect(constraintMan, Aig_ManCo(constraintMan, 1));
    // Aig_ObjConnect(constraintMan, Aig_ManCo(constraintMan, 1), Aig_ManConst0(constraintMan), NULL);

    // Aig_ObjDisconnect(constraintMan, Aig_ManCo(constraintMan, 2));
    // Aig_ObjConnect(constraintMan, Aig_ManCo(constraintMan, 2), Aig_ManConst0(constraintMan), NULL);

    // Aig_ObjDisconnect(constraintMan, Aig_ManCo(constraintMan, 4));
    // Aig_ObjConnect(constraintMan, Aig_ManCo(constraintMan, 4), Aig_ManConst0(constraintMan), NULL);

    Aig_ManCoCleanup(constraintMan);
    Aig_ManCleanup(constraintMan);
    // Abc_Ntk_t* ontk = Abc_NtkFromAigPhase(origFormula);
    // ontk =  Abc_NtkToLogic(ontk);
    

    if (Aig_ManCoNum(constraintMan) == 0)
    {
        Aig_ObjCreateCo(constraintMan, Aig_ManConst1(constraintMan));
    }
   

    // Aig_ManShow(deltaAndPhi_Man,0,NULL);
    // cin>>mySIG;
    // Aig_ManShow(origFormula,0,NULL);
    // exit(1);
    // Aig_ManShow(origFormula,0,NULL);
    // cin>>mySIG;
    origFormula = compressAig(origFormula);
    
    // Aig_ManShow(origFormula,0,NULL);
    // cin>>mySIG;

    Cnf_Dat_t* pCnf;
    Abc_Ntk_t* origFormulaNtk = Abc_NtkFromAigPhase(origFormula);
   
    pCnf = myDarToCnf(origFormulaNtk,NULL,0,0,1);

    int * pLit, * pStop;
    // printf("Num Vars: %d, Num Clauses: %d\n",pCnf->nVars, pCnf->nClauses);
    for(int i=0;i<pCnf->nClauses;i++){
        for(pLit=pCnf->pClauses[i],pStop=pCnf->pClauses[i+1];pLit<pStop;pLit++){
            // printf("%d ", Cnf_Lit2Var2(*pLit));
            solver.add(Cnf_Lit2Var2(*pLit));
        }
        // printf("0\n");
        solver.add(0);
    }
    
    int numAigInputs = Abc_NtkCiNum(origFormulaNtk);
    map<int, int> inputToVarMapping;
    FILE* mapFile = fopen("./map.txt","w") ;
    fprintf(mapFile,"ORIGINAL INPUTS:\n");
    for(int i=0;i<Abc_NtkCiNum(origFormulaNtk);i++){
        if(i==numOrigInputs){
            fprintf(mapFile,"H Variables:\n");
        }
        fprintf(mapFile,"INPUT %d , var map: %d\n",i+1, pCnf->pVarNums[Abc_ObjId(Abc_NtkCi(origFormulaNtk,i))]);
        inputToVarMapping[i+1] = pCnf->pVarNums[Abc_ObjId(Abc_NtkCi(origFormulaNtk,i))];
    }


    CaDiCaL::Solver unsatCoreExtractor;

    
    // Aig_ManShow(deltaAndPhi_Man,0,NULL);
    // cin>>mySIG;
    deltaAndPhi_Man = compressAig(deltaAndPhi_Man);
    Cnf_Dat_t* pCnf2;
    Abc_Ntk_t* deltaAndPhiNtk = Abc_NtkFromAigPhase(deltaAndPhi_Man);
   
    pCnf2 = myDarToCnf(deltaAndPhiNtk,NULL,0,0,1);

    int * pLit2, * pStop2;
    // printf("Num Vars: %d, Num Clauses: %d\n",pCnf->nVars, pCnf->nClauses);
    for(int i=0;i<pCnf2->nClauses;i++){
        for(pLit2=pCnf2->pClauses[i],pStop2=pCnf2->pClauses[i+1];pLit2<pStop2;pLit2++){
            // printf("%d ", Cnf_Lit2Var2(*pLit));
            unsatCoreExtractor.add(Cnf_Lit2Var2(*pLit2));
        }
        // printf("0\n");
        unsatCoreExtractor.add(0);
    }
    
    int numAigInputs2 = Abc_NtkCiNum(deltaAndPhiNtk);
    map<int, int> inputToVarMapping_unsatCore;
    FILE* mapFile2 = fopen("./map_unsatCore.txt","w") ;
    fprintf(mapFile2,"ORIGINAL INPUTS:\n");
    for(int i=0;i<Abc_NtkCiNum(deltaAndPhiNtk);i++){
        if(i==numOrigInputs){
            fprintf(mapFile2,"H Variables:\n");
        }
        fprintf(mapFile2,"INPUT %d , var map: %d\n",i+1, pCnf2->pVarNums[Abc_ObjId(Abc_NtkCi(deltaAndPhiNtk,i))]);
        inputToVarMapping_unsatCore[i+1] = pCnf2->pVarNums[Abc_ObjId(Abc_NtkCi(deltaAndPhiNtk,i))];
    }

    fprintf(mapFile2,"AUXILIARIES:\n");
    fprintf(mapFile,"AUXILIARIES:\n");


    int numVars = solver.vars();
    int unsatCoreNumVars = unsatCoreExtractor.vars();
    // solver.write_dimacs("./debug.dimacs");
    // exit(1);
    CaDiCaL::Solver constraintSolver;
    
    map<int, map<set<int>,pair<int,int>>> ex_caseToAuxMapping; // ex_id -> {set -> {CI_id,CI_id}}
    set<set<int>> constraints;

    Aig_ManStop(origFormula);
    Aig_ManStop(deltaAndPhi_Man);
    int iter=0;

    bool verbose=true;
    cout<<"Verbose? : ";
    // cin>>verbose;
    bool freq=false;

    map<int, int> VarToInput_unsatCoreExtractor;


    // exit(1);
    vector<int> auxilaries;
    
    map<int, int> HtoZMapping;
    map<int, vector<int>> HtoSelectorMapping;
    
    map<int, int> HtoZMapping_unsatCore;
    map<int, vector<int>> HtoSelectorMapping_unsatCore;
    
    for(auto id:deps){
        int z0 = solver.vars()+1;
        int s0 = solver.vars()+2;
        int h_id = exToHMapping[id];
        int h_var = inputToVarMapping[h_id];
        
        //z0 <=> 1
        int randomNumber = (rand() % 100);

        // if(randomNumber >=0){

        //     solver.add(z0);
        // }
        // else solver.add(-z0);
        // solver.add(0);
        
        solver.add(z0);
        solver.add(0);


        // -h or z or s
        solver.add(-h_var);
        solver.add(z0);
        solver.add(s0);
        solver.add(0);
        
        //h or -z or s
        solver.add(h_var);
        solver.add(-z0);
        solver.add(s0);
        solver.add(0);
        
        HtoZMapping[h_id] = z0;
        HtoSelectorMapping[h_id].push_back(s0);
        
        // int uc_z0 = unsatCoreExtractor.vars()+1;
        // int uc_s0 = unsatCoreExtractor.vars()+2;
        // int uc_h_var = inputToVarMapping_unsatCore[h_id];
        
        // unsatCoreExtractor.add(-uc_z0);
        // unsatCoreExtractor.add(0);
        
        // unsatCoreExtractor.add(-uc_h_var);
        // unsatCoreExtractor.add(uc_z0);
        // unsatCoreExtractor.add(uc_s0);
        // unsatCoreExtractor.add(0);
        
        // unsatCoreExtractor.add(uc_h_var);
        // unsatCoreExtractor.add(-uc_z0);
        // unsatCoreExtractor.add(uc_s0);
        // unsatCoreExtractor.add(0);
        
        // HtoZMapping_unsatCore[h_id] = uc_z0;
        // HtoSelectorMapping_unsatCore[h_id].push_back(uc_s0);
    }

    set<int> posUnates = phiCNF->get_posUnates();
    for(auto d:posUnates){
        solver.add(inputToVarMapping[d]);
        solver.add(0);

        unsatCoreExtractor.add(inputToVarMapping_unsatCore[d]);
        unsatCoreExtractor.add(0);
    }

    set<int> negUnates = phiCNF->get_negUnates();
    for(auto d:negUnates){
        solver.add(-inputToVarMapping[d]);
        solver.add(0);

        unsatCoreExtractor.add(-inputToVarMapping_unsatCore[d]);
        unsatCoreExtractor.add(0);
    }


    solver.write_dimacs("./f1.dimacs");
    unsatCoreExtractor.write_dimacs("./f2.dimacs");
    // unsatCoreExtractor.write_dimacs("./before.dimacs");
    map<set<int>, int> caseToAuxMap;
    map<set<int>, int> caseToAuxMap_unsatCore;

    map<int, vector<int>> exToAuxMap;


    global_metrics.last_checkpoint=  "CEGIS_START";
    auto cegis_start = std::chrono::high_resolution_clock::now();
    while(true){
        iter++;
        global_metrics.total_cegis_iterations++;
        //check for sat
        freq=false;
        
        for(auto id:deps){
            int h_id = exToHMapping[id];

            vector<int> selectors = HtoSelectorMapping[h_id];
            int sz = selectors.size();
            for(int i=0;i<sz-1;i++){
                // printf("Assuming %d\n", selectors[i]);
                solver.assume(selectors[i]);
            }
            // printf("Assuming %d\n", -selectors[sz-1]);
            solver.assume(-selectors[sz-1]);
        }
        // solver.write_dimacs("./f1_assumed.dimacs");

        int status = solver.solve();
        if(iter%1000==0){
            freq=true;
        }

        if(verbose && freq) cout<<"**************           ITER: "<<iter<<"        ******************"<<endl;

        // if(verbose && freq){
        //     string dumpName = "./debug_"+to_string(iter)+".dimacs";

        //     solver.write_dimacs(dumpName.c_str());
        // }

        if(status == 0){
            cerr<<"Dunno what happened\n";
            return 1;
        }

        if(status == CaDiCaL::UNSATISFIABLE){
            global_metrics.last_checkpoint = "CEGIS_END";
            cout<<"UNSAT BUT WHY?"<<endl;
            for(int asgNo=0;asgNo<1;asgNo++){
                int constrStatus = constraintSolver.solve();
                auto cegis_end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> time_elapsed= cegis_end-cegis_start;
                if(constrStatus == CaDiCaL::UNSATISFIABLE){
                    printf("Couldn't satisfy constraints, total assignments generated: %d\n",asgNo);
                    cout<<"UNSATISFIABLE\n";
                    global_metrics.execution_status = "UNSATISFIABLE";
                    global_metrics.total_cegis_time = time_elapsed.count();
                    Abc_Stop();
                    exit(1);
                }

                if(constrStatus == CaDiCaL::SATISFIABLE){

                    global_metrics.execution_status = "SATISFIABLE";
                    cout<<"SATISFIABLE\n";
                    global_metrics.total_cegis_time = time_elapsed.count();
                    Abc_Stop();
                    return 0;

                    map<int, int> cex_aux;

                    for(auto e:auxilaries){
                        int val = constraintSolver.val(inputToVarMapping[e]);
                        cex_aux[e] = val>0?1:0;
                        
                    }
                    string filename = "./assignments_NoUnit/asg_"+ to_string(asgNo) +".txt";
                    FILE* asgFile = fopen(filename.c_str(),"w");
                    for(auto p:ex_caseToAuxMapping){
                        int d_var = p.first;
                        auto cases = p.second;
                        fprintf(asgFile,"D Variable: %d\n",d_var);
    
                        for(auto p2:cases){
                            set<int> currCase = p2.first;
                            int aux = p2.second.first;
                            if(cex_aux.find(aux)==cex_aux.end()){
                                cerr<<"Error in aux map\n";
                                exit(1);
                            }
                            for(auto e:currCase){
                                fprintf(asgFile,"%d ",e);
                            }
                            fprintf(asgFile,"   =>     H_Val: %d\n",cex_aux[aux]);
                        }
                        fprintf(asgFile,"**************************\n");
                    }

                    vector<int> newConstraint;
                    vector<int> curr_auxilaries = exToAuxMap[19];
                    for(auto e:curr_auxilaries){
                        int val = constraintSolver.val(inputToVarMapping[e]);
                        if(val>0){
                            newConstraint.push_back(-e);
                        }
                        else{
                            newConstraint.push_back(e);
                        }
                    }

                    for(auto e:newConstraint){
                        if(e>0){
                            constraintSolver.add(inputToVarMapping[e]);
                        }
                        else{
                            constraintSolver.add(-inputToVarMapping[-e]);
                        }
                    }
                    constraintSolver.add(0);
                }
                // else{
                //     printf("Ran out of possible assignments, terminating. \n");
                //     printf("Total assignments generated: %d\n",asgNo+1);
                //     return 0;
                // }
            }


                cout<<"HURRAY\n";
                return 0;
            // }
            // if(constrStatus == 20){
            //     cout<<iter<<endl;
            //     cout<<"CONSTRAINT UNSAT :(\n";
            //     return 0;
            // }
            // cout<<"TIMEOUT ON CONSTR\n";
            return 1;
        }

        assert(status==10);
        //The formula is sat. Work on the counter example.
        bool changeFlag = false;
        int cex[numAigInputs];
        for(int i=1;i<=numAigInputs;i++){
            int val = solver.val(inputToVarMapping[i]);
            cex[i-1]=val>0? 1:0;
        }


        if(verbose && freq) {cout << "CEX : ";

        for (int i = 0; i < numAigInputs; i++)
        {
            cout << cex[i] << " ";
        }
        cout << endl;}

        // cin>>mySIG;

        /*TODO: 
            1. Add the implications to the "solver". (DONE)
            2. Build the constraint formula, for the "unsatCoreExtractor" using all the auxiliaries. (DONE)
            3. Add this constraint as assumption in the "unsatCoreExtractor".
            4. Get the set of failed assumptions
            5. Using those, negate and build the constraintFormula for "solver". Add this constraint clause to all the three solvers.
        */

        vector<int> currConstraint;


        vector<int> currAssumptions;

        for(auto id:deps){
            if(cex[id-1]>0){
                currAssumptions.push_back(id);
            }
            else{
                currAssumptions.push_back(-id);
            }
        }

        set<int> univs = phiCNF->get_universals();
        vector<int> univAssumptions;
        for(auto id:univs){
            if(cex[id-1]>0){
                // currConstraint.insert(-inputToVarMapping[aux]);
                univAssumptions.push_back(id);

            }
            else{
                // currConstraint.insert(inputToVarMapping[aux]);
                univAssumptions.push_back(-id);
            }
        }

        // cout<<"Universal Assumptions:\n";
        for(auto e:univAssumptions){
            // cout<<e<<endl;
            if(e>0){
                unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);

            }
            else{
                // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
            }
            // unsatCoreExtractor.add(0);
        }
        // cout<<"Auxiliary Assumptions:\n";
        for(auto e:currAssumptions){
            // cout<<e<<endl;
            if(e>0){
                // cout<<inputToVarMapping_unsatCore[e]<<endl;
                unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);

            }
            else{
                // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
            }
            // unsatCoreExtractor.add(0);
        }


        int unsatCoreStatus = unsatCoreExtractor.solve();

        if(unsatCoreStatus == CaDiCaL::SATISFIABLE){
            cerr<<"Theres a problem... unsatCore is sat after adding the assumptions\n";
            unsatCoreExtractor.write_dimacs("./sat_core.dimacs");
            int cex[numAigInputs];
            for(int i=1;i<=numAigInputs;i++){
                int val = unsatCoreExtractor.val(inputToVarMapping_unsatCore[i]);
                cex[i-1]=val>0? 1:0;
            }


            if(verbose && freq) {cout << "CEX : ";

            for (int i = 0; i < numAigInputs; i++)
            {
                cout << cex[i] << " ";
            }
            cout << endl;}

            exit(1);
        }
        set<int> unsatCoreLits;
        if(unsatCoreStatus == CaDiCaL::UNSATISFIABLE){
            // unsatCoreExtractor.write_dimacs("./debug.dimacs");
            // cout<<"Unsat Core D Variables:\n";
            for(auto e:currAssumptions){
                if(e>0){
                    if(unsatCoreExtractor.failed(inputToVarMapping_unsatCore[e])){
                        // currConstraint.insert(-e);
                        unsatCoreLits.insert(e);
                        // cout<<e<<endl;
                    }
                }
                else{
                    if(unsatCoreExtractor.failed(-inputToVarMapping_unsatCore[-e])){
                        // currConstraint.insert(-e);
                        unsatCoreLits.insert(e);
                        // cout<<e<<endl;
                    }
                }
            }

            // cout<<"Unsat Core A Variables:\n";
            for(auto e:univAssumptions){
                if(e>0){
                    if(unsatCoreExtractor.failed(inputToVarMapping_unsatCore[e])){
                        // cout<<e<<endl;
                    }
                }
                else{
                    if(unsatCoreExtractor.failed(-inputToVarMapping_unsatCore[-e])){
                        // cout<<e<<endl;
                    }
                }
            }
        }
        else{
            cerr<<"SOMETHING WENT WRONG, timeout occured for unsatCoreExtractor\n";
            exit(1);
        }

        // unsatCoreLits.insert(27);

        bool setMinimized = false;
        vector<int> removed_lits;
        while(true){

            setMinimized=false;
            // set<int> tmp;
            for(auto lit: unsatCoreLits){
                // printf("Trying to remove %d\n",lit);
                for(auto e:univAssumptions){
                    // cout<<e<<endl;
                    if(e>0){
                        unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);
        
                    }
                    else{
                        // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                        unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
                    }
                    // unsatCoreExtractor.add(0);
                }            
                for(auto e:unsatCoreLits){
                    if(e==lit) continue;
                    if(e>0){
                        // cout<<inputToVarMapping_unsatCore[e]<<endl;
                        unsatCoreExtractor.assume(inputToVarMapping_unsatCore[e]);
        
                    }
                    else{
                        // cout<<-inputToVarMapping_unsatCore[-e]<<endl;
                        unsatCoreExtractor.assume(-inputToVarMapping_unsatCore[-e]);
                    }
                }
    
                int unsatCoreStatus = unsatCoreExtractor.solve();

                if(unsatCoreStatus == CaDiCaL::SATISFIABLE){
                    // printf("%d in UNSAT Core\n",lit);
                    int cex[numOrigInputs];
                    for(int i=1;i<=numOrigInputs;i++){
                        int val = unsatCoreExtractor.val(inputToVarMapping_unsatCore[i]);
                        cex[i-1]=val>0? 1:0;
                    }


                    if(verbose && freq) {cout << "CEX : ";

                    for (int i = 0; i < numOrigInputs; i++)
                    {
                        cout << cex[i] << " ";
                    }
                    cout << endl;}
                    // tmp.insert(lit);
                }
                else if(unsatCoreStatus == CaDiCaL::UNSATISFIABLE){
                    // printf("Removed %d\n",lit);
                    // unsatCoreLits
                    unsatCoreLits.erase(lit);
                    removed_lits.push_back(lit);
                    setMinimized=true;
                    break;
                }
                else{
                    cerr<<"Timeout while minimizing UNSAT CORE\n";
                    break;
                }   
            }
            // unsatCoreLits;
            if(!setMinimized){
                break;
            }
        }
        if(verbose && freq) cout<<"Printing A_i and B_i for removed lits:\n";
        for(auto e:removed_lits){
            int id = abs(e);
            
            int a_i = Abc_NtkVerifySimulatePattern(A_Ntk[id], cex)[0];
            int b_i = Abc_NtkVerifySimulatePattern(B_Ntk[id], cex)[0];
            printf("id: %d | a_i: %d | b_i: %d\n", id, a_i, b_i);
        }

        // cout<<"UNSAT CORE D Variables:\n";
        // for(auto e:unsatCoreLits){
        //     cout<<e<<endl;
        // }

        vector<vector<int>> implicationClauses;
        for(auto e:unsatCoreLits){
            int id = abs(e);
            int a_i = Abc_NtkVerifySimulatePattern(A_Ntk[id], cex)[0];
            int b_i = Abc_NtkVerifySimulatePattern(B_Ntk[id], cex)[0];

            // if(verbose) printf("var: %d => a_i: %d | b_i: %d\n",id,a_i,b_i);
            if(!(a_i==0 && b_i==1)){
               if(verbose && freq) printf("AiBi Check Failed: %d\n",id);
                continue;
            }

            if(verbose && freq) cout<<"Dependent Var_ AiBi check pass: "<<id<<endl;

            set<int> depVal;
            set<int> depSet = phiCNF->get_dependencySet(id);
            // if(id==13) cout<<"DBBBB: size: "<<depSet.size()<<endl;
            for (auto dep : depSet)
            {
                if (cex[dep - 1] == 0)
                {
                    depVal.insert(-dep);
                }
                else
                {
                    depVal.insert(dep);
                }
                // depVal.insert(cex[dep-1]);
            }

            if(ex_caseToAuxMapping[id].find(depVal)==ex_caseToAuxMapping[id].end()){
                
                changeFlag = true;
                numAigInputs++;
                int newAux = numAigInputs;
                int cnfVar = solver.vars()+1;
                inputToVarMapping[newAux] = cnfVar;
                fprintf(mapFile,"INPUT %d , var map: %d\n", newAux, cnfVar);
                exToAuxMap[id].push_back(newAux);
                
                auxilaries.push_back(newAux);
                int unsatCoreCnfVar = unsatCoreExtractor.vars()+1;
                inputToVarMapping_unsatCore[newAux] = unsatCoreCnfVar;
                fprintf(mapFile2, "INPUT %d , var map: %d\n", newAux, unsatCoreCnfVar);
                VarToInput_unsatCoreExtractor[unsatCoreCnfVar] = newAux;

if(verbose && freq)                cout<<"Dependent Var new Aux created: "<<id<<endl;


                //Check if auxilary for this depVal exists or Not, if it does, use it else create one.


                if(caseToAuxMap.find(depVal)==caseToAuxMap.end()){
                    int newCaseVar = solver.vars()+2;

                    // dep => var === ~dep or var
                    vector<int> c1;
                    for(auto d:depVal){
                        if(d>0){
                            c1.push_back(-inputToVarMapping[d]);
                        }
                        else{
                            c1.push_back(inputToVarMapping[-d]);
                        }
                    }
                    c1.push_back(newCaseVar);
                    c1.push_back(0);

                    for(auto e:c1){
                        solver.add(e);
                    }


                    // var => dep ---> ~ var OR dep
                    for(auto d:depVal){
                        solver.add(-newCaseVar);
                        if(d>0){
                            solver.add(inputToVarMapping[d]);
                        }
                        else{
                            solver.add(-inputToVarMapping[-d]);
                        }
                        solver.add(0);
                    }

                    caseToAuxMap[depVal] = newCaseVar;
                }

                int caseVar = caseToAuxMap[depVal];

                int h_id = exToHMapping[id];
                int hVar = inputToVarMapping[h_id];
                int zVar = HtoZMapping[h_id];

                int newZ = solver.vars()+3;
                int newS = solver.vars()+4;

                // newZ = ite(caseVar, newAux,zVar)
                //adding clause 1
                solver.add(-caseVar);
                solver.add(newZ);
                solver.add(-cnfVar);
                solver.add(0);

                //adding clause 2
                solver.add(-caseVar);
                solver.add(-newZ);
                solver.add(cnfVar);
                solver.add(0);

                //adding clause 3
                solver.add(caseVar);
                solver.add(newZ);
                solver.add(-zVar);
                solver.add(0);

                //adding clause 4
                solver.add(caseVar);
                solver.add(-newZ);
                solver.add(zVar);
                solver.add(0);

                //Now we will add h<=> newZ if ~selector
                //adding h or -newZ or newS
                solver.add(hVar);
                solver.add(-newZ);
                solver.add(newS);
                solver.add(0);

                //adding -h or newZ or newS
                solver.add(-hVar);
                solver.add(newZ);
                solver.add(newS);
                solver.add(0);

                HtoZMapping[h_id] = newZ;
                HtoSelectorMapping[h_id].push_back(newS);

                
                if(cex[id-1]>0){
                    currConstraint.push_back(-newAux);
                    // currAssumptions.push_back(newAux);
                }
                else{
                    currConstraint.push_back(newAux);
                    // currAssumptions.push_back(-newAux);
                }

                ex_caseToAuxMapping[id][depVal]= {newAux, newAux};
            }
            else{
                changeFlag=true;
                int aux = ex_caseToAuxMapping[id][depVal].first;
                if(cex[id-1]>0){
                    currConstraint.push_back(-aux);
                    // currAssumptions.push_back(aux);

                }
                else{
                    currConstraint.push_back(aux);
                    // currAssumptions.push_back(-aux);
                }
            }

        }

    

        if(!changeFlag){
            cout<<"No change occured...."<<endl;
            Abc_Stop();
            return 1;
        }
        if(verbose && freq) cout<<"adding constraint clause...\n";
        for(auto e:currConstraint){
            
            if(e>0) {
                solver.add(inputToVarMapping[e]);
                if(verbose && freq) cout<<e<<" ";
            }
            else {
                solver.add(-inputToVarMapping[-e]);
                if(verbose && freq) cout<<e<<" ";
            }
        }
        if(verbose && freq)cout<<endl;
        solver.add(0);
        // cout<<"Printing current constraint:\n";
        for(auto e:currConstraint){
            if(e>0) {
                constraintSolver.add(inputToVarMapping[e]);
            }
            else constraintSolver.add(-inputToVarMapping[-e]);
            // cout<<e<<endl;
        }
        constraintSolver.add(0);
        freq=false;


    }

    
    // return 0;

    

    Abc_Stop();

    return 0;
}