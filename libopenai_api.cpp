#include "libopenai_api.h"

#include <assert.h>
#include <curl/curl.h>
#include <unistd.h>

#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>

#define log_dbg(fmt, ...) \
    printf("%s %s %ld " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define log_err log_dbg
#define log_info log_dbg

class CurlAsync {
public:
    enum method_t {
        GET,
        POST
    };
    CurlAsync()
        : m_curl(nullptr)
        , m_slist(nullptr)
        , m_multi_handle(nullptr)
        , m_has_stop(false)
        , m_answer("")
        , m_init(false)
    {
    }

    CurlAsync(const CurlAsync& other) = delete;

    ~CurlAsync() { curl_cleanup(); }

    // 回调函数，用于处理返回的数据
    static size_t write_callback(char* ptr,
        size_t size,
        size_t nmemb,
        void* userdata)
    {
        if (!userdata)
            return CURLE_WRITE_ERROR;

        CurlAsync* curl_async = (CurlAsync*)userdata;

        if (size * nmemb) {
            curl_async->m_has_res = true;
        }

        // 处理返回的数据，这里只是简单地将其输出到控制台
        // log_dbg("size: %zu, nmemb:%zu\n", size, nmemb);
        if (curl_async->m_has_stop) {
            curl_async->m_has_stop = 0;

            std::stringstream ss(ptr);
            std::string line;
            bool print_skip_data = false;

            while (std::getline(ss, line)) {
                if (0 != strncmp("data: ", line.c_str(), 6)) {
                    if (print_skip_data || (line.length() && '{' == line[0])) {
                        log_dbg("skip data: %s", line.c_str());
                        print_skip_data = true;
                    }
                    continue;
                }
                std::string data(line.c_str() + 6, line.length() - 6);

                if ('{' != data[0]) {
                    continue;
                }
                try {
                    nlohmann::json res = nlohmann::json::parse(data.c_str());
                    if (!res.is_null()) {
                        curl_async->m_res.clear();
                        curl_async->m_res.push_back(res);
                        std::cout << res.dump() << std::endl;
                    }
                    if (res["choices"][0]["finish_reason"].is_null()) {
                        std::string content = res["choices"][0]["delta"]["content"];
                        curl_async->m_answer += content;
                        std::string model = res["model"];
                        curl_async->m_model = model;
                        // std::cout << "answer: " << content << std::endl;
                        // std::cout << curl_async->m_answer << std::endl;
                    }
                } catch (const nlohmann::json::exception& e) {
                    // 处理键不存在的情况
                    std::cerr << "JSON 错误: " << e.what() << std::endl;
                    std::cout << "err data: " << data.length() << "\n```\n"
                              << data << "\n```\n"
                              << std::endl;
                }
            }
            return size * nmemb;
        } else {
            curl_async->m_has_stop = 1;
            return CURL_WRITEFUNC_PAUSE;
        }
    }

    int curl_init(method_t method,
        std::string& url,
        std::vector<std::string>& headers,
        std::string& body)
    {
        this->m_method = method;
        this->m_url = url;
        this->m_headers = headers;
        this->m_body = body;

        CURLcode code;
        curl_global_init(CURL_GLOBAL_DEFAULT);

        this->m_curl = curl_easy_init();
        if (!this->m_curl) {
            log_err("fail to init curl easy.");
            return -1;
        }

        // 设置要请求的URL
        curl_easy_setopt(this->m_curl, CURLOPT_URL, this->m_url.c_str());
        log_dbg("url: %s", this->m_url.c_str());

        // 设置HTTP头部参数
        for (const auto& header : this->m_headers) {
            struct curl_slist* tmp = curl_slist_append(this->m_slist, header.c_str());
            if (!tmp) {
                log_dbg("fail to append head: %s", header.c_str());
                goto error;
            }
            this->m_slist = tmp;
            log_dbg("append head: %s", header.c_str());
        }
        curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, this->m_slist);

        // 设置请求体内容
        curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, this->m_body.c_str());
        log_dbg("body: %s", body.c_str());

        // 设置回调函数，用于处理返回的数据
        curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, this->write_callback);
        curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, this);

        this->m_multi_handle = curl_multi_init();
        if (!this->m_multi_handle) {
            log_err("fail to init curl multi\n");
            goto error;
        }
        curl_multi_add_handle(this->m_multi_handle, this->m_curl);

        this->m_init = true;

        log_dbg("curl init done, %p %p", this->m_curl, this->m_multi_handle);

        return 0;
    error:
        curl_cleanup();
        return -1;
    }

    int pull()
    {
        CURLMcode mc;
        int numfds;

        do {
            mc = curl_multi_perform(this->m_multi_handle, &this->still_running);
            if (mc == CURLM_OK) {
                /* wait for activity or timeout */
                mc = curl_multi_poll(this->m_multi_handle, NULL, 0, 1000, &numfds);
            } else {
                log_err("curl_multi failed, code %d.\n", mc);
                return 0;
            }

            if (this->m_has_stop) {
                // 恢复传输
                CURLcode res = curl_easy_pause(this->m_curl, CURLPAUSE_CONT);
                if (res != CURLE_OK) {
                    log_err("curl_easy_pause() failed: %s\n", curl_easy_strerror(res));
                }
                // log_dbg("restore write.");
            }
        } while (this->still_running && !this->m_has_res);

        return this->still_running;
    }

    void curl_cleanup()
    {
        if (this->m_multi_handle) {
            if (this->m_curl) {
                curl_multi_remove_handle(this->m_multi_handle, this->m_curl);
            }
            curl_multi_cleanup(this->m_multi_handle);
            this->m_multi_handle = nullptr;
        }
        if (this->m_slist) {
            curl_slist_free_all(this->m_slist);
            this->m_slist = nullptr;
        }
        if (this->m_curl) {
            curl_easy_cleanup(this->m_curl);
            this->m_curl = nullptr;
        }
        curl_global_cleanup();
        this->m_init = false;
        log_dbg("clear");
    }

    std::vector<nlohmann::json> m_res;
    std::string m_answer;
    std::string m_model;
    bool m_has_stop;
    bool m_init;
    bool m_has_res;

