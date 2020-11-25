#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  exit(1);
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  JobsList::JobEntry* job = SmallShell::getInstance().getCurrentJob();
  if (job != nullptr) {
    pid_t pid = job->getPID();
    job->killJob(SIGKILL, false);
    cout << "smash: process " << pid << " was killed" << endl;
    SmallShell::getInstance().setCurrentJob(nullptr);
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

