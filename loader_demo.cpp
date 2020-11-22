#include <algorithm>

#include "loader.h"

/* TODO:
 * 1) Override weak symbols (bfd_{symtab,dynsym}[i]->flags & BSF_WEAK)
 * 2) Add support for {local,global,function} symbols
 */

void xdump(const uint8_t *bytes, size_t length, size_t width=16)
{
    for (size_t i=0; i < length; i++) {
        printf("%02x ", bytes[i]); // Display byte in hex
        if ((i%width)==(width-1) || i==length-1) {
            for (size_t j=0; j < (width-1)-(i%width); j++)
                printf("   ");
            printf("| ");
            for (size_t j=(i-(i%width)); j <= i; j++) { // Display printable bytes from line.
                unsigned char byte = bytes[j];
                if ((byte > 31) && (byte < 127)) // Inside printable char range
                    printf("%c", byte);
                else
                    printf(".");
            }
            puts(""); // End of the dump line (each line is 16 bytes)
        } // End if
    } // End for
}

void dumpSectionBytes(const Binary& bin, const char *sctnName)
{
    auto foundSection = std::find_if(bin.sections().cbegin(), bin.sections().cend(),
        [sctnName](const Section& s){ return s.name() == sctnName; });

    if (foundSection != bin.sections().cend()) {
        xdump(foundSection->bytes().data(), foundSection->size());
    }
    else {
        printf("no matching section '%s'\n", sctnName);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <binary> [<section_name>]\n", argv[0]);
        return 1;
    }

    Binary bin;
    if (bin.init(argv[1], Binary::Type::Auto, false) == false) {
        fprintf(stderr, "Failed to load binary\n");
        return 1;
    }

    printf("loaded binary '%s' %s/%s (%u bits) entry @0x%016lx\n",
        bin.filename().c_str(),
        bin.type_str().c_str(), bin.arch_str().c_str(),
        bin.bits(), bin.entry());

    if (argc >= 3) {
        dumpSectionBytes(bin, argv[2]);
        return 0;
    }

    for (const auto& sec : bin.sections()) {
        printf("  0x%016lx %-8ju %-20s %s\n",
            sec.vma(), sec.size(), sec.name().c_str(),
            (sec.type() == Section::SectionType::Code) ? "CODE" : "DATA");
    }

    if (bin.symbols().size() > 0) {
        printf("scanned symbol tables\n");
        for (const auto& sym : bin.symbols()) {
            printf("  %-40s 0x%016lx %s\n",
                sym.name().c_str(), sym.addr(),
                (sym.type() == Symbol::SymbolType::Function) ? "FUNC" : "");
        }
    }

    return 0;
}
