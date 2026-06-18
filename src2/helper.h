#ifndef HELPER_H
#define HELPER_H

// #include "Dqbf.h"

// #include "AigWrapper.h"
#include "CadicalWrapper.h"
// #include "cadical.hpp"

std::vector<KissatWrapper*> generateLocalSpecs(Dqbf* origDqbf);


int cegis(Dqbf* origDqbf, CadicalWrapper* solverWrapper, CadicalWrapper* unsatCoreWrapper, CadicalWrapper* constraintWrapper, std::map<int,int> exToHMapping);






struct Metrics {
    // Structural Metadata
    int approach_id = 2;
    std::string benchmark_name = "";
    int count_a = 0;
    int count_e = 0;
    int count_d = 0; // Keeping structure identical to app1 layout

    // Algorithmic Tracking Counters
    int total_cegis_iterations = 0;
    double total_cegis_time = 0.0;
    std::string last_checkpoint = "NOT_STARTED";
    std::string execution_status = "RUNNING";

    // Timing Profiles (Seconds)
    std::chrono::high_resolution_clock::time_point start_timestamp;
    double total_program_time = 0.0;
    double manthan_time = 0.0; // Tracks total execution duration of Kissat BVE loops

    // Feature Arrays for Structural Analysis
    std::vector<int> d_vars;
    std::vector<int> dep_set_sizes;
    std::vector<double> projection_times;
    std::vector<int> is_trivial_a;
    std::vector<int> is_trivial_b;

    // Helper method to turn a primitive vector into a JSON array string
    template<typename T>
    std::string vector_to_json_array(const std::vector<T>& vec) {
        std::string json = "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            if constexpr (std::is_same_v<T, std::string>) {
                json += "\"" + vec[i] + "\"";
            } else {
                json += std::to_string(vec[i]);
            }
            if (i < vec.size() - 1) json += ", ";
        }
        json += "]";
        return json;
    }

    // Flushes all metrics safely into a structured file
    void print_json_metrics() {
        // Strip out the directory path to isolate just the raw filename for the JSON string
        std::string clean_name = benchmark_name;
        size_t last_slash = clean_name.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            clean_name = clean_name.substr(last_slash + 1);
        }

        std::string output_filename = "./experiment/stats/app2/" + clean_name;
        // Strip extension if it ends with .dqdimacs or .qdimacs to append .json
        size_t ext_dot = output_filename.find_last_of(".");
        if (ext_dot != std::string::npos) {
            output_filename = output_filename.substr(0, ext_dot);
        }
        output_filename += ".json";

        std::ofstream json_file(output_filename);
        if (!json_file.is_open()) {
            std::cerr << "CRITICAL ERROR: Could not write metrics tracking file to " << output_filename << std::endl;
            return;
        }

        // Format and flush valid JSON format matching Approach 1 precisely
        json_file << "{\n"
                  << "  \"approach_id\": " << approach_id << ",\n"
                  << "  \"benchmark_name\": \"" << clean_name << "\",\n"
                  << "  \"count_a\": " << count_a << ",\n"
                  << "  \"count_e\": " << count_e << ",\n"
                  << "  \"count_d\": " << count_d << ",\n"
                  << "  \"total_cegis_iterations\": " << total_cegis_iterations << ",\n"
                  << "  \"total_cegis_time\": " << total_cegis_time << ",\n"
                  << "  \"last_checkpoint\": \"" << last_checkpoint << "\",\n"
                  << "  \"execution_status\": \"" << execution_status << "\",\n"
                  << "  \"total_program_time\": " << total_program_time << ",\n"
                  << "  \"manthan_time\": " << manthan_time << ",\n" // Acts as BVE quantification timer
                  << "  \"d_vars\": " << vector_to_json_array(d_vars) << ",\n"
                  << "  \"dep_set_sizes\": " << vector_to_json_array(dep_set_sizes) << ",\n"
                  << "  \"projection_times\": " << vector_to_json_array(projection_times) << ",\n"
                  << "  \"is_trivial_a\": " << vector_to_json_array(is_trivial_a) << ",\n"
                  << "  \"is_trivial_b\": " << vector_to_json_array(is_trivial_b) << "\n"
                  << "}\n";

        json_file.close();
    }
};

// Declare your global wrapper variable
inline Metrics global_metrics;



#endif // "HELPER_H"