#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

vector<string> _parseLine(const char* cmd_line)
{
  char** args = new char*[20];
  for (int i = 0; i < 20; i++)
  {
    args[i] = new char[30];
  }
  _parseCommandLine(cmd_line, args);
  vector<string> cmd_vec = vector<string>(20);
  for (int i = 0; i < 20; i++)
  {
    if(args[i] == NULL)
      break;
    cmd_vec[i] = args[i];
  }
  return cmd_vec;
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
// TODO: add your implementation
  this->prompt = "smash> ";
  this->prev_dir = "\0";
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
  vector<string> split_line = _parseLine(cmd_line);
  string command_name = split_line[0];
  if(command_name == "chprompt") {
    return new ChpromptCommand(cmd_line, split_line[1]);
  }
  else if (command_name == "showpid") {
    return new ShowPidCommand(cmd_line);
  }
  else if (command_name == "cd") {
    if (split_line[2] != ""){
      printf("smash error: cd: too many arguments\n");
    }
    else {
      return new ChangeDirCommand(cmd_line, split_line[1]);
    }
  }
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  */
  else {
    bool bg_run = false;
    //Checking for '&' symbol
    for (vector<string>::iterator it = split_line.begin(); it != split_line.end(); it++){
      if (*it == "&"){
        bg_run = true;
        break;
      }
    }
    //Filtering arguments for external command
    vector<string> ext_args = vector<string>(20);
    int i = 0;
    for (vector<string>::iterator it = split_line.begin(); it != split_line.end(); it++){
      if (it != split_line.begin() && *it != "&"){
        ext_args[i] = *it;
        i++;
      }
    }
    return new ExternalCommand(cmd_line, ext_args, bg_run);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  Command* cmd = CreateCommand(cmd_line);
  if (cmd != nullptr) {
    cmd->execute();
  }
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

//________SmallShell_________


string SmallShell::getPrompt() {
  return this->prompt;
}

void SmallShell::setPrompt(string new_prompt) {
  this->prompt = new_prompt;
}

string SmallShell::getPrevDir(){
  return this->prev_dir;
}

void SmallShell::setPrevDir(string new_dir){
  this->prev_dir = new_dir;
}

//_________chprompt_________

ChpromptCommand::ChpromptCommand(const char* cmd_line, string new_prompt) : BuiltInCommand(cmd_line), new_prompt(new_prompt){ 
  if (new_prompt == "") {
    this->new_prompt = "smash";
  }
}

void ChpromptCommand::execute() {
  SmallShell::getInstance().setPrompt(this->new_prompt + "> ");
}

//_________showpid_________

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  this->pid = getpid();
}

void ShowPidCommand::execute() {
  printf("smash pid is %d\n", this->pid);
}

//_________cd_________

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, string target_dir) : BuiltInCommand(cmd_line) {
  this->target_dir = target_dir;
}

void ChangeDirCommand::execute(){
  if (this->target_dir == "-") {
    if (SmallShell::getInstance().getPrevDir() == "\0") {
      printf("smash error: cd: OLDPWD not set\n");
    }
    else {
      string prev_temp = get_current_dir_name();
      if (chdir(SmallShell::getInstance().getPrevDir().c_str()) == 0){
        SmallShell::getInstance().setPrevDir(prev_temp);
        return;
      }
      else {
        perror("smash error: chdir failed");
      }
    }
  }
  else {
    string prev_temp = get_current_dir_name();
    if (chdir(this->target_dir.c_str()) == 0) {
      SmallShell::getInstance().setPrevDir(prev_temp);
    }
    else {
      perror("smash error: chdir failed");
    }
  }
}


//_________ExternalCommand_________

ExternalCommand::ExternalCommand(const char* cmd_line, vector<string> ext_args, bool bg_run) : Command(cmd_line), args(ext_args), bg_run(bg_run) {
  
}

void ExternalCommand::execute() {
  //FORK AND THEN PASS ARGUMANTS TO BASH THROUGH EXECV()
  int p = fork();

  if(p < 0){
    perror("smash error: fork failed");
    return;
  }
  else if (p > 0){
    if(!this->bg_run){
      wait(NULL);
    }
  }
  else {
    string command = "";
    for (vector<string>::iterator it = this->args.begin(); it != this->args.end(); it++){
      command += (*it + " ");
    }
    char* args[4];
    args[0] = "/bin/bash";
    args[1] = "-c";
    args[3] = NULL;
    char cmd[200] = "";
    strcat(cmd, this->cmd_line.c_str());
    args[2] = cmd;
    if (execv(args[0], args) < 0){
      perror("smash error: execv failed");
      exit(1);
    }
  }
}

