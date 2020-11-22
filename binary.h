#ifndef BINARY_H
#define BINARY_H

#include <optional>
#include <string>
#include <vector>

#include "section.h"
#include "symbol.h"

struct bfd;

class Binary final {
public:
    enum class Type {
        Auto,
        Elf,
        Pe,
    };

    enum class Arch {
        None,
        X86,
    };

    Binary() = default;
    Binary(const std::string& fname, Binary::Type type);

    [[nodiscard]]
    bool                        init(const std::string& fname, Binary::Type type=Type::Auto, bool mayThrow=true);

    const std::string&          filename() const { return m_filename; }
    Type                        type()     const { return m_type;     }
    const std::string&          type_str() const { return m_type_str; }
    Arch                        arch()     const { return m_arch;     }
    const std::string&          arch_str() const { return m_arch_str; }
    unsigned                    bits()     const { return m_bits;     } // is 32bit or 64bit
    uint64_t                    entry()    const { return m_entry;    }
    const std::vector<Section>& sections() const { return m_sections; }
    const std::vector<Symbol>&  symbols()  const { return m_symbols;  }
    bool                        is_valid() const { return m_is_valid; }

    std::optional<Section> get_text_section() const
    {
        for (const auto& section : sections()) {
            if (section.name() == ".text") {
                return section;
            }
        }
        return {};
    }

private:
    std::string          m_filename{};
    Type                 m_type    {};
    std::string          m_type_str{};
    Arch                 m_arch    {};
    std::string          m_arch_str{};
    unsigned             m_bits    {}; // is 32bit or 64bit
    uint64_t             m_entry   {};
    std::vector<Section> m_sections{};
    std::vector<Symbol>  m_symbols {};
    bool                 m_is_valid{};

    void load_symbols_bfd(bfd* bfd_h);
    void load_dynsym_bfd(bfd* bfd_h);
    void load_sections_bfd(bfd* bfd_h);
};

#endif
