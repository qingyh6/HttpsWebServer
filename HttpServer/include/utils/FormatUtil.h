#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <muduo/base/Logging.h>

class FormatUtil
{
public:

   //对前端使用encodeURIComponent 的参数解析解码
    std::string url_decode(const std::string& encoded) {
        std::string result;
        result.reserve(encoded.size()); // 预分配
        for (size_t i = 0; i < encoded.length(); ++i) {
            if (encoded[i] == '%' && i + 2 < encoded.length()) {
                char hex[3] = { encoded[i + 1], encoded[i + 2], '\0' };
                char* end;
                long int val = strtol(hex, &end, 16);
                if (*end == '\0') {
                    result += static_cast<char>(val);
                    i += 2;
                } else {
                    // 非法的 % 编码，保留原字符
                    result += '%';
                }
            } else if (encoded[i] == '+') {
                result += ' ';
            } else {
                result += encoded[i];
            }
        }
        return result;
    }

    // 去除字符串前后空白符
    std::string trim(const std::string& str) {
        const auto strBegin = str.find_first_not_of(" \t\r\n");
        if (strBegin == std::string::npos) return ""; // 全是空白

        const auto strEnd = str.find_last_not_of(" \t\r\n");
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

};