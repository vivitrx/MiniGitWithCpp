#include "include/clone_gadget.h"
#include <iostream>

int main() {
    try {
        std::cerr << "Testing curl_request..." << std::endl;
        auto [pack, packhash] = curl_request("https://github.com/schacon/simplegit.git");
        std::cerr << "packhash length: " << packhash.length() << std::endl;
        std::cerr << "packhash content: " << packhash << std::endl;
        std::cerr << "pack length: " << pack.length() << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
