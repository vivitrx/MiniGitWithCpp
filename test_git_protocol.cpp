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
        
        // 测试 Git 协议请求
        std::string url = "https://github.com/schacon/simplegit.git/git-upload-pack";
        std::string postdata = "0032want ca82a6dff817ec66f44342007202690a93763949\n00000009done\n";
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        
        // 设置 HTTP 头
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-git-upload-pack-request");
        headers = curl_slist_append(headers, "Accept: application/x-git-upload-pack-result");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        std::cout << "Making Git protocol request to: " << url << std::endl;
        std::cout << "Request data: " << postdata << std::endl;
        
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Request successful, received " << readBuffer.length() << " bytes" << std::endl;
            std::cout << "First 200 chars: " << readBuffer.substr(0, 200) << std::endl;
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    return 0;
}
