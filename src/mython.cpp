#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace std;

namespace {

void RunMythonProgram(istream& input, ostream& output) {
    parse::Lexer lexer(input);

    auto program = ParseProgram(lexer);

    runtime::SimpleContext context{output};
    runtime::Closure closure;
    program->Execute(closure, context);
}

}

int main(int argc, const char** argv) {
    if (argc != 3) {
            cerr << "Mython interpreter!"sv << endl;
            std::filesystem::path interpreter = argv[0];
            cerr << "Usage: "sv << interpreter.filename() << " <in_file> <out_file>"sv << endl;
            return 1;
    }

    std::filesystem::path in_path = argv[1];
    std::filesystem::path out_path = argv[2];

    ifstream ifile(in_path);
    if (!ifile.is_open()) {
        std::cerr << "Can't open file "s << in_path << endl;
    }
    ofstream ofile(out_path);
    if (!ofile.is_open()) {
        std::cerr << "Can't open file "s << out_path << endl;
    }

    try {
        RunMythonProgram(ifile, ofile);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
