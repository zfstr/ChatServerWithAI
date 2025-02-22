#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <vector>
#include <string>
using json = nlohmann::json;
using namespace std;

// 回调函数，用于处理服务器响应
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* data) {
    size_t real_size = size * nmemb;
    data->append((char*)contents, real_size);
    return real_size;
}

string filterThinkTags(const string& input) {
    string result = input;
    size_t start = 0;

    // 查找所有 <think> 标签
    while ((start = result.find("<think>", start)) != string::npos) {
        size_t end = result.find("</think>", start);
        if (end != string::npos) {
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

string chatOneLoop(CURL* curl, CURLcode & res, json & jsData,vector<json> &vecmsg, string input)
{
    
    string responsecontext;
    json userData;
    userData["role"] = "user";
    userData["content"] = input;
    vecmsg.push_back(userData);
    jsData["messages"] = vecmsg;
    // 设置POST数据（JSON 格式）
    string jsonData = jsData.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    // 设置回调函数，用于捕获响应结果
    string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // 执行请求
    res = curl_easy_perform(curl);
    // 检查请求是否成功
    if(res != CURLE_OK) {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        responsecontext = "服务器请求失败";
    } else {
        json jsResponse = json::parse(response);
        responsecontext = filterThinkTags(jsResponse["message"]["content"]);
    }
    json assistantData;
    assistantData["role"] = "assistant";
    assistantData["content"] = responsecontext;
    vecmsg.push_back(assistantData);
    return responsecontext;
}



string chatWithAI(CURL* curl, CURLcode & res)
{
    json jsData;
    
    jsData["model"] =  "deepseek-r1:1.5b";
    jsData["stream"] = false;
    vector<json> vecmsg;
    string responsecontext;
    if(curl) {
        while (true)
        {
            string input;
            getline(std::cin, input);
            cout << input << endl;
            if (input == "")
            {
                break;
            }
            if (input == "-1")
            {
                break;
            }
            string output = chatOneLoop(curl, res, jsData, vecmsg, input);
            cout << output << endl;
        }
        
        
    }
    return responsecontext;
}

int main(int argc,char * argv[]) {
    CURL* curl;
    CURLcode res;
    
    // 初始化 libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    // 设置请求的 URL
    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.1.101:8000/api/chat");
    // 设置 POST 方法
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    // 设置 HTTP 头
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    cout << "请输入你的问题：" << endl;
    string output = chatWithAI(curl, res);
    cout << output << endl;
    // 清理资源
    // curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}