//_________Command_________________
Command::Command(const char* cmd_line) : cmd_line(cmd_line) {}

//____BuiltInCommand_______________
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

//____GetCurrDir___________________

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute(){
    char * dir_path=new char[PATH_MAX];
    if(getcwd(dir_path,PATH_MAX)!=nullptr){
        std::cout<<dir_path<<std::endl;
    }
    delete[] dir_path;
}

//____jobs_________________________
JobsList::JobEntry::JobEntry(Command command,int process_id,bool is_stopped){
    this.command=command;
    this.process_id=process_id;
    time(&(this.insertion_time));
    this.is_stopped=is_stopped;
}
void JobsList::JobEntry::PrintJob() {
    cout<<this->command<<' : '<<
    this->process_id<<difftime(this->insertion_time);

    if(this->is_stopped){
        std::cout<<"(stopped)"
    }
    std::cout<<endl;
}
void JobsList::JobEntry::killJob() {
    kill(this->process_id,SIGKILL);
    std::cout<<this->process_id<<": "<<this->command<<endl;
    this->is_finished= true;
}
pid_t JobsList::JobEntry::getPID() {
    return this->process_id;
}
void JobsList::JobEntry::stopped() {
    this->is_stopped=true;
}
void JobsList::JobEntry::resumed() {
    this->is_stopped=false;
}

void JobsList::addJob(Command* cmd, bool isStopped = false, int job_pid){
    Joblist::JobEntry newJob(cmd,job_pid,is_stopped);
    this->pid_to_index[job_pid]=this->next_job_ID;
    if(isStopped)
        this->stopped_jobs_list[next_job_ID]=this->next_job_ID;
    else
        this->running_jobs_list[next_job_ID]=this->next_job_ID;
    this->next_job_ID++;
}
void JobsList::printJobsList(){
    this->removeFinishedJobs();
    JobEntry *cur_job;
    for (int i=1;i<this->next_job_ID;i++){
        cur_job=this->getJobById(i)
        if(cur_job== nullptr)
            continue;
        cout<<'['<<i<<']'<<' ';
        cur_job.PrintJob();
    }
}
void JobsList::killAllJobs(){
    int num_of_jobs=this->stopped_jobs_list.size()
            +this->running_jobs_list.size();
    cout<<"smash: sending SIGKILL signal to "<<num_of_jobs<<" jobs:";
    map<int, JobEntry>::iterator iter=this->stopped_jobs_list.begin();
    while(iter!=this->stopped_jobs_list.end())
        iter->second.killJob();
    iter=this->running_jobs_list.begin();
    while(iter!=this->running_jobs_list.end())
        iter->second.killjob();

}
void JobsList::removeFinishedJobs(){
    int status;
    pid_t cur_pid;
    while((cur_pid=waitpid(0,&status,WNOHANG))>0){
        kill(cur_pid,SIGKILL);
        int job_id=this->pid_to_index[cur_pid];
        this->removeJobById(job_id);
    }
}
JobEntry * JobsList::getJobById(int jobId){
    if(running_jobs_list.find(jobId)!=this->running_jobs_list.end()){
        return &running_jobs_list[jobId];
    }
    else if(stopped_jobs_list.find(jobId)!=this->stopped_jobs_list.end()){
        return &stopped_jobs_list[jobId];
    }
    return nullptr;
}
void JobsList::removeJobById(int jobId){
    this->running_jobs_list.erase(jobId);
    this->stopped_jobs_list.erase(jobId);
}
int JobsList::pidToIndex(pid_t pid){
    if(this->pid_to_index.find(pid)== this->pid_to_index.end())
        return -1
    jobId=this->pid_to_index[pid]
}

void JobsList::jobStopped(int jobId,bool is_pid=false) {
    if (is_pid)
        jobId = this->pidToIndex(jobId);
    map<int, JobEntry>::iterator iter;
    iter = this->running_jobs_list.find(jobId);
    if (iter == running_jobs_list.end())
        return;
    this->stopped_jobs_list[iter->first] = iter->second;
    (iter->second)->stopped();
    this->running_jobs_list.erase(iter->first);
}

void JobsList::jobResumed(int jobId,bool is_pid=false){
    if (is_pid)
        jobId = this->pidToIndex(jobId);
    map<int, JobEntry>::iterator iter;
    iter = this->stopped_jobs_list.find(jobId);
    if (iter == stopped_jobs_list.end())
        return;
    this->running_jobs_list[iter->first] = iter->second;
    (iter->second)->started();
    this->stopped_jobs_list.erase(iter->first);
}