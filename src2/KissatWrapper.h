#ifndef KISSAT_WRAPPER_H
#define KISSAT_WRAPPER_H

#include <vector>
#include <set>

extern "C" {
    #include "kissat.h"
    #include "eliminate.h"
    #include "dump.h"
}

#include "Logger.h"


// Forward declaration of the Kissat solver structure
struct kissat;
class Dqbf;

class KissatWrapper {
public:
    KissatWrapper();
    ~KissatWrapper();

    // Add a clause to the solver
    // void addClause(const std::vector<int>& clause);
    void addClause(const std::set<int>& clause);

    // Low-level add function (add literals, terminate clause with 0)
    // void add(int val);

    // Solve the instance
    // Returns 10 (SAT), 20 (UNSAT), or 0 (UNKNOWN)
    int solve();

    // Get the value of a literal (returns lit if true, -lit if false, 0 if unknown)
    // int getValue(int lit);

    //field modifiers
    void setVarsToEliminate(std::vector<int> ExistentialVarsToEliminate, std::vector<int> UniversalVarsToEliminate);
    void setNumVars(int numVars);
    void setOutputVar(int outputVar);
    void setDependencySet(std::vector<int> dependencySet);

    //field accessors
    std::vector<int> getExistentialVarsToEliminate();
    std::vector<int> getUniversalVarsToEliminate();
    int getOutputVar();
    std::vector<int> getDependencySet();
    int getNumVars();
    std::vector<std::vector<int>> getLocalSpec();


    int eliminateExistentialVars();
    void eliminateUniversalVars();

    // Dqbf* getDqbf();
    void printClauses();

private:
    struct kissat* solver;
    std::vector<int> ExistentialVarsToEliminate;
    std::vector<int> UniversalVarsToEliminate;
    int outputVar;
    std::vector<int> dependencySet;
    int numVars;
    std::vector<std::vector<int>> localSpec;
};

#endif // KISSAT_WRAPPER_H