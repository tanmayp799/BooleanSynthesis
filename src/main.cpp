
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

    // Creating phi_Ntk and phi_Man.
    Abc_Ntk_t *phi_Ntk;
    Aig_Man_t *phi_Man;

    string inpPath;
    // cout << "Enter path for PHI.dimacs: ";
    // getline(cin, inpPath);
    if(argc!=2){
        cerr<<"Usage ./bin/main <path to PHI.dimacs file>\n";
        return 1;
    }
    inpPath = argv[1];

    FS::path phiPath(inpPath);

    string phi_0Dir;
    string phi_1Dir;

    // string jsonPath;
    string phiFileName = phiPath.stem().string();

    if (FS::exists(phiPath))
    {
        char *phiPath_cstr = new char[phiPath.string().size() + 1];

        strcpy(phiPath_cstr, phiPath.c_str());

        phi_Ntk = getNtkFromCNF(phiPath_cstr);

        assert(phi_Ntk != NULL);

        FS::path phi_0DirPath = FS::path("./data/") / phiPath.stem().string() / "phi_0";
        assert(FS::exists(phi_0DirPath));
        phi_0Dir = phi_0DirPath.string();

        FS::path phi_1DirPath = FS::path("./data/") / phiPath.stem().string() / "phi_1";
        assert(FS::exists(phi_1DirPath));
        phi_1Dir = phi_1DirPath.string();
    }
    else
    {
        cerr << "File " << inpPath << " does not exist!" << endl;
        exit(1);
    }
    // return 0;
    phi_Man = Abc_NtkToDar(phi_Ntk, 0, 0);

    // Build Neg_Phi
    Aig_Man_t *negPhi_Man = Aig_ManDupOrdered(phi_Man);
    Aig_ManCo(negPhi_Man, 0)->pFanin0 = Aig_Not(Aig_ManCo(negPhi_Man, 0)->pFanin0);
    Abc_Ntk_t *negPhi_Ntk = Abc_NtkFromAigPhase(negPhi_Man);

    FS::path jsonPath = FS::path("./data/") / phiFileName / (phiFileName+".json");
    
    ifstream json_file(jsonPath.string());
    if(!json_file.is_open()){
        cerr<<"Error opening file: "<<jsonPath.string()<<endl;
        return 1;
    }

    nlohmann::json json_data;
    json_file>>json_data;

    numOrigInputs = Aig_ManCiNum(negPhi_Man);
    numX = json_data["numX"], numY = json_data["numY"];
    

    // dependencies = json_data["dependencies"].get<std::map<int, std::vector<int>>>();
    for (auto& [key, value] : json_data["dependencies"].items()) {
        int int_key = std::stoi(key);  // JSON keys are strings; convert to int
        dependencies[int_key] = value.get<std::vector<int>>();
    }
    // return 0;

    assert(numX+numY==numOrigInputs);
    

    std::cout << "numX: " << numX << std::endl;
    std::cout << "numY: " << numY << std::endl;
    std::cout << "Dependencies:" << std::endl;
    for (const auto& [key, value] : dependencies) {
        std::cout << "  " << key << ": [";
        for (size_t i = 0; i < value.size(); ++i) {
            std::cout << value[i] << (i < value.size() - 1 ? ", " : "");
        }
        std::cout << "]" << std::endl;
    }
    
    // return 0;

    cout << "NegPHI created\n";
    // Aig_ManShow(negPhi_Man,0,NULL);
    // cin>>mySIG;

    // Generating A_Ntk, B_Ntk, A_Man, B_Man, Formula
    vector<Abc_Ntk_t *> A_Ntk;
    vector<Abc_Ntk_t *> B_Ntk;

    vector<Aig_Man_t *> A_Man;
    vector<Aig_Man_t *> B_Man;

    generateBasis(phi_0Dir, phi_1Dir, A_Ntk, B_Ntk, A_Man, B_Man);
    cout << "Basis Created\n";
    // vector<Aig_Obj_t*> H;
    assert(A_Ntk.size() == numY);
    assert(B_Ntk.size() == numY);
    assert(A_Man.size() == numY);
    assert(B_Man.size() == numY);

    cout << A_Ntk.size() << endl;
    for (int i = 0; i < numY; i++)
    {
        Abc_Ntk_t *currA = A_Ntk[i];
        cout << "here\n";
        Abc_Ntk_t *currB = B_Ntk[i];
        if (!Abc_NtkAppend(negPhi_Ntk, currA, 1))
        {
            cerr << "Failed appending A[" << i << "] into Neg_PHI network";
            exit(1);
        }

        if (!Abc_NtkAppend(negPhi_Ntk, currB, 1))
        {
            cerr << "Failed appending B[" << i << "] into Neg_PHI network";
            exit(1);
        }
        // cout<<"Appended basis for "<<i<<" var\n";
    }

    Aig_Man_t *tmpMan = Abc_NtkToDar(negPhi_Ntk, 0, 0);
    Aig_Obj_t *delta = nullptr;

    // Aig_ManShow(tmpMan,0,NULL);
    // cin>>mySIG;

    for (int i = 0; i < numY; i++)
    {
        Aig_Obj_t *outA = Aig_ManCo(tmpMan, 2 * i + 1)->pFanin0;
        Aig_Obj_t *outB = Aig_ManCo(tmpMan, 2 * i + 2)->pFanin0;

        Aig_Obj_t *currH = Aig_ObjCreateCi(tmpMan);
        // H.push_back(currH);

        Aig_Obj_t *H_And_B = Aig_And(tmpMan, currH, outB);

        Aig_Obj_t *defin = Aig_Or(tmpMan, outA, H_And_B);
        Aig_Obj_t *y_imp_defin = Aig_Or(tmpMan, Aig_Not(Aig_ManCi(tmpMan, numX + i)), defin);
        Aig_Obj_t *defin_imp_y = Aig_Or(tmpMan, Aig_Not(defin), Aig_ManCi(tmpMan, numX + i));

        Aig_Obj_t *currDelta = Aig_And(tmpMan, y_imp_defin, defin_imp_y);

        if (delta == nullptr)
        {
            delta = currDelta;
        }
        else
        {
            delta = Aig_And(tmpMan, delta, currDelta);
        }
    }
    Aig_Obj_t *newOut = Aig_And(tmpMan, Aig_ManCo(tmpMan, 0)->pFanin0, delta);
    Aig_Obj_t *reqdOutNode = Aig_ObjCreateCo(tmpMan, newOut);
    Aig_Man_t *origFormula = Aig_ManDupOneOutput(tmpMan, Aig_ManCoNum(tmpMan) - 1, 0);

    // Aig_Man_t* Formula = Aig_ManDupOrdered(origFormula);

    Aig_ManStop(tmpMan);

    

    // Aig_ManShow(origFormula,0,NULL);
    // cin>>mySIG;

    vector<set<vector<int>>> defaultCase(numY); //dc[i]-> a list of lists.-> 1 entry is a list of lits which form a formula like !(and of list1) AND !(and of list 2) AND ...
    // vector<vector<vector<int>>> mu(numY); //mu[i]->list of list. It forms the formula (and of list1) OR (and of list 2) OR ....
            //The lists(for dc and mu) [i]  are actually the same!!! 

    Aig_Man_t* Formula = Aig_ManDupOrdered(origFormula);

    //Creating mu[i] and dc[i] followed by cumulatives to create final output node.
    Aig_Obj_t* dcCum=Aig_ManConst1(Formula);
    Aig_Obj_t* muCum = Aig_ManConst1(Formula);
    // auto garbge = defaultCase[numX];
    for(int i=0;i<numY;i++){
        set<vector<int>> list = defaultCase[i];
        cout<<"Creating default case and mu for y_"<<i<<endl;

        Aig_Obj_t* dc_i=Aig_ManConst1(Formula);
        Aig_Obj_t* mu_i = Aig_ManConst0(Formula);
        if(list.empty()){
            // dc_i=Aig_ManConst1(Formula);
            cout<<"empty\n";
        }
        else{
            for(auto c: list){
                Aig_Obj_t* tmp = Aig_ManConst1(Formula);
                for(auto lit:c){
                    if(lit>0){
                        tmp = Aig_And(Formula,tmp,Aig_ManCi(Formula,lit-1));
                    }
                    else{
                        tmp = Aig_And(Formula,tmp,Aig_Not(Aig_ManCi(Formula,-lit-1)));
                    }
                }
                dc_i = Aig_And(Formula,dc_i,Aig_Not(tmp));
                mu_i = Aig_Or(Formula, mu_i, tmp);
            }
        }

        Aig_Obj_t* dc_i_imp_H = Aig_Or(Formula, Aig_Not(dc_i), Aig_ManCi(Formula,numOrigInputs+i));
        dcCum = Aig_And(Formula,dcCum,dc_i_imp_H);

        Aig_Obj_t* mu_i_imp_notH = Aig_Or(Formula,Aig_Not(mu_i),Aig_Not(Aig_ManCi(Formula,numOrigInputs+i)));
        muCum = Aig_And(Formula,muCum,mu_i_imp_notH);
    }

    Aig_Obj_t* outAndDefault = Aig_And(Formula,Aig_ManCo(Formula,0)->pFanin0, dcCum);
    Aig_Obj_t* finalOut = Aig_And(Formula, outAndDefault, muCum);

    Aig_ObjCreateCo(Formula,finalOut);

    // Aig_ManShow(Formula,0,NULL);
    // cin>>mySIG;
    //removing previous out
    Aig_ObjDisconnect(Formula,Aig_ManCo(Formula,0));
    Aig_ObjConnect(Formula, Aig_ManCo(Formula,0),Aig_ManConst0(Formula),NULL);

    Aig_ManCoCleanup(Formula);

    // Aig_ManShow(Formula,0,NULL);
    // cin>>mySIG;



    int totalInputs = Aig_ManCiNum(Formula);
    int iter=0;
    while (true)
    {
        iter++;
        Abc_Ntk_t *FNtk = Abc_NtkFromAigPhase(Formula);

        int status = Abc_NtkMiterSat(FNtk, 100000, 0, 1, NULL, NULL);
        if (status == -1)
        {
            cerr << "Timeout Occured\n";
            exit(1);
        }

        if (status == 1)
        {
            cout << "UNSat!!\n";
            return 0;
        }

        cout << "Formula is SAT. Working on the counter-example...\n";

        int *cex = FNtk->pModel;
        cout<<"ITER: "<<iter<<" | CEX : ";
        for(int i=0;i<totalInputs;i++){
            cout<<cex[i]<<" ";
        }
        cout<<endl;
        bool changeFlag=false;
        for(int i=0;i<numY;i++){

            int a_i = Abc_NtkVerifySimulatePattern(A_Ntk[i],cex)[0];
            int b_i = Abc_NtkVerifySimulatePattern(B_Ntk[i],cex)[0];

            printf("A[%d]: %d | B[%d]: %d\n",i,a_i,i,b_i);
            if(!(a_i==0 && b_i==1)){
                continue;
            }

            //get the dependency clause to insert in defccase
            vector<int> newClause;
            for(auto dep:dependencies[numX+i+1]){
                if(cex[dep-1]==0){
                    newClause.push_back(-dep);
                }
                else{
                    newClause.push_back(dep);
                }
            }
            sort(newClause.begin(),newClause.end());
            int oldSize = defaultCase[i].size();
            defaultCase[i].insert(newClause);
            if(defaultCase[i].size()!=oldSize){
                changeFlag=true;
            }

        }

        if(!changeFlag){
            cout<<"No change occured.. :( Ending loop.\n";
            cout<<"Skolem Function do not exist.\n";
            break;
        }

        Aig_ManStop(Formula);

        Formula = Aig_ManDupOrdered(origFormula);

        Aig_Obj_t* dcCum=Aig_ManConst1(Formula);
        Aig_Obj_t* muCum = Aig_ManConst1(Formula);

        for(int i=0;i<numY;i++){
            set<vector<int>> list = defaultCase[i];
            cout<<"Creating default case and mu for y_"<<i<<endl;

            Aig_Obj_t* dc_i=Aig_ManConst1(Formula);
            Aig_Obj_t* mu_i = Aig_ManConst0(Formula);
            if(list.empty()){
                // dc_i=Aig_ManConst1(Formula);
                cout<<"empty\n";
            }
            else{
                for(auto c: list){
                    Aig_Obj_t* tmp = Aig_ManConst1(Formula);
                    for(auto lit:c){
                        if(lit>0){
                            tmp = Aig_And(Formula,tmp,Aig_ManCi(Formula,lit-1));
                        }
                        else{
                            tmp = Aig_And(Formula,tmp,Aig_Not(Aig_ManCi(Formula,-lit-1)));
                        }
                    }
                    dc_i = Aig_And(Formula,dc_i,Aig_Not(tmp));
                    mu_i = Aig_Or(Formula, mu_i, tmp);
                }
            }

            Aig_Obj_t* dc_i_imp_H = Aig_Or(Formula, Aig_Not(dc_i), Aig_ManCi(Formula,numOrigInputs+i));
            dcCum = Aig_And(Formula,dcCum,dc_i_imp_H);

            Aig_Obj_t* mu_i_imp_notH = Aig_Or(Formula,Aig_Not(mu_i),Aig_Not(Aig_ManCi(Formula,numOrigInputs+i)));
            muCum = Aig_And(Formula,muCum,mu_i_imp_notH);
        }

        Aig_Obj_t* outAndDefault = Aig_And(Formula,Aig_ManCo(Formula,0)->pFanin0, dcCum);
        Aig_Obj_t* finalOut = Aig_And(Formula, outAndDefault, muCum);

        Aig_ObjCreateCo(Formula,finalOut);

        // Aig_ManShow(Formula,0,NULL);
        // cin>>mySIG;
        //removing previous out
        Aig_ObjDisconnect(Formula,Aig_ManCo(Formula,0));
        Aig_ObjConnect(Formula, Aig_ManCo(Formula,0),Aig_ManConst0(Formula),NULL);

        Aig_ManCoCleanup(Formula);

        // Aig_ManShow(Formula,0,NULL);
        // cin>>mySIG;
    }

    Abc_Stop();

    return 0;
}