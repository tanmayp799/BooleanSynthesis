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

int main(int argc, char* argv[]){
    Abc_Start();
    srand(time(0));

    CaDiCaL::Solver solver;
    int sig;
    

    string inpPath = argv[1];
    FS::path phiPath(inpPath);
    DQCNF *phiCNF = new DQCNF(phiPath.string());

    Aig_Man_t* phiMan = phiCNF->genAIGMan();

    return 0;
}