private:
    int still_running;
    CURL* m_curl;
    struct curl_slist* m_slist;
    CURLM* m_multi_handle;
    method_t m_method;
    std::string m_url;
    std::vector<std::string> m_headers;
    std::string m_body;
};

std::string OpenaiAPI::ChatCompletion::Request::json_body()
{
    nlohmann::json json_messages = nlohmann::json::array();

    for (const auto& message : this->messages) {
        nlohmann::json json_obj;
        json_obj["role"] = message.role;
        json_obj["content"] = message.content;
        if (!message.name.empty())
            json_obj["name"] = message.name;
        if (!message.function_call.empty())
            json_obj["function_call"] = message.function_call;

        json_messages.push_back(json_obj);
    }

    nlohmann::json json_dump = { { "stream", this->stream },
        { "model", this->model },
        { "messages", json_messages } };

    std::string jsd;
    try {
        jsd = json_dump.dump(4, ' ', false, nlohmann::json::error_handler_t::ignore);
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "fail to dump js: " << e.what() << std::endl;
    }

    return jsd;
}

int OpenaiAPI::ChatCompletion::create::setup()
{
    if (this->m_ctx.curl_async && this->m_ctx.curl_async->m_init) {
        return 0;
    }
    CurlAsync* curl_async = new CurlAsync;
    if (!curl_async) {
        log_err("fail to new curl async");
        return -1;
    }
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + this->m_ctx.req.api_key,
    };
    std::string js_body = this->m_ctx.req.json_body();
    int ret = curl_async->curl_init(CurlAsync::method_t::POST,
        this->m_ctx.req.api_base,
        headers,
        js_body);
    if (ret) {
        delete curl_async;
        log_err("fail to init curl");
        return -1;
    }

    this->m_ctx.curl_async = curl_async;
    log_dbg("new init done");
    return 0;
}

OpenaiAPI::ChatCompletion::create::iterator::iterator(ctx_t* ctx, bool complate)
    : m_ctx(ctx)
    , m_complete(complate)
{
}

OpenaiAPI::ChatCompletion::create::res_t
OpenaiAPI::ChatCompletion::create::iterator::operator*()
{
    if (!this->m_ctx->curl_async || !this->m_ctx->curl_async->m_init) {
        log_err("iterator*: curl no init");
        return {
            .content = "curl no init"
        };
    }
    return {
        .res = &this->m_ctx->curl_async->m_res,
        .content = this->m_ctx->curl_async->m_answer,
        .model = this->m_ctx->curl_async->m_model,
    };
}

bool OpenaiAPI::ChatCompletion::create::iterator::operator!=(const iterator& other) const
{
    return this->m_complete != other.m_complete;
}

bool OpenaiAPI::ChatCompletion::create::iterator::operator==(const iterator& other) const
{
    return !this->operator!=(other);
}

void OpenaiAPI::ChatCompletion::create::iterator::operator++()
{
    if (!this->m_ctx->curl_async || !this->m_ctx->curl_async->m_init) {
        this->m_complete = true;
        log_err("iterator++: curl no init");
        return;
    }
    this->m_complete = this->m_ctx->curl_async->pull() ? false : true;
}

OpenaiAPI::ChatCompletion::create::create(Request& req)
    : m_ctx({ .req = req, .curl_async = nullptr })
{
}

OpenaiAPI::ChatCompletion::create::create(Request&& req)
    : m_ctx({ .req = req, .curl_async = nullptr })
{
}

OpenaiAPI::ChatCompletion::create::~create()
{
    if (this->m_ctx.curl_async) {
        delete this->m_ctx.curl_async;
        this->m_ctx.curl_async = nullptr;
    }
}

OpenaiAPI::ChatCompletion::create::iterator
OpenaiAPI::ChatCompletion::create::begin()
{
    setup();

    auto iter = iterator(&this->m_ctx, false);
    iter.operator++();

    return iter;
}

OpenaiAPI::ChatCompletion::create::iterator
OpenaiAPI::ChatCompletion::create::end()
{
    return iterator(&this->m_ctx, true);
}