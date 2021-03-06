///
/// Copyright (C) 2014, Cyberhaven
/// All rights reserved.
///
/// Licensed under the Cyberhaven Research License Agreement.
///

#include <iomanip>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <vmi/ExecutableFile.h>

using namespace llvm;

namespace {
cl::opt<std::string> File("file", cl::desc("File to dump"), cl::Positional);

cl::opt<std::string> Address("address", cl::desc("Address where to dump"), cl::Required);

cl::opt<std::string> Count("count", cl::desc("How many bytes to dump"), cl::Required);
}

using namespace vmi;

void hexDump(std::ostream &os, uint64_t address, const uint8_t *buffer, size_t size) {
    unsigned i;
    char buff[17] = {0};

    // Process every byte in the data.
    for (i = 0; i < size; i++) {
        // Multiple of 16 means new line (with line offset).
        auto data = buffer[i];

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0) {
                os << "  " << buff << "\n";
            }
            // Output the offset.
            os << std::hex << "0x" << (address + i) << std::dec;
        }

        // Now the hex code for the specific character.
        os << " " << std::hex << std::setfill('0') << std::right << std::setw(2) << (int) data << std::dec;

        // And store a printable ASCII character for later.
        if ((data < 0x20) || (data > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = data;
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        os << "   ";
        i++;
    }

    // And print the final ASCII bit.
    os << "  " << buff << "\n";
}

int main(int argc, char **argv) {
    cl::ParseCommandLineOptions(argc, (char **) argv, " reader");

    auto fp = FileSystemFileProvider::get(File, false);
    if (!fp) {
        llvm::errs() << "Can't open " << File << "\n";
        return -1;
    }

    auto binary = vmi::ExecutableFile::get(fp, false, 0);
    if (!binary) {
        llvm::errs() << "Can't parse " << File << "\n";
        return -1;
    }

    uint64_t address = strtoul(Address.c_str(), NULL, 0);
    ssize_t count = strtoul(Count.c_str(), NULL, 0);

    std::unique_ptr<uint8_t[]> buffer(new uint8_t[count]);

    ssize_t read_count = binary->read(buffer.get(), count, address);
    if (read_count != count) {
        printf("Could only read %ld bytes from %lx\n", read_count, address);
    }

    hexDump(std::cout, address, buffer.get(), read_count);

    return 0;
}
