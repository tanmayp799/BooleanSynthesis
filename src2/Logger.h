// src2/Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
// #include <format>
#include <fmt/ranges.h>
#include <vector>
#include <map>
#include <fmt/core.h>
#include <fmt/format.h>
#include <filesystem>
// #include "ScopedTimer.h"
enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG,
    STATS
};

class Logger {
public:
    Logger();
    ~Logger();

    // Redirect logs to a file (optional)
    void setOutputFile(const std::string& filename);

    void closeOutputFile();

    // Core logging function
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const std::string& prefix, const std::vector<int>& vec);

private:
    std::ofstream logFile;
    std::mutex logMutex; // Ensures thread safety if you use threads later
    std::string levelToString(LogLevel level);
};

// Declare the global logger instance
extern Logger globalLogger;
extern Logger statisticsLogger;




struct ExperimentalMetrics {
    int approach_id = 4;                  // Approach 4: BDD Dynamic Reordering
    std::string benchmark_name = "";
    
    // Global formula metadata
    int count_a = 0;
    int count_e = 0;
    int count_d = 0;
    
    // Per-variable relational lists
    std::vector<int> d_vars;
    std::vector<int> individual_dep_set_sizes; 
    std::vector<double> individual_exis_quant_times;
    std::vector<double> individual_univ_quant_times;
    std::vector<double> individual_bdd_gen_times;
    std::vector<long long> individual_bdd_sizes;
    std::vector<long long> individual_aig_sizes;
    std::vector<bool> is_trivial_a;       // Corresponds to Condition 6: Is A=0?
    std::vector<bool> is_trivial_b;       // Corresponds to Condition 7: Is B=1?

    double tseitin_elimination_time = 0.0;
    double getBDD_time = 0.0;

    // Execution state tracking
    std::string last_checkpoint = "INIT";              
    int total_cegis_iterations = 0;
    double total_cegis_time = 0.0;
    
    std::string execution_status = "RUNNING";
    std::chrono::time_point<std::chrono::high_resolution_clock> start_timestamp;

    ExperimentalMetrics() {
        start_timestamp = std::chrono::high_resolution_clock::now();
    }

    void print_json_metrics() const {
        std::filesystem::path p(benchmark_name);
        std::string pure_name = p.stem().string(); 
        std::string json_output_path = "./experiment/stats/app" + std::to_string(approach_id) + "/" + pure_name + ".json";

        std::ofstream json_file(json_output_path);
        if (!json_file.is_open()) {
            std::cerr << "CRITICAL: Could not open JSON log file at " << json_output_path << std::endl;
            return;
        }

        auto end_timestamp = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> total_program_elapsed = end_timestamp - start_timestamp;
        double total_program_time = total_program_elapsed.count();

        // Calculate Average Dependency Set Size dynamically for quick summary reviews
        // double avg_dep_set_size = individual_dep_set_sizes.empty() ? 0.0 : 
        //     std::accumulate(individual_dep_set_sizes.begin(), individual_dep_set_sizes.end(), 0.0) / individual_dep_set_sizes.size();

        json_file << "{\n";
        json_file << "  \"approach_id\": " << approach_id << ",\n";
        json_file << "  \"benchmark_name\": \"" << p.filename().string() << "\",\n";
        json_file << "  \"count_a\": " << count_a << ",\n";
        json_file << "  \"count_e\": " << count_e << ",\n";
        json_file << "  \"count_d\": " << count_d << ",\n";
        // json_file << "  \"avg_dependency_set_size\": " << avg_dep_set_size << ",\n";
        json_file << "  \"total_cegis_iterations\": " << total_cegis_iterations << ",\n";
        json_file << "  \"last_checkpoint\": \"" << last_checkpoint << "\",\n";
        json_file << "  \"execution_status\": \"" << execution_status << "\",\n";
        json_file << "  \"total_cegis_time\": " << total_cegis_time << ",\n";
        json_file << "  \"total_program_time\": " << total_program_time << ",\n";
        
        // Per-Variable Vectors
        json_file << "  \"d_vars\": [";
        for (size_t i = 0; i < d_vars.size(); ++i) {
            json_file << d_vars[i] << (i < d_vars.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"dep_set_sizes\": [";
        for (size_t i = 0; i < individual_dep_set_sizes.size(); ++i) {
            json_file << individual_dep_set_sizes[i] << (i < individual_dep_set_sizes.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"exis_quant_times\": [";
        for (size_t i = 0; i < individual_exis_quant_times.size(); ++i) {
            json_file << individual_exis_quant_times[i] << (i < individual_exis_quant_times.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"univ_quant_times\": [";
        for (size_t i = 0; i < individual_univ_quant_times.size(); ++i) {
            json_file << individual_univ_quant_times[i] << (i < individual_univ_quant_times.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"bdd_gen_times\": [";
        for (size_t i = 0; i < individual_bdd_gen_times.size(); ++i) {
            json_file << individual_bdd_gen_times[i] << (i < individual_bdd_gen_times.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"bdd_sizes\": [";
        for (size_t i = 0; i < individual_bdd_sizes.size(); ++i) {
            json_file << individual_bdd_sizes[i] << (i < individual_bdd_sizes.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"aig_sizes\": [";
        for (size_t i = 0; i < individual_aig_sizes.size(); ++i) {
            json_file << individual_aig_sizes[i] << (i < individual_aig_sizes.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"is_trivial_a\": [";
        for (size_t i = 0; i < is_trivial_a.size(); ++i) {
            json_file << (is_trivial_a[i] ? 1 : 0) << (i < is_trivial_a.size() - 1 ? "," : "");
        }
        json_file << "],\n  \"is_trivial_b\": [";
        for (size_t i = 0; i < is_trivial_b.size(); ++i) {
            json_file << (is_trivial_b[i] ? 1 : 0) << (i < is_trivial_b.size() - 1 ? "," : "");
        }
        json_file << "]\n}\n";
        
        json_file.close();
    }
};

static ExperimentalMetrics global_metrics;
inline std::map<int,int> dep_to_id;

#endif // LOGGER_H
