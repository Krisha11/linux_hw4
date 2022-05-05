#include "common.h"

#include <sys/wait.h>

void killProcesses(std::vector<pid_t> &processes, int n) {
    for (int i = 0; i < processes.size(); i++) {
        kill(processes[i], SIGINT);
    }
}

