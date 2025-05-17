#pragma once

#include "../Middleware.h"
#include "../../http/HttpRequest.h"
#include "../../http/HttpResponse.h"

namespace http 
{
namespace middleware 
{

class GzipMiddleware : public Middleware 
{
public:
    GzipMiddleware():clientSupportGzip_(true){};
    void before(HttpRequest& request) override;
    void after(HttpResponse& response) override;
    void setClientSupportGzip(bool flag){clientSupportGzip_=flag;}
    bool isClinetSupportGzip() const {return clientSupportGzip_;}
private:
    bool compressGzip(const std::string& input, std::string& output);
    bool clientSupportGzip_;
};

} // namespace middleware
} // namespace http