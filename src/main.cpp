
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

// Abc_Frame_t *pAbc = NULL;

// int numX, numY;
map<int, vector<int>> dependencies;

int main(int argc, char *argv[])
{

    int mySIG;
    Abc_Start();

    // ensure phi.dqdimacs is taken as input

    if (argc != 2)
    {
        cerr << "Usage ./bin/main <path to PHI.dqdimacs file>\n";
        return 1;
    }

    // Aig_Man_t* mm = Aig_ManStart(0);
    // Aig_ObjCreateCo(mm,Aig_ManConst0(mm));
    // Aig_ManShow(mm,0,NULL);
    // exit(1);

    main_time_start = TIME_NOW;

    string inpPath = argv[1];
    FS::path phiPath(inpPath);
    DQCNF *phiCNF = new DQCNF(phiPath.string());
    cout << "Read the file\n";
    Aig_Man_t *phi_Man = phiCNF->genAIGMan();
    // Aig_ManShow(phi_Man,0,NULL);
    // int xx;
    // cin>>xx;
    // exit(1);
    cout << "Generated Manager for PHI\n";
    Abc_Ntk_t *phi_Ntk = Abc_NtkFromAigPhase(phi_Man);

    auto exs = phiCNF->get_existentials();

    TIME_MEASURE_START
    cout << "Generating DQCNF and Aig_Man_t Objects for Projected PHI_i" << endl;
    map<int, DQCNF *> projectedPhis;
    map<int, Aig_Man_t *> projectedMans;
    for (auto id : exs)
    {
        DQCNF *tCNF = phiCNF->getProjection(id);
        projectedPhis[id] = tCNF;
        Aig_Man_t *tMan = tCNF->genAIGMan();
        projectedMans[id] = tMan;
        cout << "Generated DQCNF and Aig_Man_t Objects for Projected PHI_" << id << endl;
        // exit(1);
        // Aig_ManShow(tMan,0,NULL);
        // int q;
        // cin>>q;
    }
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
        // Aig_ManShow(tMan,0,NULL);
        // int q;
        // cin>>q;
        // tMan = compressAig(tMan);
        // tMan = compressAig(tMan);

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
        // Aig_ManShow(tMan,0,NULL);
        // // int q;
        // cin>>q;
        // tMan = compressAig(tMan);
        // tMan = compressAig(tMan);

        phi_0_Man[p.first] = tMan;
    }
    cout<<"DONE. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
    // cout << "****   DONE!   *****\n";


    TIME_MEASURE_START
    cout << "Generating Basis Functions...\n";

    map<int, Aig_Man_t *> A_Man;
    map<int, Aig_Man_t *> B_Man;
    map<int, Abc_Ntk_t *> A_Ntk;
    map<int, Abc_Ntk_t *> B_Ntk;

    for (auto id : exs)
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
        // tMan = comcompresspressAig(tMan);

        A_Man[id] = tMan;
        A_Ntk[id] = Abc_NtkFromAigPhase(tMan);
        // Aig_ManShow(tMan,0,NULL);
        // int q;
        // cin>>q;
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
        // tMan = compressAig(tMan);
        // tMan = compressAig(tMan);

        B_Man[id] = tMan;
        B_Ntk[id] = Abc_NtkFromAigPhase(tMan);
        // Aig_ManShow(tMan,0,NULL);
        // // int q;
        // cin>>q;
    }
    cout<<"DONE. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
    // cout << "****   DONE!   *****\n";

    cout << "Generating AIG Manager for negPHI...\n";
    Aig_Man_t *negPhi_Man = Aig_ManDupOrdered(phi_Man);
    Aig_ManCo(negPhi_Man, 0)->pFanin0 = Aig_Not(Aig_ManCo(negPhi_Man, 0)->pFanin0);

    cout << "Creating the formula DELTA (AND) !PHI\n";

    Aig_Man_t *defsMan = Aig_ManStart(0);

    // Append all basis into negPHI
    Abc_Ntk_t *negPHI_Ntk = Abc_NtkFromAigPhase(negPhi_Man);

    map<int, pair<int, int>> exToOutMapping;
    for (auto id : exs)
    {
        Abc_Ntk_t *ANtk = Abc_NtkFromAigPhase(A_Man[id]);
        Abc_Ntk_t *BNtk = Abc_NtkFromAigPhase(B_Man[id]);

        Abc_NtkAppend(negPHI_Ntk, ANtk, 1);
        Abc_NtkAppend(negPHI_Ntk, BNtk, 1);

        cout << "Appended Basis for EX_VAR: " << id << endl;

        exToOutMapping[id] = {Abc_NtkCoNum(negPHI_Ntk) - 2, Abc_NtkCoNum(negPHI_Ntk) - 1};
    }

    Aig_Man_t *tMan = Abc_NtkToDar(negPHI_Ntk, 0, 0);

    // compose the outputs and create the final output for delta ^ !phi
    map<int, Aig_Obj_t *> exToHMapping;
    Aig_Obj_t *delta = Aig_ManConst1(tMan);
    for (auto id : exs)
    {
        Aig_Obj_t *outA = Aig_ManCo(tMan, exToOutMapping[id].first)->pFanin0;
        Aig_Obj_t *outB = Aig_ManCo(tMan, exToOutMapping[id].second)->pFanin0;

        Aig_Obj_t *currH = Aig_ObjCreateCi(tMan);
        // Aig_ObjCreat

        exToHMapping[id] = currH;
        Aig_Obj_t *HAndB = Aig_And(tMan, currH, outB);
        Aig_Obj_t *defin = Aig_Or(tMan, outA, HAndB);
        Aig_Obj_t *yImpDefin = Aig_Or(tMan, Aig_Not(Aig_ManCi(tMan, id - 1)), defin);
        Aig_Obj_t *definImpY = Aig_Or(tMan, Aig_Not(defin), Aig_ManCi(tMan, id - 1));

        Aig_Obj_t *currDelta = Aig_And(tMan, yImpDefin, definImpY);

        delta = Aig_And(tMan, delta, currDelta);
    }
    Aig_Obj_t *outputDriver = Aig_And(tMan, delta, Aig_ManCo(tMan, 0)->pFanin0);
    Aig_Obj_t *newOut = Aig_ObjCreateCo(tMan, outputDriver);

    int numOuts = Aig_ManCoNum(tMan);
    for (int i = 0; i < numOuts - 1; i++)
    {
        Aig_ObjDisconnect(tMan, Aig_ManCo(tMan, i));
        Aig_ObjConnect(tMan, Aig_ManCo(tMan, i), Aig_ManConst0(tMan), NULL);
    }
    // Abc_Ntk_t* tntk = Abc_NtkFromAigPhase(origFormula);
    // exit(1);
    // Aig_Man_t* origFormula = Aig_ManDupOneOutput(tMan,Aig_ManCoNum(tMan)-1,0);
    Aig_ManCoCleanup(tMan);
    Aig_ManCleanup(tMan);

    if (Aig_ManCoNum(tMan) == 0)
    {
        Aig_ObjCreateCo(tMan, Aig_ManConst0(tMan));
    }

    // tMan = compressAig(tMan);
    // tMan = compressAig(tMan);
    Aig_ManCleanup(tMan);

    Aig_Man_t *origFormula = tMan;
    // origFormula = compressAig(origFormula);
    // origFormula = compressAig(origFormula);
    Aig_ManCleanup(origFormula);
    // Aig_ManReduceLaches(origFormula,1);
    Aig_Obj_t *deltaAndNegPhi = Aig_ManCo(origFormula, 0)->pFanin0;
    
    // Aig_ManShow(ori)

    map<int, Aig_Obj_t *> exToCases; // cases when h=0;

    for (auto id : exs)
    {
        exToCases[id] = Aig_ManConst0(origFormula);
    }

    // build mu and default case;
    Aig_Obj_t *mu = Aig_ManConst1(origFormula);
    Aig_Obj_t *defaultCase = Aig_ManConst1(origFormula);

    for (auto id : exs)
    {
        Aig_Obj_t *currCase = exToCases[id];

        // case imp ~h = ~case v ~h
        Aig_Obj_t *tmp = Aig_Or(origFormula, Aig_Not(currCase), Aig_Not(exToHMapping[id]));
        mu = Aig_And(origFormula, mu, tmp);

        // ~case => h<=>1 = case v h
        tmp = Aig_Or(origFormula, currCase, exToHMapping[id]);
        defaultCase = Aig_And(origFormula, defaultCase, tmp);
    }

    Aig_Obj_t *constraint = Aig_ManConst1(origFormula);

    // compose the nodes to create final output
    newOut = Aig_ManConst1(origFormula);
    newOut = Aig_And(origFormula, newOut, deltaAndNegPhi); // this stays constant
    newOut = Aig_And(origFormula, newOut, mu);             // this changes by mu -> mu AND tmpMu
    newOut = Aig_And(origFormula, newOut, defaultCase);    // we build this from exToCases
    newOut = Aig_And(origFormula, newOut, constraint);     // this updates by constraint -> constraint AND newConstraint.

    Aig_ObjCreateCo(origFormula, newOut);

    Aig_ObjDisconnect(origFormula, Aig_ManCo(origFormula, 0));
    Aig_ObjConnect(origFormula, Aig_ManCo(origFormula, 0), Aig_ManConst0(origFormula), NULL);

    Aig_ManCoCleanup(origFormula);
    Aig_ManCleanup(origFormula);

    if (Aig_ManCoNum(origFormula) == 0)
    {
        Aig_ObjCreateCo(origFormula, Aig_ManConst0(origFormula));
    }

    // A manager for constraints to check for sat
    Aig_Man_t *constraintMan = Aig_ManDupOrdered(origFormula);
    Aig_ObjDisconnect(constraintMan, Aig_ManCo(constraintMan, 0));
    Aig_ObjConnect(constraintMan, Aig_ManCo(constraintMan, 0), Aig_ManConst1(constraintMan), NULL);
    Aig_ManCoCleanup(constraintMan);
    Aig_ManCleanup(constraintMan);

    if (Aig_ManCoNum(constraintMan) == 0)
    {
        Aig_ObjCreateCo(constraintMan, Aig_ManConst1(constraintMan));
    }

    // Aig_ManShow(constraintMan,0,NULL);
    // int aa;
    // cin>>aa;

    // start the main loop
    // origFormu  , constrMan //
    map<pair<int, set<int>>, pair<Aig_Obj_t *, Aig_Obj_t *>> ex_caseToAuxMapping; // {ex_id,[depVal] }-> <Aig_Obj_t*, Aig_Obj_t*>

    map<set<int>, Aig_Obj_t *> caseToNodeMapping;
    map<Aig_Obj_t*, int> auxToIdMapping;
    int iter = 0;
    // set<set<int>> constraintSet;
    cout<<"****************      Starting Main Loop     *******************"<<endl;

    
    TIME_MEASURE_START
    while (true)
    {
        int totalInputs = Aig_ManCiNum(origFormula);
        iter++;
        bool verbose=false;



        if(iter%500==0){
            verbose=true;
        }
        if(verbose) cout << "************     ITER: " << iter << "    **************" << endl;

        if(verbose){
            cout<<"************          INFOO TIMEEEEEE      ****************\n";
            cout<<"NUM INPUTS: "<<Aig_ManCiNum(origFormula)<<endl;;
            cout<<"NETWORK SIZE: "<<Aig_ManNodeNum(origFormula)<<endl;
            cout<<"Num Aux Created: "<<auxToIdMapping.size()<<endl;
        }
        // cout<<"SHOWING ORIGFORMULA: ";
        // int aa;
        // cin>>aa;
        // Aig_ManShow(origFormula,0,NULL);
        
        // cin>>aa;
        Abc_Ntk_t *FNtk = Abc_NtkFromAigPhase(origFormula);
        int status = Abc_NtkMiterSat(FNtk, 100000, 0, 0, NULL, NULL);
        if (status == -1)
        {
            cerr << "Timeout Occured.\n";
            cout<<"Ending Main Loop. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
            cout<<"TIMEOUT OCCURED."<<endl;
            Abc_Stop();
            exit(1);
        }
        // Aig_ManShow(origFormula,0,NULL);
        
        // int res = Abc_NtkVerifySimulatePattern(FNtk,ex)[0];
        // cout<<res<<endl;
        // exit(1);
        if (status == 1)
        {
            Abc_Ntk_t *constraintNtk = Abc_NtkFromAigPhase(constraintMan);

            int constrStatus = Abc_NtkMiterSat(constraintNtk, 100000, 0, 0, NULL, NULL);
            if (constrStatus !=0)
            {   
                // Aig_ManShow(constraintMan,0,NULL);
                cout << "Failed to satisfy auxilary variable constraints. Terminating...\n";
                cout<<"Ending Main Loop. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
                cout<<"UNSATISFIABLE"<<endl;
                Abc_Stop();
                return 0;
            }
            cout << "Formula has become unsat :)\n";
            cout<<"Ending Main Loop. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
            cout<<"SATISFIABLE"<<endl;
            Abc_Stop();
            return 0;
        }

        if(verbose)    cout << "Formula is SAT. Working on the counter-example...\n";

        int *cex = FNtk->pModel;

        // cout << "CEX : ";

        // for (int i = 0; i < totalInputs; i++)
        // {
        //     cout << cex[i] << " ";
        // }
        // cout << endl;

        Aig_Obj_t *newConstr1 = Aig_ManConst0(origFormula);
        Aig_Obj_t *newConstr2 = Aig_ManConst0(constraintMan);

        bool changeFlag = false;
        Aig_Obj_t *tmpMu = Aig_ManConst1(origFormula);

        int constraintManInitialSize = constraintMan->vObjs->nSize;
        // set<int> constraintTempSet;
        for (auto id : exs)
        {

            int a_i = Abc_NtkVerifySimulatePattern(A_Ntk[id], cex)[0];
            int b_i = Abc_NtkVerifySimulatePattern(B_Ntk[id], cex)[0];

            // if(verbose) printf("A[%d]: %d | B[%d]: %d\n", id, a_i, id, b_i);
            if (!(a_i == 0 && b_i == 1))
            {
                continue;
            }

            // get the set cex|dep
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

            // Aig_Obj_t* newCase = Aig_ManConst1(origFormula);
            // if(id==13){
                // cout<<"DEBUG: ";
                // for(auto dep:depVal){
                //     cout<<dep<<" ";
                // }
                // cout<<endl;
            // }
            if (ex_caseToAuxMapping.find({id, depVal}) == ex_caseToAuxMapping.end())
            {
                cout << "Created new auxiliary variable for id: " << id << endl;
                changeFlag = true;
                // generate new aux var and update mu
                Aig_Obj_t *newAux = Aig_ObjCreateCi(origFormula);
                Aig_Obj_t *newAuxConstraint = Aig_ObjCreateCi(constraintMan);
                // cout<<"CIO id: "<<Aig_ObjCioId(newAux)<<endl;
                // constraintTempSet.insert(-Aig_ManCiNum(origFormula));
                auxToIdMapping[newAux] = Aig_ManCiNum(origFormula);

                // mu =  mu AND case-> h<->newAux
                Aig_Obj_t *newCase = Aig_ManConst1(origFormula);
                if (caseToNodeMapping.find(depVal) == caseToNodeMapping.end())
                {
                    // create the node for case
                    // cout<<"HERE\n";
                    for (auto dep : depVal)
                    {
                        if (dep > 0)
                        {
                            newCase = Aig_And(origFormula, newCase, Aig_ManCi(origFormula, dep - 1));
                        }
                        else
                        {
                            newCase = Aig_And(origFormula, newCase, Aig_Not(Aig_ManCi(origFormula, -dep - 1)));
                        }
                    }
                    caseToNodeMapping[depVal] = newCase;
                }
                else
                {
                    newCase = caseToNodeMapping[depVal];
                }
        
                Aig_Obj_t *hImpAux = Aig_Or(origFormula, Aig_Not(exToHMapping[id]), newAux);
                Aig_Obj_t *auxImpH = Aig_Or(origFormula, Aig_Not(newAux), exToHMapping[id]);
                Aig_Obj_t *hIFFaux = Aig_And(origFormula, hImpAux, auxImpH);

                Aig_Obj_t *caseImpAsg = Aig_Or(origFormula, Aig_Not(newCase), hIFFaux);
                // update tmpMu -> tmpMu AND caseImpAsg
                tmpMu = Aig_And(origFormula, tmpMu, caseImpAsg);
                // cout<<"Added implication: ";
                // cin>>aa;
                // Aig_ManShow(origFormula,0,NULL);
                // cin>>aa;
                exToCases[id] = Aig_Or(origFormula, exToCases[id], newCase); // for defaultCase node.

                // uodate the newConstr nodes.
                newConstr1 = Aig_Or(origFormula, newConstr1, Aig_Not(newAux));
                newConstr2 = Aig_Or(constraintMan, newConstr2, Aig_Not(newAuxConstraint));

                ex_caseToAuxMapping[{id, depVal}] = {newAux, newAuxConstraint};
            }
            else
            {
                // the aux variable already exists, which implies that we only need to update the constraints.
                auto p = ex_caseToAuxMapping[{id, depVal}];
                int inputId = auxToIdMapping[p.first];
                // cout<<inputId<<endl;
                // cout<<"CEX[inputId]: "<<cex[inputId]<<endl;
                if(cex[inputId-1]==1){
                    // constraintTempSet.insert(-inputId);
                    newConstr1 = Aig_Or(origFormula, newConstr1, Aig_Not(p.first));
                    newConstr2 = Aig_Or(constraintMan, newConstr2, Aig_Not(p.second));
                }
                else{
                    // constraintTempSet.insert(inputId);
                    newConstr1 = Aig_Or(origFormula, newConstr1, p.first);
                    newConstr2 = Aig_Or(constraintMan, newConstr2, p.second);
                }
            }
        }
        // int initVal = constraintSet.size();
        // constraintSet.insert(constraintTempSet);
        // int newVal = constraintSet.size();
        // cout<<"PRINTING CONSTRAINT SET SETS: "<<endl;
        int sid=0;
        int cont;
        // cin>>cont;
        
        // for(auto s:constraintSet){
        //     cout<<sid<<": [";
        //     for(auto e:s){
        //         cout<<e<<" ";
        //     }
        //     cout<<"]"<<endl;
        //     sid++;
        // }
        // cout<<"DONE PRINTING\n";
        // if(verbose) printf("SET VAL: init: %d | new: %d\n",initVal,newVal);
        // if(newVal!=initVal){
        //     changeFlag=true;
        // }

        // update origFormula
        Aig_Obj_t *newOut_tmp = Aig_ManConst1(origFormula);

        // update mu
        // cout<<"BEFORE APPENDING tmpMu: ";
        // cin>>aa;
        // Aig_ManShow(origFormula,0,NULL);
        // cin>>aa;

        if(tmpMu != Aig_ManConst1(origFormula)){
            // cout<<"HERE2\n";
            mu = Aig_And(origFormula, mu, tmpMu);
            // cout<<Aig_ObjId(mu)<<endl;
            // cout<<"AFTER APPENDING: ";
            // cin>>aa;
            // Aig_ManShow(origFormula,0,NULL);
            // cin>>aa;
        }

        // update constraint
        if(newConstr1 != Aig_ManConst0(origFormula)){
            // newConstr1 = Aig_ManConst1(origFormula);
            constraint = Aig_And(origFormula, constraint, newConstr1);
        }

        // build defaultCase
        defaultCase = Aig_ManConst1(origFormula);
        for (auto id : exs)
        {

            Aig_Obj_t *currCase = exToCases[id];

            // ~case => h<=>1 = case v h
            // if(iter==2) {Aig_ManShow(origFormula,0,NULL);
            // int aa;
            // cin>>aa;}

            Aig_Obj_t *tmp = Aig_Or(origFormula, currCase, exToHMapping[id]);
            defaultCase = Aig_And(origFormula, defaultCase, tmp);
            // cout<<"BUILT DEFAULT CASE FOR id: "<<id<<endl;
            // cout<<"PRINTING: ";
            // cin>>aa;
            // Aig_ManShow(origFormula,0,NULL);
            // cin>>aa;
        }

        newOut_tmp = Aig_And(origFormula, newOut_tmp, deltaAndNegPhi);
        newOut_tmp = Aig_And(origFormula, newOut_tmp, mu);
        newOut_tmp = Aig_And(origFormula, newOut_tmp, defaultCase);
        newOut_tmp = Aig_And(origFormula, newOut_tmp, constraint);

        Aig_ObjCreateCo(origFormula, newOut_tmp);

        Aig_ObjDisconnect(origFormula, Aig_ManCo(origFormula, 0));
        Aig_ObjConnect(origFormula, Aig_ManCo(origFormula, 0), Aig_ManConst0(origFormula), NULL);

        Aig_ManCoCleanup(origFormula);
        Aig_ManCleanup(origFormula);

        if (Aig_ManCoNum(origFormula) == 0)
        {
            Aig_ObjCreateCo(origFormula, Aig_ManConst0(origFormula));
        }

        // update constraintMan
        //  output = output AND newConstr2
        
        Aig_Obj_t *constrNewOut = Aig_ManCo(constraintMan, 0)->pFanin0;
        Aig_Obj_t* constrOldOut = constrNewOut;
        if(newConstr2 !=Aig_ManConst0(constraintMan)){
            constrNewOut = Aig_And(constraintMan, constrNewOut, newConstr2);
        }
        
        if(constrOldOut !=constrNewOut){
            changeFlag=true;
        }

        Aig_ObjCreateCo(constraintMan, constrNewOut);

        Aig_ObjDisconnect(constraintMan, Aig_ManCo(constraintMan, 0));
        Aig_ObjConnect(constraintMan, Aig_ManCo(constraintMan, 0), Aig_ManConst0(constraintMan), NULL);

        Aig_ManCoCleanup(constraintMan);
        Aig_ManCleanup(constraintMan);
        
        if (Aig_ManCoNum(constraintMan) == 0)
        {
            Aig_ObjCreateCo(constraintMan, Aig_ManConst0(constraintMan));
        }
        // cout<<"ENTER 1 to show constraint network: ";
        // int inp;
        // cin>>inp;
        // if(inp == 1){Aig_ManShow(constraintMan,0,NULL);
        // cin>>aa;}

        // changeFlag=true;
        if (changeFlag == false)
        {
            cerr << "No new variables created... exiting :(\n";
            cout<<"Ending Main Loop. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
            cout<<"UNSATISFIABLE"<<endl;
            Abc_Stop();
            exit(1);
        }
        // if(1){
        //     cout<<"INITIAL SIZE: "<<Aig_ManNodeNum(origFormula)<<endl;
        // }
        // origFormula = Dar_ManCompress2(origFormula,1,1,50,1,1);
        // if(1){
        //     cout<<"New SIZE: "<<Aig_ManNodeNum(origFormula)<<endl;
        // }
    }
    Abc_Stop();
    cout<<"Ending Main Loop. Time Elapsed: "<<TIME_MEASURE_ELAPSED<<endl;
    cout<<"UNSATISFIABLE."<<endl;
    return 0;

    // while (true)
    // {
    //     iter++;
    //     Abc_Ntk_t *FNtk = Abc_NtkFromAigPhase(Formula);
    //     // Aig_ManShow(Formula,0,NULL);
    //     // cout<<"Enter 1 ";
    //     // cin>>mySIG;
    //     int status = Abc_NtkMiterSat(FNtk, 100000, 0, 1, NULL, NULL);
    //     if (status == -1)
    //     {
    //         cerr << "Timeout Occured\n";
    //         exit(1);
    //     }

    //     if (status == 1)
    //     {
    //         cout << "UNSat!!\n";

    //         return 0;
    //     }

    //     cout << "Formula is SAT. Working on the counter-example...\n";

    //     int *cex = FNtk->pModel;
    //     cout << "ITER: " << iter << " | CEX : ";
    //     for (int i = 0; i < totalInputs; i++)
    //     {
    //         cout << cex[i] << " ";
    //     }
    //     cout << endl;
    //     bool changeFlag = false;
    //     for (int i = 0; i < numY; i++)
    //     {

    //         int a_i = Abc_NtkVerifySimulatePattern(A_Ntk[i], cex)[0];
    //         int b_i = Abc_NtkVerifySimulatePattern(B_Ntk[i], cex)[0];

    //         printf("A[%d]: %d | B[%d]: %d\n", i, a_i, i, b_i);
    //         if (!(a_i == 0 && b_i == 1))
    //         {
    //             continue;
    //         }

    //         // get the dependency clause to insert in defccase
    //         vector<int> newClause;
    //         for (auto dep : dependencies[numX + i + 1])
    //         {
    //             if (cex[dep - 1] == 0)
    //             {
    //                 newClause.push_back(-dep);
    //             }
    //             else
    //             {
    //                 newClause.push_back(dep);
    //             }
    //         }
    //         sort(newClause.begin(), newClause.end());
    //         int oldSize = defaultCase[i].size();
    //         defaultCase[i].insert(newClause);
    //         if (defaultCase[i].size() != oldSize)
    //         {
    //             changeFlag = true;
    //         }
    //     }

    //     if (!changeFlag)
    //     {
    //         cout << "No change occured.. :( Ending loop.\n";
    //         cout << "Skolem Function do not exist.\n";
    //         break;
    //     }

    //     Aig_ManStop(Formula);

    //     Formula = Aig_ManDupOrdered(origFormula);

    //     Aig_Obj_t *dcCum = Aig_ManConst1(Formula);
    //     Aig_Obj_t *muCum = Aig_ManConst1(Formula);

    //     for (int i = 0; i < numY; i++)
    //     {
    //         set<vector<int>> list = defaultCase[i];
    //         // cout<<"Creating default case and mu for y_"<<i<<endl;

    //         Aig_Obj_t *dc_i = Aig_ManConst1(Formula);
    //         Aig_Obj_t *mu_i = Aig_ManConst0(Formula);
    //         if (list.empty())
    //         {
    //             // dc_i=Aig_ManConst1(Formula);
    //             // cout<<"empty\n";
    //         }
    //         else
    //         {
    //             for (auto c : list)
    //             {
    //                 Aig_Obj_t *tmp = Aig_ManConst1(Formula);
    //                 for (auto lit : c)
    //                 {
    //                     if (lit > 0)
    //                     {
    //                         tmp = Aig_And(Formula, tmp, Aig_ManCi(Formula, lit - 1));
    //                     }
    //                     else
    //                     {
    //                         tmp = Aig_And(Formula, tmp, Aig_Not(Aig_ManCi(Formula, -lit - 1)));
    //                     }
    //                 }
    //                 dc_i = Aig_And(Formula, dc_i, Aig_Not(tmp));
    //                 mu_i = Aig_Or(Formula, mu_i, tmp);
    //             }
    //         }

    //         Aig_Obj_t *dc_i_imp_H = Aig_Or(Formula, Aig_Not(dc_i), Aig_ManCi(Formula, numOrigInputs + i));
    //         dcCum = Aig_And(Formula, dcCum, dc_i_imp_H);

    //         Aig_Obj_t *mu_i_imp_notH = Aig_Or(Formula, Aig_Not(mu_i), Aig_Not(Aig_ManCi(Formula, numOrigInputs + i)));
    //         muCum = Aig_And(Formula, muCum, mu_i_imp_notH);
    //     }

    //     Aig_Obj_t *outAndDefault = Aig_And(Formula, Aig_ManCo(Formula, 0)->pFanin0, dcCum);
    //     Aig_Obj_t *finalOut = Aig_And(Formula, outAndDefault, muCum);

    //     Aig_ObjCreateCo(Formula, finalOut);

    //     // Aig_ManShow(Formula,0,NULL);
    //     // cin>>mySIG;
    //     // removing previous out
    //     Aig_ObjDisconnect(Formula, Aig_ManCo(Formula, 0));
    //     Aig_ObjConnect(Formula, Aig_ManCo(Formula, 0), Aig_ManConst0(Formula), NULL);

    //     Aig_ManCoCleanup(Formula);

    //     // Aig_ManShow(Formula,0,NULL);
    //     // cin>>mySIG;
    // }

    Abc_Stop();

    return 0;
}