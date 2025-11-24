#pragma once
#include "se_pch.h"

namespace se {
struct ApplicationCommandLineArgs {
    int Count = 0;
    char** Args = nullptr;

    const char* operator[](int index) const {
        return Args[index];
    }
};

struct ApplicationSpec {
    std::string Name = "Simple-Engine Application";
    uint32_t WindowWidth = 1280;
    uint32_t WindowHeight = 720;
    ApplicationCommandLineArgs CommandLineArgs;
};
} // namespace se
