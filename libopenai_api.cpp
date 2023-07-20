#include "libopenai_api.h"

#include <vector>
#include <string>
#include <iostream>

class OpenaiAPI {
    class ChatCompletion {
        public:
            struct Request {
                std::string api_base;
            };
            class create {
                typedef struct {
                    std::string res;
                    Request req;
                } ctx_t;

                public:
                    create(Request &req) : m_ctx({.req=req}){} 

                    class iterator {
                        public:
                            int chat_complatetion_create(Request &req)
                            {
                                if (&req) {
                                    return 1;
                                } else {
                                    return 0;
                                }    
                            }
                        public:
                            iterator(ctx_t *ctx, bool complate) : m_ctx(ctx), m_complate(complate) {}
                            
                            std::string &operator*()
                            {
                                return this->m_ctx->res;
                            }

                            bool isEnd() const
                            {
                                return this->m_complate;
                            }

                            void next()
                            {
                                this->m_complate = chat_complatetion_create(this->m_ctx->req) ? false : true;
                            }

                        private:
                            ctx_t *m_ctx;
                            bool m_complate;
                    };

                    
                    iterator begin() {
                        return iterator(&this->m_ctx, true);
                    }
                    
                    iterator end() {
                        return iterator(&this->m_ctx, false);
                    }
                private:
                    ctx_t m_ctx;
            };
        private:
            bool m_complate;
    };
};


int main(int argc, char *argv[])
{
    for (const auto &answer : OpenaiAPI::ChatCompletion::create({.api_base = "test"})) {
        std::cout << answer << std::endl;
    }
    return 0;
}
