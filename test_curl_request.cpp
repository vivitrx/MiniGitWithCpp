#include <iostream>
#include <string>
#include "include/clone_gadget.h"

int main() {
    std::cerr << "Testing curl_request function..." << std::endl;
    
    try {
        auto [pack, packhash] = curl_request("https://github.com/schacon/simplegit.git");
        
        std::cerr << "curl_request completed successfully" << std::endl;
        std::cerr << "packhash length: " << packhash.length() << std::endl;
        std::cerr << "packhash content: " << packhash << std::endl;
        std::cerr << "pack length: " << pack.length() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        return 1;
    }
    
    return 0;
}
