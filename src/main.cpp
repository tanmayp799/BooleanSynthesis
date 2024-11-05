
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
#include "helper.h"

using namespace std;

////////////////////////////////////////////////////////////////////////
///                           GLOBALS                                ///
////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
	
    Abc_Start();


	Aig_Man_t* manager;
	manager= Aig_ManStart(0);
    // cout<<manager->nFansAlloc<<endl;

    cout<<Aig_ManCiNum(manager)<<endl;
    
    Aig_Obj_t* p1;
    Aig_Obj_t* p2;
    Aig_Obj_t* p3;
    Aig_Obj_t* p4;
    p1 = Aig_ObjCreateCi(manager);
    p2 = Aig_ObjCreateCi(manager);
    p3 = Aig_ObjCreateCi(manager);
    p4 = Aig_ObjCreateCi(manager);

    cout<<Aig_ManCiNum(manager)<<endl;

    Aig_Obj_t* or_1 = Aig_Or(manager, p1, p2);
    Aig_Obj_t* or_2 = Aig_Or(manager, p1, p4);

    Aig_Obj_t* and_1 = Aig_And(manager, or_1, or_2);

    Aig_Obj_t* po1 = Aig_ObjCreateCo(manager, Aig_Not(and_1));
    
    Aig_Obj_t* po2 = Aig_ObjCreateCo(manager, or_1);


    sat_solver* solver = sat_solver_new();


    
    Aig_ManShow(manager, 0, NULL);

    Abc_Stop();

	return 0;
}