#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <QString>

#include <curl/curl.h>
#include "validity.hpp"

namespace lockifi
{

namespace http
{

using namespace std::literals::string_literals;

std::pair<long, std::string> get(QString url, long timeout_ms=1000)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    
    if (curl)
    {        
        long code=0; std::string body;
        
        res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
        if (res != CURLE_OK) goto curl_error;
        
        auto url2 = url.toStdString();//keep lifetime for surity
        res = curl_easy_setopt(curl, CURLOPT_URL, url2.c_str());
        if (res != CURLE_OK) goto curl_error;
        
        res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[] // unary + decays lambda to function ptr
        // callback may be called multiple times with partial data
        (void* in_ptr, size_t size_elem, size_t num_elem, void* out_ptr)
        {
            auto& body = *(std::string*)out_ptr;
            auto  size = size_elem * num_elem;
            auto  oldsize = body.size();
            
            body.resize(oldsize + size);
            std::memcpy(&body[oldsize], in_ptr, size);
            
            return size;
        });
        if (res != CURLE_OK) goto curl_error;
        
        res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        if (res != CURLE_OK) goto curl_error;
        
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) goto curl_error;
        
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if (res != CURLE_OK) goto curl_error;
        
        curl_easy_cleanup(curl);
        return {code, body};
    }
    else throw std::runtime_error{"lockifi::http::get -> libcurl initialization faliure"};
    
curl_error:
    curl_easy_cleanup(curl);
    throw std::runtime_error{"lockifi::http::get -> "s + curl_easy_strerror(res)};
}

}

inline void validate_ip(const QString& ipv4_str)
{
    if (not is_valid_ip(ipv4_str))
        throw std::runtime_error{"invalid ip selected"};
}

bool ping(QString lock_ip) noexcept
{
    try
    {
        validate_ip(lock_ip);
        auto [code,body] = http::get("http://"+lock_ip+"/ping", 500);
        return code == 200 && body == "UwU";
    }
    catch(...) {return false;}
}

void add_user(QString lock_ip, QString mac, QString name, uint8_t level)
{
    validate_ip(lock_ip); mac.replace(':',""); name.replace(' ', '~');
    auto [code,body] = http::get("http://"+lock_ip+"/add_user?mac="+mac+"&name="+name+"&lvl="+QString("%1").arg((int)level));
    
    if (not (code == 200 and body == "OK"))
        throw std::runtime_error{"lockifi::add_user -> Error Adding User\n"
                                 "code = "+std::to_string(code)+"\n"
                                 "body = "+body+'\n'};
}

void remove_user(QString lock_ip, QString mac)
{
    validate_ip(lock_ip); mac.replace(':',"");
    auto [code,body] = http::get("http://"+lock_ip+"/remove_user?mac="+mac);
    
    if (not (code == 200 and body == "OK"))
        throw std::runtime_error{"lockifi::remove_user -> Error Removing User\n"
                                 "code = "+std::to_string(code)+"\n"
                                 "body = "+body+'\n'};
}

QString check_user(QString lock_ip, QString mac)
{
    validate_ip(lock_ip); mac.replace(':',"");
    auto [code,body] = http::get("http://"+lock_ip+"/check_user?mac="+mac);
    if (code == 200) return body.c_str();
    else throw std::runtime_error{"lockifi: unable to check for user, code "+std::to_string(code)+" returned"};
}

auto user_list(QString lock_ip)
{
    validate_ip(lock_ip);
    auto [code,body] = http::get("http://"+lock_ip+"/user_list");
    
    if (code == 200)
    {
        uint8_t level;
        std::string line,mac,name;
        std::stringstream ss{body};
        std::vector<std::tuple<QString, QString, uint8_t>> user_list;
        
        while (std::getline(ss, line))
        {
            level = line[0] - '0';
            mac   = line.substr(2,19);
            name  = line.substr(20);
            user_list.emplace_back(mac.c_str(), name.c_str(), level);
        }
        
        return user_list;
    }
    else throw std::runtime_error{"lockifi: unable to fetch user list, code "+std::to_string(code)+" returned"};
    
}

std::string access_logs(QString lock_ip)
{
    validate_ip(lock_ip);
    auto [code,body] = http::get("http://"+lock_ip+"/access_logs");
    if (code == 200) return body;
    else throw std::runtime_error{"lockifi: unable to fetch access logs, code "+std::to_string(code)+" returned"};
}

}
