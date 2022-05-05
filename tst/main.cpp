#include "test.h"

int main() {
    Test t;
    t.runAllTests();
    bool res = t.showFinalResult();
    if (!res) {
        return 1;
    }
    return 0;
}