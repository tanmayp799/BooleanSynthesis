// src2/Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#define FMT_HEADER_ONLY 1
#define FMT_STATIC_THROWS 1

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <vector>
#include <filesystem>
#include <csignal>

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


struct ExperimentalMetrics {
    int approach_id = 2;                  // Set this based on your current Git branch/commit
    std::string benchmark_name = "";
    
    // Global formula metadata
    int count_a = 0;
    int count_e = 0;
    int count_d = 0;
    
    // Per-variable relational tracking vectors (Parallel lists resized to count_d)
	std::vector<int> d_vars;
    std::vector<int> individual_dep_set_sizes; 
    std::vector<double> individual_kissat_times;
    std::vector<bool> is_trivial_a;
    std::vector<bool> is_trivial_b;
    
    // Execution state and loop metrics
    std::string last_checkpoint = "INIT";              // 1=Boot, 2=Parsing/Prep, 3=Manthan Done, 4=Projection Loop, 5=Success
    int total_cegis_iterations = 0;
    double total_cegis_time = 0.0;
	// double manthan_time = 0.0;
    std::string execution_status = "RUNNING";
    
    // Initial clock timestamp
    std::chrono::time_point<std::chrono::high_resolution_clock> start_timestamp;

    // Generates a valid JSON string containing all scalar data and vectors
    void print_json_metrics() const {
    // 1. Construct a clean output filename (e.g., bin/benchmarks/cnt20y.dqdimacs -> ./logs/app1_cnt20y.json)
    std::filesystem::path p(benchmark_name);
    std::string pure_name = p.stem().string(); // Extract "cnt20y"
    std::string json_output_path = "./experiment/stats/app" + std::to_string(approach_id) + "/" + pure_name + ".json";

    // 2. Open the dedicated json file stream
    std::ofstream json_file(json_output_path);
    if (!json_file.is_open()) {
        // Fallback to standard error if directory doesn't exist so you don't lose data entirely
        std::cerr << "CRITICAL: Could not open JSON log file at " << json_output_path << std::endl;
        return;
    }

    auto end_timestamp = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_program_elapsed = end_timestamp - start_timestamp;
    double total_program_time = total_program_elapsed.count();

    // 3. Write directly to the file stream instead of cout
    json_file << "{\n";
    json_file << "  \"approach_id\": " << approach_id << ",\n";
    json_file << "  \"benchmark_name\": \"" << p.filename().string() << "\",\n";
    json_file << "  \"count_a\": " << count_a << ",\n";
    json_file << "  \"count_e\": " << count_e << ",\n";
    json_file << "  \"count_d\": " << count_d << ",\n";
    json_file << "  \"total_cegis_iterations\": " << total_cegis_iterations << ",\n";
    json_file << "  \"total_cegis_time\": " << total_cegis_time << ",\n";
    json_file << "  \"last_checkpoint\": \"" << last_checkpoint << "\",\n";
    json_file << "  \"execution_status\": \"" << execution_status << "\",\n";
    json_file << "  \"total_program_time\": " << total_program_time << ",\n";
	// json_file << "  \"manthan_time\": " << manthan_time << ",\n";
    

	json_file << "  \"d_vars\": [";
    for (size_t i = 0; i < d_vars.size(); ++i) {
        json_file << d_vars[i] << (i < d_vars.size() - 1 ? "," : "");
    }
    // json_file << "  \"dep_set_sizes\": [";
	json_file << "],\n  \"dep_set_sizes\": [";
    for (size_t i = 0; i < individual_dep_set_sizes.size(); ++i) {
        json_file << individual_dep_set_sizes[i] << (i < individual_dep_set_sizes.size() - 1 ? "," : "");
    }
    json_file << "],\n  \"individual_kissat_times\": [";
    for (size_t i = 0; i < individual_kissat_times.size(); ++i) {
        json_file << individual_kissat_times[i] << (i < individual_kissat_times.size() - 1 ? "," : "");
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

// 1. Instantiated as a global instance to make it universally accessible to handlers
inline ExperimentalMetrics global_metrics;


#endif // LOGGER_H
