#include "helper.h"

using namespace std;

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
bool didManthan=false;
char* manthanFile;
char* manthanMapping;


struct SkolemInfo{
    int nPos, nNeg;
    bool defaultVal;

    vector<set<int>> posCases;
    vector<set<int>> negCases;
};


int main(int argc, char* argv[]){
    Abc_Start();
    srand(time(0));

    CaDiCaL::Solver solver;
    int sig;
    

    string inpPath = argv[1];
    FS::path phiPath(inpPath);
    DQCNF *phiCNF = new DQCNF(phiPath.string());

    string reduced_dqbf_file = argv[2];
    string skolem_file = argv[3];


    ifstream skolem(skolem_file);


    // ifstream reduced_dqbf(reduced_dqbf_file);

    // vector<set<int>> reduced_clauses;
    
    // if(reduced_dqbf.is_open()){
    //     int num_clauses;
    //     reduced_dqbf>>num_clauses;
        
    //     for(int i=0;i<num_clauses;i++){
    //         set<int> clause;
    //         int lit;
    //         while(true){
    //             reduced_dqbf>>lit;
    //             if(lit==0) break;
    //             clause.insert(lit);
    //         }
    //         reduced_clauses.push_back(clause);
    //     }
    // }

    // DQCNF* reduced_dqcnf = new DQCNF(phiCNF->get_universals(), phiCNF->get_existentials(),phiCNF->get_deps(),
    //                                 phiCNF->getNumInputs(),reduced_clauses.size(),reduced_clauses,phiCNF->get_dependency(),phiCNF->get_constAssumption(),
    //                             phiCNF->filename);

    Aig_Man_t* phiMan = phiCNF->genAIGMan();


    // Aig_ManShow(phiMan,0,NULL);
    // cin>>sig;

    // Aig_ManShow(phiMan,0,NULL);
    // cin>>sig;

    // string reduced_dqbf_file = argv[2];
    // string skolem_file = argv[3];


    // ifstream skolem(skolem_file);


    map<int, bool> constAssignments;
    map<int, SkolemInfo> skolemFunctions;
    if(skolem.is_open()){
        int const_outs, skolem_outs;
        skolem>>const_outs>>skolem_outs;
        printf("%d %d\n", const_outs, skolem_outs);

        for(int i=0;i<const_outs;i++){
            int var, assumption;
            skolem>>var>>assumption;
            constAssignments[var] = assumption;
        }

        for(int i=0;i<skolem_outs;i++){
            int var, nPos, nNeg, defaultVal;
            skolem>>var>>nPos>>nNeg>>defaultVal;

            vector<set<int>> posCases;
            for(int i=0;i<nPos;i++){
                set<int> tmp;
                while(true){
                    int lit;
                    skolem>>lit;
                    if(lit==0) break;
                    tmp.insert(lit);
                }
                posCases.push_back(tmp);
            }

            vector<set<int>> negCases;
            for(int i=0;i<nNeg;i++){
                set<int> tmp;
                while(true){
                    int lit;
                    skolem>>lit;
                    if(lit==0) break;
                    tmp.insert(lit);
                }
                negCases.push_back(tmp);
            }

            SkolemInfo s_info;
            s_info.defaultVal=defaultVal;
            s_info.posCases = posCases;
            s_info.negCases = negCases;
            s_info.nPos=nPos;
            s_info.nNeg=nNeg;

            skolemFunctions[var]=s_info;
        }
    }

    cout<<"Setting constant values: \n";
    for(auto p:constAssignments){
        
        int var = p.first;
        int asg=p.second;

        cout<<var<<" "<<asg<<endl;

        Aig_Obj_t* newOut = Aig_SubstituteConst(phiMan, Aig_ManCo(phiMan,0),Aig_ObjId(Aig_ManCi(phiMan,var-1)),asg);

        Aig_ObjCreateCo(phiMan, newOut);

        Aig_ObjDisconnect(phiMan, Aig_ManCo(phiMan, 0));
        Aig_ObjConnect(phiMan, Aig_ManCo(phiMan, 0), Aig_ManConst0(phiMan), NULL);
        Aig_ManCoCleanup(phiMan);
        Aig_ManCleanup(phiMan);

        if (Aig_ManCoNum(phiMan) == 0)
        {
            Aig_ObjCreateCo(phiMan, Aig_ManConst0(phiMan));
        }

        // Aig_ManShow(phiMan,0,NULL);
        // cin>>sig;
    }

    // Aig_ManShow(phiMan,0,NULL);
    //     cin>>sig;


    ifstream reduced_dqbf(reduced_dqbf_file);

    vector<set<int>> reduced_clauses;
    
    if(reduced_dqbf.is_open()){
        int num_clauses;
        reduced_dqbf>>num_clauses;
        
        for(int i=0;i<num_clauses;i++){
            set<int> clause;
            int lit;
            while(true){
                reduced_dqbf>>lit;
                if(lit==0) break;
                clause.insert(lit);
            }
            reduced_clauses.push_back(clause);
        }
    }

    DQCNF* reduced_dqcnf = new DQCNF(phiCNF->get_universals(), phiCNF->get_existentials(),phiCNF->get_deps(),
                                    phiCNF->getNumInputs(),reduced_clauses.size(),reduced_clauses,phiCNF->get_dependency(),phiCNF->get_constAssumption(),
                                phiCNF->filename);
    
    map<int, DQCNF *> projectedPhis;
    map<int, Aig_Man_t *> projectedMans;

    auto deps = reduced_dqcnf->get_all_edvars();

    for (auto id : deps)
    {

        if(constAssignments.find(id)!=constAssignments.end()) continue;
        // cout<<id<<endl;
        DQCNF *tCNF = reduced_dqcnf->getProjection(id);
        projectedPhis[id] = tCNF;
        Aig_Man_t *tMan = tCNF->genAIGMan();

        cout << "Generated DQCNF and Aig_Man_t Objects for Projected PHI_" << id << endl;
        // Aig_ManShow(tMan,0,NULL);
        // cin>>sig;
        tMan = compressAig(tMan);
        projectedMans[id] = tMan;


        // if(Aig_ObjFanin0(Aig_ManCo(tMan,0)) ==  Aig_ManConst1(tMan) 
        //     && Aig_ObjFaninC0(Aig_ManCo(tMan,0))){
        //         printf("Phi_i is const 0 for id: %d\n",id);
        //     }

        // Aig_ManShow(tMan,0,NULL);
        // cin>>mySIG;
        // exit(1);
        // Aig_ManShow(tMan,0,NULL);
        // int q;
        // cin>>q;
    }

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

    map<int, Aig_Man_t *> A_Man;
    map<int, Aig_Man_t *> B_Man;
    map<int, Abc_Ntk_t *> A_Ntk;
    map<int, Abc_Ntk_t *> B_Ntk;


    for (auto id : deps)
    {
        if(constAssignments.find(id)!=constAssignments.end()) continue;

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


    
    map<int, pair<int, int>> exToOutMapping;
    map<int, int> exToHMapping;
    for (auto id : deps)
    {
        
        if(constAssignments.find(id)!=constAssignments.end()) continue;
        Abc_Ntk_t *phiNtk = Abc_NtkFromAigPhase(phiMan);

        int numPhiInputs = Aig_ManCiNum(phiMan);

        while(Aig_ManCiNum(A_Man[id]) < numPhiInputs){
            Aig_ObjCreateCi(A_Man[id]);
            Aig_ObjCreateCi(B_Man[id]);
        }

        Abc_Ntk_t *ANtk = Abc_NtkFromAigPhase(A_Man[id]);
        Abc_Ntk_t *BNtk = Abc_NtkFromAigPhase(B_Man[id]);

        printf("pi_1 : %d | pi_2 : %d\n", Abc_NtkPiNum(phiNtk), Abc_NtkPiNum(ANtk));

        Abc_NtkAppend(phiNtk, ANtk, 1);
        Abc_NtkAppend(phiNtk, BNtk, 1);

        // Abc_NtkAppend(phi_ntk, ANtk, 1);
        // Abc_NtkAppend(phi_ntk, BNtk, 1);

        cout << "Appended Basis for EX_VAR: " << id << endl;

        exToOutMapping[id] = {Abc_NtkCoNum(phiNtk) - 2, Abc_NtkCoNum(phiNtk) - 1};
        Aig_ManStop(A_Man[id]);
        Aig_ManStop(B_Man[id]);
        

        phiMan = Abc_NtkToDar(phiNtk,0,0);

        Aig_Obj_t *outA = Aig_ManCo(phiMan, exToOutMapping[id].first)->pFanin0;
        Aig_Obj_t *outB = Aig_ManCo(phiMan, exToOutMapping[id].second)->pFanin0;

        Aig_Obj_t *currH = Aig_ObjCreateCi(phiMan);

        exToHMapping[id] = Aig_ManCiNum(phiMan);

        Aig_Obj_t *HAndB = Aig_And(phiMan, currH, outB);

        Aig_Obj_t *defin = Aig_Or(phiMan, outA, HAndB); // A v (h ^ B)

        //substitute y_i with defin
        // Aig_ManShow(phiMan,0,NULL);
        // int q;
        // cin>>q;

        Aig_Obj_t* newOut = Aig_Substitute(phiMan,Aig_ManCo(phiMan,0), Aig_ObjId(Aig_ManCi(phiMan,id-1)), defin);
        Aig_ObjCreateCo(phiMan,newOut);



        int numOuts = Aig_ManCoNum(phiMan);
        for (int i = 0; i < numOuts - 1; i++)
        {
            Aig_ObjDisconnect(phiMan, Aig_ManCo(phiMan, i));
            Aig_ObjConnect(phiMan, Aig_ManCo(phiMan, i), Aig_ManConst0(phiMan), NULL);
        }

        Aig_ManCoCleanup(phiMan);
        Aig_ManCleanup(phiMan);



        if (Aig_ManCoNum(phiMan) == 0)
        {
            Aig_ObjCreateCo(phiMan, Aig_ManConst0(phiMan));
        }

        //  Aig_ManShow(phiMan,0,NULL);
        // // int q;
        // cin>>q;


    }


    // Aig_ManShow(phiMan,0,NULL);
    // cin>>sig;
    //Now build the function for h_is and substitute them back in

    for(auto id:deps){
        if(constAssignments.find(id)!=constAssignments.end()) continue;

        assert(skolemFunctions.find(id)!=skolemFunctions.end());

        SkolemInfo s_info = skolemFunctions[id];
        cout<<"Building H_i for id: "<<id<<endl;

        Aig_Obj_t* funcH = Aig_ManConst1(phiMan);

        //first build the posCases formula -> (C1 v C2 v C3 v ...)
        Aig_Obj_t* posCasesNode = Aig_ManConst0(phiMan);
        for(auto asgCase : s_info.posCases){
            Aig_Obj_t* currCaseNode = Aig_ManConst1(phiMan);

            for(auto lit:asgCase){
                cout<<lit<<endl;
                Aig_Obj_t* litNode = lit<0? Aig_Not(Aig_ManCi(phiMan,-lit-1)):Aig_ManCi(phiMan,lit-1);
                currCaseNode = Aig_And(phiMan, currCaseNode, litNode);
                // Aig_ManShow(phiMan,0,NULL);
                // cin>>sig;
            }

            posCasesNode = Aig_Or(phiMan, posCasesNode, currCaseNode);
            // Aig_ManShow(phiMan,0,NULL);
            // cin>>sig;
        }

        if(s_info.defaultVal==true){
            //default val is 1, so we need to make the following change :
                // posCaseNode -> posCaseNode v (~C1 ^ ~C2 ^ ~C3 ^.... all cases)
            
            Aig_Obj_t* conjNode = Aig_ManConst1(phiMan);

            for(auto asgCase:s_info.posCases){
                Aig_Obj_t* currCaseNode = Aig_ManConst1(phiMan);

                for(auto lit:asgCase){
                    cout<<lit<<endl;
                    Aig_Obj_t* litNode = lit<0? Aig_Not(Aig_ManCi(phiMan, -lit-1)) : Aig_ManCi(phiMan, lit -1);
                    currCaseNode = Aig_And(phiMan, currCaseNode, litNode);
                    // Aig_ManShow(phiMan,0,NULL);
                    // cin>>sig;
                }

                conjNode = Aig_And(phiMan, conjNode, Aig_Not(currCaseNode));
                // Aig_ManShow(phiMan,0,NULL);
                // cin>>sig;
            }

            for(auto asgCase:s_info.negCases){
                Aig_Obj_t* currCaseNode = Aig_ManConst1(phiMan);

                for(auto lit:asgCase){
                    cout<<lit<<endl;
                    Aig_Obj_t* litNode = lit<0? Aig_Not(Aig_ManCi(phiMan, -lit-1)) : Aig_ManCi(phiMan, lit -1);
                    currCaseNode = Aig_And(phiMan, currCaseNode, litNode);
                    // Aig_ManShow(phiMan,0,NULL);
                    // cin>>sig;
                }
                conjNode = Aig_And(phiMan, conjNode, Aig_Not(currCaseNode));
                // Aig_ManShow(phiMan,0,NULL);
                // cin>>sig;
            }

            posCasesNode = Aig_Or(phiMan, posCasesNode, conjNode);
            // Aig_ManShow(phiMan,0,NULL);
            // cin>>sig;
        }

        funcH = Aig_And(phiMan, funcH, posCasesNode);
        // Aig_ManShow(phiMan,0,NULL);
        // cin>>sig;
        //now, modify funcH as follows to account for negCases
        // funcH -> funcH ^ ~C4 ^ ~C5 ^ ...

        for(auto asgCase:s_info.negCases){

            Aig_Obj_t* currCaseNode = Aig_ManConst1(phiMan);

            for(auto lit:asgCase){
                Aig_Obj_t* litNode = lit<0? Aig_Not(Aig_ManCi(phiMan, -lit-1)) : Aig_ManCi(phiMan, lit -1);
                currCaseNode = Aig_And(phiMan, currCaseNode, litNode);
                // Aig_ManShow(phiMan,0,NULL);
                // cin>>sig;
            }
            funcH = Aig_And(phiMan, funcH, Aig_Not(currCaseNode));
            // Aig_ManShow(phiMan,0,NULL);
            // cin>>sig;
        }

        if(s_info.defaultVal==false){
            //taken care of by default
            /*
                eg:
                (c1 v c2) ^ ~c3 ^~c4
                if c=c5, then
                    c1=0, c2=0,c3=0,c4=0
                    (0 v 0) ^ 1 ^ 1 = 0
            */
        }

        //now we have funcH, we just have to plug it in place of h_i

        Aig_Obj_t* newOut = Aig_Substitute(phiMan, Aig_ManCo(phiMan,0), exToHMapping[id], funcH);



        Aig_ObjCreateCo(phiMan, newOut);

        // Aig_ManShow(phiMan,0,NULL);
        //     cin>>sig;

        Aig_ObjDisconnect(phiMan, Aig_ManCo(phiMan, 0));
        Aig_ObjConnect(phiMan, Aig_ManCo(phiMan, 0), Aig_ManConst0(phiMan), NULL);
        Aig_ManCoCleanup(phiMan);
        Aig_ManCleanup(phiMan);

        if (Aig_ManCoNum(phiMan) == 0)
        {
            Aig_ObjCreateCo(phiMan, Aig_ManConst0(phiMan));
        }


        // Aig_ManShow(phiMan,0,NULL);
        // cin>>sig;

    }

    //now, we take the negated formula, give it to solver. It should return UNSAT (expected)

    Aig_ManCo(phiMan, 0)->pFanin0 = Aig_Not(Aig_ManCo(phiMan, 0)->pFanin0);

    Cnf_Dat_t* pCnf;
    Abc_Ntk_t* phiNtk = Abc_NtkFromAigPhase(phiMan);

    pCnf = myDarToCnf(phiNtk,NULL,0,0,1);


    int * pLit, * pStop;
    for(int i=0;i<pCnf->nClauses;i++){
        for(pLit=pCnf->pClauses[i],pStop=pCnf->pClauses[i+1];pLit<pStop;pLit++){
            // printf("%d ", Cnf_Lit2Var2(*pLit));
            solver.add(Cnf_Lit2Var2(*pLit));
        }
        // printf("0\n");
        solver.add(0);
    }

    int status = solver.solve();

    if(status == CaDiCaL::UNSATISFIABLE){
        cout<<"Skolem functions are correct!\n";
        
    }
    else{
        cout<<"Negation is SAT, some error :( \n";
        
    }

    return 0;



    // Aig_Man_t *tMan = Abc_NtkToDar(phiNtk, 0, 0);
    


    // for (auto id : deps)
    // {
    //     if(constAssignments.find(id)!=constAssignments.end()) continue;

    //     Aig_Obj_t *outA = Aig_ManCo(tMan, exToOutMapping[id].first)->pFanin0;
    //     Aig_Obj_t *outB = Aig_ManCo(tMan, exToOutMapping[id].second)->pFanin0;

    //     // Aig_Obj_t *outA2 = Aig_ManCo(tMan2, exToOutMapping[id].first)->pFanin0;
    //     // Aig_Obj_t *outB2 = Aig_ManCo(tMan2, exToOutMapping[id].second)->pFanin0;

    //     Aig_Obj_t *currH = Aig_ObjCreateCi(tMan);
        
    //     // Aig_Obj_t* currH2 = Aig_ObjCreateCi(tMan2);

    //     exToHMapping[id] = Aig_ManCiNum(tMan);
    //     Aig_Obj_t *HAndB = Aig_And(tMan, currH, outB);
    //     Aig_Obj_t *defin = Aig_Or(tMan, outA, HAndB); // A v (h ^ B)

    //     //substitute y_i with defin

    //     Aig_Obj_t* newOut = Aig_Substitute(tMan,Aig_ManCo)


    //     // Aig_Obj_t *yImpDefin = Aig_Or(tMan, Aig_Not(Aig_ManCi(tMan, id - 1)), defin);
    //     // Aig_Obj_t *definImpY = Aig_Or(tMan, Aig_Not(defin), Aig_ManCi(tMan, id - 1));

    //     // Aig_Obj_t *currDelta = Aig_And(tMan, yImpDefin, definImpY);

    //     // delta = Aig_And(tMan,delta,currDelta);

    //     // Aig_Obj_t *HAndB2 = Aig_And(tMan2, currH2, outB2);
    //     // Aig_Obj_t *defin2 = Aig_Or(tMan2, outA2, HAndB2);
    //     // Aig_Obj_t *yImpDefin2 = Aig_Or(tMan2, Aig_Not(Aig_ManCi(tMan2, id - 1)), defin2);
    //     // Aig_Obj_t *definImpY2 = Aig_Or(tMan2, Aig_Not(defin2), Aig_ManCi(tMan2, id - 1));

    //     // Aig_Obj_t *currDelta2 = Aig_And(tMan2, yImpDefin2, definImpY2);

    //     // delta2 = Aig_And(tMan2, delta2, currDelta2);
    // }

    return 0;
}