#pragma once

#include "common.h"

#include <ranges>
#include <span>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>

template <typename R, typename T, typename F>
requires (std::ranges::range<R>,  std::invocable<F, std::ranges::range_value_t<R>>, std::copyable<std::invoke_result<F, std::ranges::range_value_t<R>>>)
auto TransformWithProcess(R&& range, F&& f, T&& out) {
    for (int i = 0; i < range.size(); i++) {
        out[i] = f(range[i]);
    }
}

template <typename R, typename F>
requires (std::ranges::range<R>,  std::invocable<F, std::ranges::range_value_t<R>>, std::copyable<std::invoke_result<F, std::ranges::range_value_t<R>>>)
auto TransformWithProcesses(R&& range, F&& f, int nprocesses) {
    void* res = mmap(NULL, sizeof(decltype(f(*range.begin()))) * range.size(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    std::span out{
        (decltype(f(*range.begin()))*)res,
        range.size()
    };

    if (range.size() == 0) {
        return out;
    }
    int rangeSize = std::max((size_t)1, range.size() / nprocesses);

    res = mmap(NULL, sizeof(bool), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
        return out;
    }
    bool* hasErrors = (bool*)res;
    *hasErrors = false;

    std::vector<pid_t> processes;
    for (int i = 0; i < range.size(); i += rangeSize) {
        auto pid = fork();

        if (pid == -1) {
            killProcesses(processes, i);
            *hasErrors = true;
            break;
        }

        if (pid == 0) {
            try {
                TransformWithProcess(std::ranges::subrange(range.begin() + i, range.begin() + i + rangeSize), f, std::ranges::subrange(out.begin() + i, out.begin() + i + rangeSize));
            } catch (...) {
                *hasErrors = true;
            }
            exit(0);
        } else {
            processes.push_back(pid);
        }
    }

    bool terminateErrors = false;
    for (int i = 0; i < processes.size(); i++) {
        if (waitpid(processes[i], NULL, 0) == -1) {
            terminateErrors = true;
        }
    }

    if (*hasErrors) {
        throw std::runtime_error("Errors during exectuion");
    } else if (terminateErrors) {
        throw std::runtime_error("Errors during terminating");
    }

    return out;
}
