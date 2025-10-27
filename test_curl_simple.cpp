#include <iostream>
#include <curl/curl.h>
#include <string>

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main() {
    CURL *curl;
    CURLcode res;
    
    curl = curl_easy_init();
    if(curl) {
        std::string readBuffer;
        
        // 测试简单的 HTTP GET 请求
        curl_easy_setopt(curl, CURLOPT_URL, "https://httpbin.org/get");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        std::cout << "Making HTTP request..." << std::endl;
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Request successful, received " << readBuffer.length() << " bytes" << std::endl;
            std::cout << "First 100 chars: " << readBuffer.substr(0, 100) << std::endl;
        }
        
        curl_easy_cleanup(curl);
    }
    
    return 0;
}
