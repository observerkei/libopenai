#include "libopenai_api.h"
#include <iostream>
#include <string>

#define log_dbg(fmt, ...) fprintf(stdout, "[%s:%s:%ld]  " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define input(prompt) ({ \
    fprintf(stdout, prompt); \
    std::string _input; \
    std::getline(std::cin, _input, '\n'); \
    _input; })

int main(int argc, char* argv[])
{
    OpenaiAPI::ChatCompletion::Request::messages_t messages = {
        { .role = "system", .content = "You are a helpful assistant." }
    };

    bool _exit = false;
    std::string answer;
    std::string prompt = "are you ok ?";

    log_dbg("q: %s", prompt.c_str());
    do {
        messages.push_back({ .role = "user", .content = prompt });

        for (const auto& res : OpenaiAPI::ChatCompletion::create(
                 { .api_key = "",
                   .model = "gpt-3.5-turbo",
                   .messages = messages })) {
            log_dbg("a: %s", res.content.c_str());
            answer = res.content;
        }
        messages.push_back({ .role = "assistant", .content = answer });

        log_dbg("q: %s\na: %s", prompt.c_str(), answer.c_str());

        prompt = input("q: ");
        if ("exit" == prompt)
            _exit = true;

    } while (!_exit);

    return 0;
}
