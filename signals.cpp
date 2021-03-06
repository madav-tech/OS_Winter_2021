#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
    JobsList::JobEntry* job = SmallShell::getInstance().getCurrentJob();
    if (job != nullptr) {
        pid_t pid = job->getPID();
        job->killJob(SIGSTOP, false);
        cout << "smash: process " << pid << " was stopped" << endl;
        SmallShell::getInstance().setCurrentJob(nullptr);
    }
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
  cout << "smash: got an alarm" << endl;
  SmallShell &smash = SmallShell::getInstance();
  smash.getTimeoutList()->doomEntry();
}

