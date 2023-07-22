#include "libopenai_api.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    OpenaiAPI::ChatCompletion::Request::messages_t messages = {
        { .role = "system", .content = "You are a helpful assistant." }
    };

    bool _exit = false;
    std::string answer;
    std::string prompt = "are you ok ?";
    do {
        messages.push_back({ .role = "user", .content = prompt });

        for (const auto& res : OpenaiAPI::ChatCompletion::create(
                 { .api_key = "",
                   .model = "gpt-3.5-turbo",
                   .messages = messages })) {
            std::cout << "a: " << res.content << std::endl;
            answer = res.content;
        }
        messages.push_back({ .role = "assistant", .content = answer });

        std::cout << "q: " << prompt << std::endl;
        std::cout << "a: " << answer << std::endl;

        std::cout << "q: ";
        std::cin >> prompt;
        if ("exit" == prompt)
            _exit = true;

    } while (!_exit);

    return 0;
}
