#ifndef SECTION_H
#define SECTION_H

#include <string>
#include <vector>

class Binary;

class Section final {
public:
    enum class SectionType {
        None,
        Code,
        Data,
    };

    Section(const Section& s)
    : m_binary(s.m_binary)
    , m_type(s.m_type)
    , m_name(s.m_name)
    , m_vma(s.m_vma)
    , m_bytes(s.m_bytes)
    {
    }

    Section& operator=(const Section& s)
    {
        if (this != &s) {
            m_binary = s.m_binary;
            m_type = s.m_type;
            m_name = s.m_name;
            m_vma = s.m_vma;
            m_bytes = s.m_bytes;
        }
        return *this;
    }

    Section(Binary* bin, SectionType type, const std::string& name,
        uint64_t vma, const std::vector<uint8_t>& bytes)
    : m_binary(bin)
    , m_type(type)
    , m_name(name)
    , m_vma(vma)
    , m_bytes(bytes)
    {}

    const Binary& binary()       const { return *m_binary; } // probably never nullptr
    SectionType type()           const { return  m_type;   }
    const std::string& name()    const { return  m_name;   }
    uint64_t vma()               const { return  m_vma;    }
    std::vector<uint8_t> bytes() const { return  m_bytes;  }
    size_t size()                const { return  m_bytes.size();  }

    bool contains(uint64_t addr) const
    {
        return (vma() <= addr) && (addr < vma() + size());
    }

private:
    Binary*              m_binary{};
    SectionType          m_type  {SectionType::None};
    std::string          m_name  {};
    uint64_t             m_vma   {}; // starting address
    std::vector<uint8_t> m_bytes {};
};

#endif
