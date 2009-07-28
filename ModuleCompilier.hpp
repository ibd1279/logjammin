#pragma once

#include <string>

namespace llamativo {
    class ModuleCompilier {
    public:
        ModuleCompilier(std::istream &is);
        virtual ~ModuleCompilier();
        std::string& script();
    private:
        std::string _script;
    };
}; // namespace llamativo
