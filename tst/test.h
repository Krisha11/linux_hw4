#pragma once

#include <ranges>

class Test {
public:
    bool showFinalResult();
    
    void runAllTests();

private:
    int failedNum = 0;
    int totalNum = 0;
                                                                
    void check(bool expr); 

    template <typename R1, typename R2, typename F>
    requires (std::ranges::range<R1>, std::ranges::range<R2>)
    bool compare(R1&& a,  R2&& b, F&& cmp) {
        if (a.size() != b.size()) {
            return false;
        }

        for (int i = 0; i < a.size(); i++) {
            if (!cmp(a[i], b[i])) {
                return false;
            }
        }

        return true;
    }

    void testValues();
    void testWithSleep();
    void testExceptions();
};

