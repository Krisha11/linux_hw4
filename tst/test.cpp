#include "common.h"
#include "test.h"
#include "simple.h"
#include "smart.h"

#include <iostream>
#include <string>

void Test::check(bool expr) {
	if (expr) {
		std::cout << "Test passed\n";
	}
	else {
		Test::failedNum++;
		std::cout << "Test failed\n";
	}
	Test::totalNum++;
}

bool Test::showFinalResult() {
	std::cout << "Failed: " << Test::failedNum << "/" << Test::totalNum << "\n";
	if (Test::failedNum == 0) {
		std::cout << "OK\n";
        return true;
    }

    return false;
}

void Test::runAllTests() {
    testValues();
    testWithSleep();
    testExceptions();
}

void Test::testValues() {
    auto const ints = std::vector<int>{ 0, 1, 2, 3, 4, 5 };

    auto f = [](int x) {
        return x + 1;
    };

    auto cmp = [](int x, int y) {
        return x == y;
    };

    std::vector<int> out(ints.size());
    std::transform(ints.begin(), ints.end(), out.begin(), f);

    for (auto n : {1, 3, 8}) {
        check(compare(TransformWithProcesses(ints, f, n), out, cmp));

        for (auto M : {1, 2, 3}) {
            check(compare(TransformWithProcessesSmart(ints, f, n, M), out, cmp));
        }
    }
}

void Test::testWithSleep() {
    auto const ints = std::vector<int>{ 0, 1, 2, 3, 4, 5 };

    auto f = [](int x) {
        if (x == 1 || x == 5) {
            sleep(5);
        }
        return x + 1;
    };

    auto cmp = [](int x, int y) {
        return x == y;
    };

    std::vector<int> out(ints.size());
    std::transform(ints.begin(), ints.end(), out.begin(), f);

    uint32_t start = clock();
    check(compare(TransformWithProcesses(ints, f, 3), out, cmp));
    uint32_t end = clock();
    check(((float)(end - start))/CLOCKS_PER_SEC < 7);

    start = clock();
    check(compare(TransformWithProcessesSmart(ints, f, 2, 2), out, cmp));
    end = clock();
    check(((float)(end - start))/CLOCKS_PER_SEC < 7);
}

void Test::testExceptions() {
    auto const ints = std::vector<int>{ 0, 1, 2, 3, 4, 5 };

    auto f = [](int x) {
        if (x == 1 || x == 5) {
            throw "error";
        }
        return x + 1;
    };

    try {
        TransformWithProcesses(ints, f, 3);
    } catch (std::exception &e) {
        check(std::string(e.what()) == "Errors during exectuion");
    }

    try {
        TransformWithProcessesSmart(ints, f, 2, 2);
    } catch(std::exception &e) {
        check(std::string(e.what()) == "Errors during exectuion");
    }
}
