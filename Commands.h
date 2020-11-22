#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

using namespace std;

class Command {
// TODO: Add your data members
 protected:
  string cmd_line;
 public:
  Command(const char* cmd_line);
  virtual ~Command() {};
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
  virtual void execute() = 0;
};

class ExternalCommand : public Command {
 private:
  vector<string> args;
  bool bg_run;
 public:
  ExternalCommand(const char* cmd_line, vector<string> ext_args, bool bg_run);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
 private:
  string target_dir;
 public:
  ChangeDirCommand(const char* cmd_line, string target_dir);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};
//HERE
class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 private:
  int pid;
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
  private:
    string new_prompt;
  public:
    ChpromptCommand(const char* cmd_line, string new_prompt);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class CommandsHistory {
 protected:
  class CommandHistoryEntry {
	  // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  CommandsHistory();
  ~CommandsHistory() {}
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() {}
  void execute() override;
};

class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
        string command;
        pid_t process_id;
        time_t insertion_time;
        bool is_stopped;
    public:
        JobEntry(const string &command,int process_id,bool is_stopped);
        JobEntry()=default;
        // prints job info in the format of
        //<command> : <process id> <seconds-elapsed> (if stopped will add (stopped)
        void PrintJob();

        //sending a signal to the Job, if specified will print a note
        //signal number <signal> was sent to pid <process id>
        void killJob(int signal, bool print=false);

        //returns pid
        pid_t getPID() const;

        //return command
        string getCommand() const;

        //if set as stopped if stopping is true
        void stoppedOrResumed(bool stopping);
    };
private:
    std::map<int,JobEntry> job_list;
    map<pid_t,int> pid_to_index;
    // TODO: Add your data members
public:
    JobsList()=default;
    ~JobsList()=default;

    //adds new  job into the list
    void addJob(string cmd, int job_pid, bool isStopped = false);

    //prints the jobs
    void printJobsList();

    //killing all jobs. prints in format:
    //<job_pid>: <command>
    void killAllJobs();

    //removing from list finished jobs.
    void removeFinishedJobs();

    JobEntry * getJobById(int jobId);

    //remove a Job, does not kill.
    void removeJobById(int jobId);

    int pidToIndex(pid_t pid);
    void jobStoppedOrResumed(int jobId,bool isStopped);
    // TODO: Add extra methods or modify exisitng ones as needed

    void sendSignal(int jobID, int sig_num);
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 private:
  int jobID;
  int sig_num;
 public:
  KillCommand(const char* cmd_line, int jobID, int sig_num);
  virtual ~KillCommand() {}
  void execute() override;

  static bool validLine(vector<string> line);
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

// TODO: add more classes if needed 
// maybe ls, timeout ?

class SmallShell {
 private:
  // TODO: Add your data members
  string prev_dir;
  string prompt;
  JobsList job_list;

  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
  string getPrompt();
  void setPrompt(string new_prompt);

  string getPrevDir();
  void setPrevDir(string new_dir);

  JobsList* getJobList();
};

#endif //SMASH_COMMAND_H_
