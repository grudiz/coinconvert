#pragma once

#include <string>

#include "error.hpp"

namespace coinconvert {

class HttpClient {
public:
    virtual ~HttpClient() = default;
    virtual std::string get(const std::string& url) = 0;
};

class CurlClient : public HttpClient {
public:
    std::string get(const std::string& url) override;
};

inline std::string fetch(HttpClient& http, const std::string& provider, const std::string& url) {
    try {
        return http.get(url);
    } catch (const FailError& err) {
        throw FailError(err.kind, provider + ": " + err.what());
    }
}

}
