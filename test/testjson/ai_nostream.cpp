// #ifndef CHATSERVICE_H
// #define CHATSERVICE_H

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
class AI
{
public:
    AI(string ainame)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT); 
        this->curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.1.101:8000/api/chat");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        jsData["model"] =  ainame;
        jsData["stream"] = false;

    }
    string chat(string input)
    {
        string responsecontext;
        json userData;
        string response;
        userData["role"] = "user";
        userData["content"] = input;
        vecmsg.push_back(userData);
        jsData["messages"] = vecmsg;
        string jsonData = jsData.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            responsecontext = "服务器请求失败";
        } 
        else 
        {
            json jsResponse = json::parse(response);
            responsecontext = filterThinkTags(jsResponse["message"]["content"]);
        }
        json assistantData;
        assistantData["role"] = "assistant";
        assistantData["content"] = responsecontext;
        vecmsg.push_back(assistantData);
        cout << get_message_size() << endl;
        return responsecontext;
    }
    ~AI()
    {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
private:
    CURL* curl;
    CURLcode res;
    struct curl_slist* headers;
    json jsData;
    vector<json> vecmsg;
    int get_message_size()
    {
        return vecmsg.size();
    }
};

int main()
{
    AI ai("deepseek-r1:1.5b");
    
    cout << "你好！我是AI" << endl;
    while(true)
    {
        string input;
        getline(std::cin, input);
        string out = ai.chat(input);
        cout << out << endl;
    }
    return 0;
}


// #endif