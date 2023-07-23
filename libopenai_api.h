#ifndef __LIBOPENAI_API_H__
#define __LIBOPENAI_API_H__

#ifdef __cplusplus

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class CurlAsync;

class OpenaiAPI {
public:
    class ChatCompletion {
    public:
        struct Request {
        public:
            std::string json_body();

            std::string api_base = "https://api.openai.com/v1/chat/completions";
            std::string api_key;

            std::string model;

            struct message_t {
                std::string role;
                std::string content;
                std::string name;
                std::string function_call;
            };
            typedef std::vector<message_t> messages_t;
            messages_t messages;

            struct function_t {
                std::string name;
                std::string description;
                nlohmann::json parameters;
            };
            typedef std::vector<function_t> functions_t;
            functions_t functions;

            nlohmann::json function_call;

            float temperature = 1;
            float top_p = 1;
            int n = 1;
            bool stream = true;
            std::vector<std::string> stop;
            int max_tokens;
            float presence_penalty = 0;
            float frequency_penalty = 0;
            std::string logit_bias;
            std::string user;
        };

        struct Response {
            nlohmann::json data; // response
            std::string content; // response stream content

            std::string id;
            std::string object;
            long created;
            std::string model;

            struct choice_t {
                struct delta_t {
                    std::string content;
                };
                int index;
                delta_t delta;
                std::string finish_reason;
            };
            std::vector<choice_t> choices;
        };

        class create {
        public:
            struct ctx_t {
                Request req;
                CurlAsync* curl_async;
            };

            struct res_t {
                std::vector<nlohmann::json>* response;
                std::string content;
                std::string model;
            };

        public:
            create(Request& req);

            create(Request&& req);

            ~create();

            int setup();

            class iterator {
            public:
                iterator(ctx_t* ctx, bool complate);

                res_t operator*();

                bool operator!=(const iterator& other) const;

                bool operator==(const iterator& other) const;

                void operator++();

            private:
                ctx_t* m_ctx;
                bool m_complete;
            };

            iterator begin();
            iterator end();

        private:
            ctx_t m_ctx;
        };
    };
};

#endif //__cplusplus

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__LIBOPENAI_API_H__
