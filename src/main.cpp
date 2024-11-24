
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
#include "helper.h"

using namespace std;

////////////////////////////////////////////////////////////////////////
///                           GLOBALS                                ///
////////////////////////////////////////////////////////////////////////

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

    Abc_Start();

    pAbc = Abc_FrameGetGlobalFrame();

    // string filename;

    // char cwd[100000];
    // getcwd(cwd,sizeof(cwd));
    // cout<<cwd<<endl;
    // string cwd_string = std::string(cwd);

    // cin>>filename;

    // filename=cwd_string+"/"+filename;
    // char* filename_cstr=new char[filename.size()+1];
    // cout<<"here\n";
    // strcpy(filename_cstr,filename.c_str());
    // Abc_Ntk_t * NtkPHI = getNtkFromCNF(filename_cstr);
    // Aig_Man_t * ManPHI = Abc_NtkToDar(NtkPHI, 0, 0);

    // Generating A_Ntk, B_Ntk, A_Man, B_Man, Formula

    // FS::path dir1 = "data/phi_0/";
    // FS::path dir2 = "data/phi_1/";

    vector<Abc_Ntk_t *> A_Ntk;
    vector<Abc_Ntk_t *> B_Ntk;

    vector<Aig_Man_t *> A_Man;
    vector<Aig_Man_t *> B_Man;

    generateBasis("data/phi_0/","data/phi_1/",A_Ntk,B_Ntk,A_Man,B_Man);

    // vector<string> files1, files2;

    // for (const auto &entry : FS::directory_iterator(dir1))
    // {
    //     if (FS::is_regular_file(entry.path()))
    //     {
    //         string path_str = entry.path().string();
    //         files1.push_back(path_str);
    //     }
    // }

    // for (const auto &entry : FS::directory_iterator(dir2))
    // {
    //     if (FS::is_regular_file(entry.path()))
    //     {
    //         string path_str = entry.path().string();
    //         files2.push_back(path_str);
    //     }
    // }

    // assert(files1.size() == files2.size());

    // sort(files1.begin(), files1.end());
    // sort(files2.begin(), files2.end());

    // int sz = files1.size();

    // for (int i = 0; i < sz; i++)
    // {
    //     // Get file path for phi_i(0) and phi_i(1)
    //     char *phi_0 = new char[files1[i].size() + 1];
    //     strcpy(phi_0, files1[i].c_str());

    //     char *phi_1 = new char[files2[i].size() + 1];
    //     strcpy(phi_1, files2[i].c_str());

    //     // Get their corresponding Networks
    //     Abc_Ntk_t *phi_0_Ntk = getNtkFromCNF(phi_0);
    //     Abc_Ntk_t *phi_1_Ntk = getNtkFromCNF(phi_1);

    //     // Append phi_i(1) into phi_i(0) network
    //     Abc_NtkAppend(phi_0_Ntk, phi_1_Ntk, 1);

    //     // Creating A[i]
    //     Aig_Man_t *tmp = Abc_NtkToDar(phi_0_Ntk, 0, 0);

    //     Aig_Obj_t *newNode = Aig_And(tmp, Aig_ManCo(tmp, 1)->pFanin0, Aig_Not(Aig_ManCo(tmp, 0)->pFanin0));
    //     Aig_ObjDeletePo(tmp, Aig_ManCo(tmp, 1));
    //     Aig_ObjDeletePo(tmp, Aig_ManCo(tmp, 0));

    //     Aig_ObjCreateCo(tmp, newNode);

    //     Aig_Man_t *Man_A = Aig_ManDupOneOutput(tmp, 0, 0);
    //     Abc_Ntk_t *Ntk_A = Abc_NtkFromAigPhase(Man_A);
    //     A_Man.push_back(Man_A);
    //     A_Ntk.push_back(Ntk_A);

    //     // Generating B[i]
    //     tmp = Abc_NtkToDar(phi_0_Ntk, 0, 0);

    //     // phi1 OR !phi_0
    //     Aig_Obj_t *newNode1 = Aig_Or(tmp, Aig_ManCo(tmp, 1)->pFanin0, Aig_Not(Aig_ManCo(tmp, 0)->pFanin0));

    //     // !phi OR phi0
    //     Aig_Obj_t *newNode2 = Aig_Or(tmp, Aig_ManCo(tmp, 0)->pFanin0, Aig_Not(Aig_ManCo(tmp, 1)->pFanin0));

    //     // newNode1 AND newNode2
    //     Aig_Obj_t *finalNewNode = Aig_And(tmp, newNode1, newNode2);

    //     Aig_ObjDeletePo(tmp, Aig_ManCo(tmp, 1));
    //     Aig_ObjDeletePo(tmp, Aig_ManCo(tmp, 0));

    //     Aig_ObjCreateCo(tmp, finalNewNode);

    //     Aig_Man_t *Man_B = Aig_ManDupOneOutput(tmp, 0, 0);
    //     Abc_Ntk_t *Ntk_B = Abc_NtkFromAigPhase(Man_B);
    //     B_Man.push_back(Man_B);
    //     B_Ntk.push_back(Ntk_B);
    // }
    cout << "SHOWING A\n";
    for (auto e : A_Man)
    {
        Aig_ManShow(e,0,NULL);
        cout<<"Enter 1\n";
        int x;
        cin>>x;
    }

    cout << "SHOWING B\n";
    for (auto e : B_Man)
    {
        Aig_ManShow(e,0,NULL);
        cout<<"Enter 1\n";
        int x;
        cin>>x;
    }

    Abc_Stop();

    return 0;
}