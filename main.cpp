#include <iostream>
#include "Limit_Order_Book/Book.hpp"
#include "itch_parser/itch_parser.hpp"
#include "Limit_Order_Book/rdtsc.hpp" 

int main() {
    Book matching_engine;
    
    std::cout << "Initializing NanoMatch Engine...\n";
    std::cout << "Loading real-world order data into Memory Pool...\n";
    
    uint64_t start_time = rdtsc();
    
    ItchParser::processFile("data/small_sample.itch", matching_engine);
    
    uint64_t end_time = rdtsc();
    
    std::cout << "Engine processing complete.\n";
    
    // Convert cycles to milliseconds (Assuming roughly ~3.0 GHz CPU)
    double time_taken_ms = (end_time - start_time) / 3000000.0; 
    std::cout << "Time taken: " << time_taken_ms << " ms\n";
    
    return 0;
}
