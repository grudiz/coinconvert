#include "http.hpp"

#include <curl/curl.h>

#include <exception>
#include <sstream>

#include "json.hpp"

namespace coinconvert {
namespace {

struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
    CurlGlobal(const CurlGlobal&) = delete;
    CurlGlobal& operator=(const CurlGlobal&) = delete;
};

struct CurlSession {
    CURL* easy = nullptr;
    curl_slist* headers = nullptr;

    CurlSession() {
        easy = curl_easy_init();
        if (easy == nullptr) {
            throw FailError(Fail::Runtime, "could not start an HTTP request");
        }
        headers = curl_slist_append(nullptr, "Accept: application/json");
    }

    ~CurlSession() {
        if (headers != nullptr) {
            curl_slist_free_all(headers);
        }
        if (easy != nullptr) {
            curl_easy_cleanup(easy);
        }
    }

    CurlSession(const CurlSession&) = delete;
    CurlSession& operator=(const CurlSession&) = delete;
};

size_t write_body(char* ptr, size_t size, size_t nmemb, void* userdata) {
    const size_t bytes = size * nmemb;
    auto* body = static_cast<std::string*>(userdata);
    if (body->size() + bytes > 1000000) {
        return 0;
    }
    body->append(ptr, bytes);
    return bytes;
}

std::string clip(const std::string& body) {
    std::string out;
    for (unsigned char c : body) {
        if (out.size() >= 180) {
            break;
        }
        if (c == '\n' || c == '\r' || c == '\t') {
            out.push_back(' ');
        } else if (c >= 32 && c < 127) {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

std::string json_detail(const std::string& body) {
    try {
        const Json root = Json::parse(body);
        if (const Json* status = root.find("status"); status != nullptr && status->is_object()) {
            if (const Json* message = status->find("error_message");
                message != nullptr && message->is_string() && !message->string().empty()) {
                return message->string();
            }
        }
        if (const Json* error = root.find("error"); error != nullptr && error->is_string() && !error->string().empty()) {
            return error->string();
        }
        if (const Json* message = root.find("message");
            message != nullptr && message->is_string() && !message->string().empty()) {
            return message->string();
        }
        if (const Json* msg = root.find("msg"); msg != nullptr && msg->is_string() && !msg->string().empty()) {
            return msg->string();
        }
    } catch (const std::exception&) {
        return "";
    }
    return "";
}

std::string http_failure(long code, const std::string& body) {
    std::ostringstream message;
    message << "request failed with HTTP " << code;
    if (body.empty() || body[0] == '<') {
        return message.str();
    }
    std::string detail = json_detail(body);
    if (detail.empty()) {
        detail = clip(body);
    } else if (detail.size() > 180) {
        detail.resize(180);
    }
    if (!detail.empty()) {
        message << ": " << detail;
    }
    return message.str();
}

}  // namespace

std::string CurlClient::get(const std::string& url) {
    static CurlGlobal global;
    CurlSession session;
    std::string body;
    char errbuf[CURL_ERROR_SIZE];
    errbuf[0] = '\0';

    curl_easy_setopt(session.easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(session.easy, CURLOPT_HTTPHEADER, session.headers);
    curl_easy_setopt(session.easy, CURLOPT_USERAGENT, "coinconvert/1.0");
    curl_easy_setopt(session.easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(session.easy, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(session.easy, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(session.easy, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(session.easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(session.easy, CURLOPT_WRITEFUNCTION, write_body);
    curl_easy_setopt(session.easy, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(session.easy, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(session.easy, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(session.easy, CURLOPT_SSL_VERIFYHOST, 2L);
#if CURL_AT_LEAST_VERSION(7, 85, 0)
    curl_easy_setopt(session.easy, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(session.easy, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
    curl_easy_setopt(session.easy, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
    curl_easy_setopt(session.easy, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
#endif

    const CURLcode result = curl_easy_perform(session.easy);
    if (result != CURLE_OK) {
        const char* detail = errbuf[0] != '\0' ? errbuf : curl_easy_strerror(result);
        throw FailError(Fail::Runtime, std::string("network request failed: ") + detail);
    }

    long code = 0;
    curl_easy_getinfo(session.easy, CURLINFO_RESPONSE_CODE, &code);
    if (code != 200) {
        throw FailError(Fail::Runtime, http_failure(code, body));
    }
    return body;
}

}  // namespace coinconvert
