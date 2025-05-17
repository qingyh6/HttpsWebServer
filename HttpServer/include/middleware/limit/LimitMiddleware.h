
#pragma once
#include "../Middleware.h"
#include "../../http/HttpRequest.h"
#include "../../http/HttpResponse.h"
#include <chrono>
#include <mutex>

namespace http
{
namespace middleware
{

class LimitMiddleware: public Middleware 
{
public:
    // 构造函数
    // rate：令牌生成速率（个/秒）
    // capacity：桶最大容量（最多存多少个令牌）
    LimitMiddleware(int rate, int capacity);

    // 在请求处理前调用，用于限流
    void before(HttpRequest& request) override;
    void after(HttpResponse& response) override {};
private:
    // 补充令牌（根据时间推移）
    void refillTokens();

private:
    int rate_;            // 令牌生成速率（个/秒）
    int capacity_;        // 桶容量（最大令牌数）
    double tokens_;       // 当前可用令牌数（允许小数，更精确）
    std::chrono::steady_clock::time_point lastRefillTime_; // 上一次补充时间
    std::mutex mutex_;    // 保护多线程访问
};

} // namespace middleware
} // namespace http
