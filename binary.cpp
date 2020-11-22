#define PACKAGE         // a hack to enable usage of libbfd
#define PACKAGE_VERSION // ^
#include <bfd.h>

#include "binary.h"
#include "exceptions.h"
#include "loader.h"
#include "section.h"
#include "symbol.h"

static bfd* open_bfd(const char* fname);

[[nodiscard]]
bool Binary::init(const std::string& fname, Binary::Type type, bool mayThrow)
{
    if (m_is_valid) {
        return true;
    }

    m_filename = fname;
    m_type = type;

    bfd* bfd_h = open_bfd(fname.c_str());
    if (bfd_h == NULL) {
        if (mayThrow) {
            throw FailedOpen("failed to open binary");
        }
        return false;
    }

    m_entry = bfd_get_start_address(bfd_h);

    m_type_str = std::string(bfd_h->xvec->name);
    switch (bfd_h->xvec->flavour) {
        case bfd_target_elf_flavour:
            m_type = Binary::Type::Elf;
            break;
        case bfd_target_coff_flavour:
            m_type = Binary::Type::Pe;
            break;
        default:
            bfd_close(bfd_h);
            if (mayThrow) {
                throw BadFormat("unsupported binary type");
            }
            return false;
    }

    const bfd_arch_info_type* bfd_info = bfd_get_arch_info(bfd_h);
    m_arch_str = std::string(bfd_info->printable_name);
    switch (bfd_info->mach) {
        case bfd_mach_i386_i386:
            m_arch = Binary::Arch::X86;
            m_bits = 32;
            break;
        case bfd_mach_x86_64:
            m_arch = Binary::Arch::X86;
            m_bits = 64;
            break;
        default:
            bfd_close(bfd_h);
            if (mayThrow) {
                throw BadFormat("unsupported architecture");
            }
            return false;
    }

    /* Symbol handling is best-effort only (they may not even be present) */
    load_symbols_bfd(bfd_h);
    load_dynsym_bfd(bfd_h);

    load_sections_bfd(bfd_h);

    m_is_valid = true;
    return true;
}

Binary::Binary(const std::string& fname, Binary::Type type)
{
    m_is_valid = init(fname, type, true);
}

static bfd* open_bfd(const char* fname)
{
    static int bfd_inited = 0;
    if (!bfd_inited) {
        bfd_init();
        bfd_inited = 1;
    }

    bfd* bfd_h = bfd_openr(fname, NULL);
    if (!bfd_h) {
        throw FailedOpen("failed to open binary");
    }

    if (!bfd_check_format(bfd_h, bfd_object)) {
        throw BadFormat("file '" + std::string(fname) + "' does not look like an executable");
    }

    /* Some versions of bfd_check_format pessimistically set a wrong_format
     * error before detecting the format and then neglect to unset it once
     * the format has been detected. We unset it manually to prevent problems.
     */
    bfd_set_error(bfd_error_no_error);

    if (bfd_get_flavour(bfd_h) == bfd_target_unknown_flavour) {
        throw BadFormat("unrecognized format for binary '" + std::string(fname) + "'");
    }

    return bfd_h;
}

void Binary::load_symbols_bfd(bfd* bfd_h)
{
    asymbol** bfd_symtab = NULL;
    long n = bfd_get_symtab_upper_bound(bfd_h);
    if (n < 0) {
        throw ReadFail("failed to read symtab");
    } else if (n > 0) {
        bfd_symtab = (asymbol**)malloc(n);
        if (bfd_symtab == NULL) {
            throw AllocFail("out of memory");
        }

        long nsyms = bfd_canonicalize_symtab(bfd_h, bfd_symtab);
        if (nsyms < 0) {
            throw ReadFail("failed to read symtab");
        }

        for (int i=0; i < nsyms; i++) {
            if (bfd_symtab[i]->flags & BSF_FUNCTION) {
                m_symbols.emplace_back(
                    std::string(bfd_symtab[i]->name),
                    bfd_asymbol_value(bfd_symtab[i]),
                    Symbol::SymbolType::Function);
            }
        }
    }

    if (bfd_symtab) {
        free(bfd_symtab);
    }
}

void Binary::load_dynsym_bfd(bfd* bfd_h)
{
    asymbol** bfd_dynsym;
    long n = bfd_get_dynamic_symtab_upper_bound(bfd_h);
    if (n < 0) {
        throw ReadFail("failed to read dynamic symtab");
    } else if (n > 0) {
        bfd_dynsym = (asymbol**)malloc(n);
        if (bfd_dynsym == NULL) {
            throw AllocFail("out of memory");
        }

        long nsyms = bfd_canonicalize_dynamic_symtab(bfd_h, bfd_dynsym);
        if (nsyms < 0) {
            throw ReadFail("failed to read dynamic symtab");
        }

        for (int i=0; i < nsyms; i++) {
            if (bfd_dynsym[i]->flags & BSF_FUNCTION) {
                m_symbols.emplace_back(
                    std::string(bfd_dynsym[i]->name),
                    bfd_asymbol_value(bfd_dynsym[i]),
                    Symbol::SymbolType::Function);
            }
        }
    }

    if (bfd_dynsym) {
        free(bfd_dynsym);
    }
}

void Binary::load_sections_bfd(bfd* bfd_h)
{
    for (asection* bfd_sec = bfd_h->sections; bfd_sec != NULL; bfd_sec = bfd_sec->next) {
        Section::SectionType sectype = Section::SectionType::None;
        int bfd_flags = bfd_section_flags(bfd_sec);
        if (bfd_flags & SEC_CODE) {
            sectype = Section::SectionType::Code;
        } else if (bfd_flags & SEC_DATA) {
            sectype = Section::SectionType::Data;
        } else {
            continue;
        }

        const char* secname = bfd_section_name(bfd_sec);
        if (secname == NULL) {
            secname = "<unnamed>";
        }

        uint64_t size = bfd_section_size(bfd_sec);
        std::vector<uint8_t> bytes(size);

        if (bfd_get_section_contents(bfd_h, bfd_sec, bytes.data(), 0, size) == 0) {
            throw ReadFail("failed to read section '" + std::string(secname) + "'");
        }

        uint64_t vma = bfd_section_vma(bfd_sec);
        m_sections.emplace_back(
            this,
            sectype,
            std::string(secname),
            vma,
            bytes);
    }
}

