#include <unistd.h>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <dirent.h>

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

#define MAX_DIR_LEN 260



// 0->Valid, 1->Job doesn't exist, 2->No args but job list is empty, 3->Invalid args
#define FG_VALID 0
#define FG_JOB_ID_INVALID 1
#define FG_LIST_EMPTY 2
#define FG_INVALID_ARGS 3


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
    this->job_list = JobsList();
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

    else if (command_name == "pwd"){
        return new GetCurrDirCommand(cmd_line);
    }

    else if (command_name == "kill"){
        if (!KillCommand::validLine(split_line)){
            cout << "smash error: kill: invalid arguments" << endl;
            return nullptr;
        }
        string sig_str = split_line[1].substr(1, split_line[1].size() - 1);
        //Convert strings to int
        int sig_num = stoi(sig_str, nullptr, 10);
        int job_id = stoi(split_line[2], nullptr, 10);
        return new KillCommand(cmd_line, job_id, sig_num); //TODO_NADAV MAYBE SEND POINTER TO JOB LIST
    }

    else if (command_name == "jobs"){
        return new JobsCommand(cmd_line);
    }

    else if (command_name == "ls"){
        return new ListDirectoryContents(cmd_line);
    }

    else if(command_name == "fg"){
      int job_id = 0;
      int validity = ForegroundCommand::validLine(split_line);
      if (validity == FG_JOB_ID_INVALID){
        cout << "smash error: fg: job-id " << stoi(split_line[1]) << " does not exist" << endl;
        return nullptr;
      }
      if (validity == FG_LIST_EMPTY){
        cout << "smash error: fg: jobs list is empty" << endl;
        return nullptr;
      }
      if (validity == FG_INVALID_ARGS){
        cout << "smash error: fg: invalid arguments" << endl;
        return nullptr;
      }
      if (split_line[1] != "")
        job_id = stoi(split_line[1]);
      else
        job_id = -1;
      return new ForegroundCommand(cmd_line, job_id);
    }

    else if (command_name != "") {
        bool bg_run = false;

        //Checking for '&' symbol
        if (_isBackgroundComamnd(cmd_line)){
          bg_run = true;
        }

        return new ExternalCommand(cmd_line, bg_run);
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

JobsList* SmallShell::getJobList(){
    return &this->job_list;
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
    cout << "smash pid is " << this->pid << endl;
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

ExternalCommand::ExternalCommand(const char* cmd_line, bool bg_run) : Command(cmd_line), bg_run(bg_run) {

}

void ExternalCommand::execute() {

    int p = fork();

    if(p < 0){
        perror("smash error: fork failed");
        return;
    }
    else if (p > 0){
        if(!this->bg_run){
            wait(NULL);
        }
        else{
            SmallShell::getInstance().getJobList()->addJob(string(cmd_line), p, false);
        }
    }
    else {

        setpgrp();
        string temp_line = string(this->cmd_line);
        temp_line += ";";

        //Removing '&'
        if (this->bg_run){
          for (auto it = temp_line.begin(); it != temp_line.end(); it++){
            if (*it == '&'){
              temp_line.erase(it);
              break;
            }
          }
        }

        char sent_cmd[200] = "";
        strcat(sent_cmd, temp_line.c_str());

        char* args[4];
        args[0] = "/bin/bash";
        args[1] = "-c";
        args[3] = NULL;
        args[2] = sent_cmd;
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
    char* dir_path = new char[MAX_DIR_LEN];
    if(getcwd(dir_path,MAX_DIR_LEN)!=nullptr){
        std::cout<<dir_path<<std::endl;
    }
    delete[] dir_path;
}

//____kill____
KillCommand::KillCommand(const char* cmd_line, int jobID, int sig_num) : BuiltInCommand(cmd_line), jobID(jobID), sig_num(sig_num) {}

void KillCommand::execute() {
    SmallShell::getInstance().getJobList()->sendSignal(jobID, sig_num);
}

bool KillCommand::validLine(vector<string> line){
    //Check number of argumants
    if (line[3] != "" || line[2] == "" || line[1] == "")
        return false;
    //Check "-<signum>" format
    string str = line[1];
    string::iterator it = str.begin();
    if (*it != '-')
        return false;
    it++;
    while (it != str.end()){
        if (*it < '0' || *it > '9')
            return false;
        it++;
    }
    //Check <job-id> format
    str = line[2];
    it = str.begin();
    while (it != str.end()){
        if (*it < '0' || *it > '9')
            return false;
        it++;
    }
    //Everything's fine
    return true;
}

//____jobs_________________________
JobsList::JobEntry::JobEntry(const string &command, int process_id, bool is_stopped)
        :command(command), process_id(process_id), is_stopped(is_stopped){
    time(&insertion_time);
}

void JobsList::JobEntry::PrintJob() {
    time_t cur_time;
    time(&cur_time);
    cout << this->command << " : " << this->process_id << " "
         << difftime(cur_time,this->insertion_time) << " secs";
    if(this->is_stopped){
        cout << " (stopped)";
    }
    std::cout << endl;
}

void JobsList::JobEntry::killJob(int signal,bool print) {
    if(print){
        cout << "signal number " << signal << " was sent to pid " << process_id << endl;
    }
    if(kill(this->process_id,signal) != 0){
        perror("“smash error: kill failed");
    }
}

pid_t JobsList::JobEntry::getPID() const {
    return this->process_id;
}

string JobsList::JobEntry::getCommand() const {
    return this->command;
}

void JobsList::JobEntry::stoppedOrResumed(bool stopping) {
    this->is_stopped=stopping;
}

void JobsList::addJob(string cmd, int job_pid, bool isStopped) {
    JobEntry new_entry(cmd, job_pid, isStopped);
    auto iter = this->job_list.rbegin();
    int insert_to;
    if(iter == this->job_list.rend())
        insert_to = 1;
    else
        insert_to = iter->first + 1;
    this->job_list[insert_to] = new_entry;
    this->pid_to_index[job_pid] = insert_to;

    //PRINTING JOB LIST ON EACH INSERT FOR TESTING
    // for (auto it = this->job_list.begin(); it != this->job_list.end(); it++) {
    //    cout << "ID: " << it->first << ", Command: " << it->second.getCommand() << endl;
    // }
}

void JobsList::printJobsList() {
    this->removeFinishedJobs();
    for(auto iter = this->job_list.begin(); iter != this->job_list.end(); iter++){
        cout << '[' << iter->first << ']';
        iter->second.PrintJob();
    }
}

void JobsList::killAllJobs() {
    cout<<"smash: sending SIGKILL signal to " << this->job_list.size() << "jobs:" << endl;
    for(auto iter = this->job_list.begin(); iter != this->job_list.end(); iter++){
        cout << iter->second.getPID() << ": " << iter->second.getCommand() << endl;
        iter->second.killJob(SIGKILL,false);
    }
}

void JobsList::removeFinishedJobs() {
    int status;
    pid_t cur_pid;
    while((cur_pid=waitpid(-1, &status, WNOHANG)) > 0){
        cout << "REMOVED" << endl;
        int job_id = this->pid_to_index[cur_pid];
        this->removeJobById(job_id);
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    auto iter = this->job_list.find(jobId);
    if(iter == this->job_list.end())
        return nullptr;
    return &iter->second;
}

bool JobsList::JobEntry::checkStopped(){
  return this->is_stopped;
}

void JobsList::removeJobById(int jobId) {
    this->pid_to_index.erase(this->job_list[jobId].getPID());
    this->job_list.erase(jobId);
}

int JobsList::pidToIndex(pid_t pid) {
    auto iter = this->pid_to_index.find(pid);
    if(iter == this->pid_to_index.end())
        return -1;
    return iter->second;
}

void JobsList::jobStoppedOrResumed(int jobId,bool isStopped) {
    auto cur_job = this->getJobById(jobId);
    if (cur_job == nullptr)
        return;
    else
        cur_job->stoppedOrResumed(isStopped);
}

void JobsList::sendSignal(int jobID, int sig_num){
    if (this->job_list.find(jobID) == this->job_list.end()) {
        cout << "smash error: kill: job-id " << jobID << " does not exist" << endl;
    }
    else {
        this->job_list[jobID].killJob(sig_num, true);
    }
}

int JobsList::lastJob(){
  return this->job_list.rbegin()->first;
}

bool JobsList::checkStopped(int jobID){
  return this->job_list[jobID].checkStopped();
}

bool JobsList::isEmpty(){
  return this->job_list.empty();
}

//____jobsCommand____
JobsCommand::JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute(){
    SmallShell::getInstance().getJobList()->printJobsList();
}

//________ls________

ListDirectoryContents::ListDirectoryContents(const char* cmd_line) : BuiltInCommand(cmd_line){}

void ListDirectoryContents::execute() {
    struct dirent** dir;
    int test=scandir(".", &dir, NULL, alphasort);
    for(int i=0;i<test;i++){
        cout<<dir[i]->d_name<<endl;
        free(dir[i]);
    }
    free(dir);
}


//______fg________
ForegroundCommand::ForegroundCommand(const char* cmd_line, int job_id) : BuiltInCommand(cmd_line){
  if (job_id <= 0){
    this->job_id = SmallShell::getInstance().getJobList()->lastJob();
  }
  else{
    this->job_id = job_id;
  }
}

void ForegroundCommand::execute(){
  if (SmallShell::getInstance().getJobList()->checkStopped(this->job_id)){
    SmallShell::getInstance().getJobList()->sendSignal(this->job_id, SIGCONT);
  }
  pid_t pid = SmallShell::getInstance().getJobList()->getJobById(this->job_id)->getPID();
  waitpid(pid, NULL, WUNTRACED);
  SmallShell::getInstance().getJobList()->removeJobById(this->job_id);
}

// 0->Valid, 1->Job doesn't exist, 2->No args but job list is empty, 3->Invalid args
int ForegroundCommand::validLine(vector<string> split){
  if(split[2] != "")
    return FG_INVALID_ARGS;

  if (split[1] != ""){
    for (auto it = split[1].begin(); it != split[1].end(); it++){
      if (*it < '0' || *it > '9')
        return FG_INVALID_ARGS;
    }
    int job_id = stoi(split[1]);
    if (SmallShell::getInstance().getJobList()->getJobById(job_id) == nullptr){
      return FG_JOB_ID_INVALID;
    }
  }
  else {
    if (SmallShell::getInstance().getJobList()->isEmpty()){
      return FG_LIST_EMPTY;
    }
  }
  return FG_VALID;
}