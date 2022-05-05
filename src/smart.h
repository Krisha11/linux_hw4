#pragma once

#include "common.h"

#include <ranges>
#include <span>

template <typename R, typename T, typename F>
requires (std::ranges::range<R>,  std::invocable<F, std::ranges::range_value_t<R>>, std::copyable<std::invoke_result<F, std::ranges::range_value_t<R>>>)
auto TransformWithProcessSmart(R&& range, F&& f, T&& out, int from, int cnt, pthread_mutex_t* mutex, int* controller) {
    while (true) {
        for (int i = from; i < std::min((size_t)(from + cnt), range.size()); i++) {
            out[i] = f(range.data()[i]);
        }

        pthread_mutex_lock(mutex);
        if (*controller < range.size()) {
            from = *controller;
            *controller += cnt;
        } else {
            pthread_mutex_unlock(mutex);
            return;
        }


        pthread_mutex_unlock(mutex);
    }
}

template <typename R, typename F>
requires (std::ranges::range<R>,  std::invocable<F, std::ranges::range_value_t<R>>, std::copyable<std::invoke_result<F, std::ranges::range_value_t<R>>>)
auto TransformWithProcessesSmart(R&& range, F&& f, int nprocesses, int M) {
    if (range.size() <= nprocesses * M) {
        return TransformWithProcesses(range, f, nprocesses);
    }

    void* res = mmap(NULL, sizeof(decltype(f(*range.begin()))) * range.size(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    std::span out{
        (decltype(f(*range.begin()))*)res,
        range.size()
    };

    res = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    int* controller = (int*)res;
    (*controller) = nprocesses * M;

    pthread_mutexattr_t attrmutex;
    pthread_mutexattr_init(&attrmutex);
    pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);   
    res = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    pthread_mutex_t* pmutex = (pthread_mutex_t *)res;
    pthread_mutex_init(pmutex, &attrmutex);

    res = mmap(NULL, sizeof(bool), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (res == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    bool* hasErrors = (bool*)res;
    *hasErrors = false;

    std::vector<pid_t> processes;
    for (int i = 0; i < nprocesses; i++) {
        auto pid = fork();

        if (pid == -1) {
            killProcesses(processes, i);
            *hasErrors = true;
            break;
        }

        if (pid == 0) {
            try {
                TransformWithProcessSmart(range, f, out, i * M, M, pmutex, controller);
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

    pthread_mutex_destroy(pmutex);
    pthread_mutexattr_destroy(&attrmutex); 

    if (*hasErrors) {
        throw std::runtime_error("Errors during exectuion");
    } else if (terminateErrors) {
        throw std::runtime_error("Errors during terminating");
    }

    return out;
}
