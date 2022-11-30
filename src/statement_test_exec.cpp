#include "statement.h"
#include "test_runner_p.h"

#include <iostream>

using namespace std;

namespace ast {
void RunUnitTests(TestRunner& tr);
}


int main() {

    try {
        TestRunner tr;
        ast::RunUnitTests(tr);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
