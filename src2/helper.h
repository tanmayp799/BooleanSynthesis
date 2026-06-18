#ifndef HELPER_H
#define HELPER_H

// #include "Dqbf.h"

// #include "AigWrapper.h"
#include "CadicalWrapper.h"
#include "nnf.h"
// #include "cadical.hpp"


extern bool didManthan;
// extern int numOrigInputs;

// std::vector<KissatWrapper*> generateLocalSpecs(Dqbf* origDqbf);


// extern AigWrapper* skolemAig;

int cegis(Dqbf* origDqbf, CadicalWrapper* solverWrapper, CadicalWrapper* unsatCoreWrapper, CadicalWrapper* constraintWrapper, std::map<int,int> exToHMapping);

int verify(AigWrapper* finalFormula, Dqbf* origDqbf, std::vector<std::pair<int, AigWrapper*>>& tseitinSKolems, char* argv[]);

void getBDD(AigWrapper* formula, DdManager* &ddMan, DdNode* &FddNode, Abc_Ntk_t* &pNtk);

AigWrapper* quantify(Abc_Ntk_t* pNtk, DdManager* ddMan, DdNode* FddNode, std::vector<int> &varsToEliminate);
// void quantify2(Aig_Man_t* pMan, std::vector<int>& exisVarsToElim);

std::vector<std::pair<int, AigWrapper*>> getTseitinSkolems(Aig_Man_t* SAig, std::vector<int> existentialVarsToEliminate);
// void getMonoAig(Aig_Man_t* pMan);
// AigWrapper* getMonotonicCircuit(AigWrapper* formula);


// struct ExperimentalMetrics {
//     int approach_id = 3;                  // Updated explicitly for Approach 3
//     std::string benchmark_name = "";
    
//     // Global formula metadata
//     int count_a = 0;
//     int count_e = 0;
//     int count_d = 0;
    
//     // Per-variable relational tracking vectors (Parallel lists resized to count_d)
//     std::vector<int> d_vars;
//     std::vector<int> individual_dep_set_sizes; 
//     std::vector<double> individual_projection_times;
//     std::vector<bool> is_trivial_a;
//     std::vector<bool> is_trivial_b;
    
//     // Execution state and loop metrics
//     std::string last_checkpoint = "INIT";              
//     int total_cegis_iterations = 0;
//     double total_cegis_time = 0.0;
//     double manthan_time = 0.0;            // Re-mapped to profile BDD/NNF compilation duration
//     std::string execution_status = "RUNNING";
    
//     // Initial clock timestamp
//     std::chrono::time_point<std::chrono::high_resolution_clock> start_timestamp;

//     // Inline constructor automatically guards against the Epoch (1970) timestamp trap
//     ExperimentalMetrics() {
//         start_timestamp = std::chrono::high_resolution_clock::now();
//     }

//     // Generates a valid JSON string containing all scalar data and vectors
//     void print_json_metrics() const {
//         std::filesystem::path p(benchmark_name);
//         std::string pure_name = p.stem().string(); 
//         std::string json_output_path = "./experiment/stats/app" + std::to_string(approach_id) + "/" + pure_name + ".json";

//         std::ofstream json_file(json_output_path);
//         if (!json_file.is_open()) {
//             std::cerr << "CRITICAL: Could not open JSON log file at " << json_output_path << std::endl;
//             return;
//         }

//         auto end_timestamp = std::chrono::high_resolution_clock::now();
//         std::chrono::duration<double> total_program_elapsed = end_timestamp - start_timestamp;
//         double total_program_time = total_program_elapsed.count();

//         json_file << "{\n";
//         json_file << "  \"approach_id\": " << approach_id << ",\n";
//         json_file << "  \"benchmark_name\": \"" << p.filename().string() << "\",\n";
//         json_file << "  \"count_a\": " << count_a << ",\n";
//         json_file << "  \"count_e\": " << count_e << ",\n";
//         json_file << "  \"count_d\": " << count_d << ",\n";
//         json_file << "  \"total_cegis_iterations\": " << total_cegis_iterations << ",\n";
//         json_file << "  \"total_cegis_time\": " << total_cegis_time << ",\n";
//         json_file << "  \"last_checkpoint\": \"" << last_checkpoint << "\",\n";
//         json_file << "  \"execution_status\": \"" << execution_status << "\",\n";
//         json_file << "  \"total_program_time\": " << total_program_time << ",\n";
//         json_file << "  \"manthan_time\": " << manthan_time << ",\n";
        
//         json_file << "  \"d_vars\": [";
//         for (size_t i = 0; i < d_vars.size(); ++i) {
//             json_file << d_vars[i] << (i < d_vars.size() - 1 ? "," : "");
//         }
//         json_file << "],\n  \"dep_set_sizes\": [";
//         for (size_t i = 0; i < individual_dep_set_sizes.size(); ++i) {
//             json_file << individual_dep_set_sizes[i] << (i < individual_dep_set_sizes.size() - 1 ? "," : "");
//         }
//         json_file << "],\n  \"projection_times\": [";
//         for (size_t i = 0; i < individual_projection_times.size(); ++i) {
//             json_file << individual_projection_times[i] << (i < individual_projection_times.size() - 1 ? "," : "");
//         }
//         json_file << "],\n  \"is_trivial_a\": [";
//         for (size_t i = 0; i < is_trivial_a.size(); ++i) {
//             json_file << (is_trivial_a[i] ? 1 : 0) << (i < is_trivial_a.size() - 1 ? "," : "");
//         }
//         json_file << "],\n  \"is_trivial_b\": [";
//         for (size_t i = 0; i < is_trivial_b.size(); ++i) {
//             json_file << (is_trivial_b[i] ? 1 : 0) << (i < is_trivial_b.size() - 1 ? "," : "");
//         }
//         json_file << "]\n}\n";
        
//         json_file.close();
//     }
// };

// static ExperimentalMetrics global_metrics;

#endif // "HELPER_H"