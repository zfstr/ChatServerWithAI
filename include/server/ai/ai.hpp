#ifndef AI_H
#define AI_H
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <vector>
#include <string>
using json = nlohmann::json;
using namespace std;

class AI
{
public:
    AI();
    string chat(string input);
    ~AI();

private:
    CURL* curl;
    CURLcode res;
    struct curl_slist* headers;
    json jsData;
    vector<json> vecmsg;
    int get_message_size();
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* data);
    string filterThinkTags(const string& input);
};


#endif