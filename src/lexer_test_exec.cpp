#include "lexer.h"
#include "test_runner_p.h"

#include <iostream>

#include <sstream>
#include <string>

namespace parse {
void RunOpenLexerTests(TestRunner& tr);
}

using namespace std;

int main() {

    try {
        TestRunner tr;
        parse::RunOpenLexerTests(tr);
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }

}