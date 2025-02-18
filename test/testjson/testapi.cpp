#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

// 回调函数，用于处理服务器响应
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    size_t real_size = size * nmemb;
    data->append((char*)contents, real_size);
    return real_size;
}

std::string filterThinkTags(const std::string& input) {
    std::string result = input;
    size_t start = 0;

    // 查找所有 <think> 标签
    while ((start = result.find("<think>", start)) != std::string::npos) {
        size_t end = result.find("</think>", start);
        if (end != std::string::npos) {
            // 删除 <think> 和 </think> 之间的内容
            result.erase(start, end - start + 8); // 8 是 </think> 的长度
        } else {
            // 如果没有找到 </think>，删除从 <think> 开始的剩余部分
            result.erase(start);
            break;
        }
    }

    return result;
}

int main(int argc,char * argv[]) {
    CURL* curl;
    CURLcode res;

    // 初始化 libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    json jsData;
    jsData["model"] =  "deepseek-r1:1.5b";
    jsData["prompt"] = argv[1];
    jsData["stream"] = false;

    if(curl) {
        // 设置请求的 URL
        curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.1.101:8000/api/generate");

        // 设置 POST 方法
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // 设置 POST 数据（JSON 格式）
        std::string jsonData = jsData.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        // 设置 HTTP 头
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 设置回调函数，用于捕获响应结果
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // 执行请求
        res = curl_easy_perform(curl);

        // 检查请求是否成功
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            // std::cout << "Server Response: " << std::endl;
            json jsResponse = json::parse(response);
            std::string responsecontext = filterThinkTags(jsResponse["response"]);
            std::cout << responsecontext << std::endl;
        }

        // 清理资源
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return 0;
}

