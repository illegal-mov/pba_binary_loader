#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>

class Symbol final {
public:
    enum class SymbolType {
        Function,
        Unknown,
    };

    Symbol(const Symbol& s)
    {
        m_name = s.m_name;
        m_addr = s.m_addr;
        m_type = s.m_type;
    }

    Symbol(const std::string& name, uint64_t addr, SymbolType type)
    : m_name(name)
    , m_addr(addr)
    , m_type(type)
    {}

    const std::string& name() const { return m_name; }
    uint64_t addr()           const { return m_addr; }
    SymbolType type()         const { return m_type; }

private:
    std::string m_name{};
    uint64_t    m_addr{};
    SymbolType  m_type{SymbolType::Unknown};
};

#endif
