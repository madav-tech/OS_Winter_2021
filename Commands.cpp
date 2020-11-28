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
#include <fcntl.h>

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

#define BG_VALID 0
#define BG_JOB_ID_INVALID 1
#define BG_INVALID_ARGS 3
#define BG_ALREADY_RUNNING 4
#define BG_NO_JOBS_STOPPPED -1

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
    this->current_job = nullptr;
    this->timeout_list = TimeoutList();
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
    for(auto iter = split_line.begin(); iter != split_line.end(); iter++){
        if(*iter == ">" || *iter == ">>"){
            string string_command = "";
            for(auto command_iter = split_line.begin(); command_iter != iter; command_iter++){
                string_command += *command_iter;
                string_command += " ";
            }
            Command * redirection_command = this->CreateCommand(string_command.c_str());
            return new RedirectionCommand(cmd_line, redirection_command, *(iter + 1), *iter);

        }
    }
    for(auto iter=split_line.begin();iter!=split_line.end();iter++){
        if(*iter=="|"||*iter=="|&"){
            string src_string="";
            for(auto command_iter=split_line.begin();command_iter!=iter;command_iter++){
                src_string+=*command_iter;
                src_string+=" ";
            }
            cout<<src_string<<endl;
            Command * src_command=this->CreateCommand(src_string.c_str());
            string dest_string="";
            for(auto command_iter=iter+1;command_iter!=split_line.end();command_iter++){
                dest_string+=*command_iter;
                dest_string+=" ";
            }
            cout<<dest_string<<endl;
            Command * dest_command=this->CreateCommand(dest_string.c_str());
            return new PipeCommand(cmd_line,src_command,dest_command,*iter);
        }
    }

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

    else if(command_name == "bg"){
        int job_id = 0;
        int validity = BackgroundCommand::validLine(split_line);

        if (validity == BG_JOB_ID_INVALID){
            cout << "smash error: bg: job-id "<< stoi(split_line[1]) <<" does not exist" << endl;
            return nullptr;
        }
        if (validity == BG_INVALID_ARGS){
            cout << "smash error: bg: invalid arguments" << endl;
            return nullptr;
        }
        if (validity == BG_ALREADY_RUNNING){
            cout << "smash error: bg: job-id " << stoi(split_line[1]) << " is already running in the background" << endl;
            return nullptr;
        }
        if (validity == BG_NO_JOBS_STOPPPED){
            cout << "smash error: bg: there is no stopped jobs to resume" << endl;
            return nullptr;
        }
        if (split_line[1] != "")
            job_id = stoi(split_line[1]);
        else
            job_id = -1;
        return new BackgroundCommand(cmd_line, job_id);
    }

    else if(command_name == "quit"){
        JobsList* jobs = this->getJobList();
        bool kill = false;
        if(split_line[1] == "kill")
            kill = true;
        return new QuitCommand(cmd_line, jobs, kill);
    }

    else if(command_name == "timeout"){
        int duration = stoi(split_line[1]);
        bool bg_run = false;
        if(_isBackgroundComamnd(cmd_line)){
            bg_run = true;
        }
        return new TimeoutCommand(cmd_line, duration, bg_run);
    }

    //TO NOT RUN EMPTY COMMAND
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
    this->job_list.removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
    if (cmd != nullptr) {
        cmd->execute();
    }
    delete cmd;
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

JobsList::JobEntry* SmallShell::getCurrentJob(){
  return this->current_job;
}

void SmallShell::setCurrentJob(JobsList::JobEntry* job){
  this->current_job = job;
}

TimeoutList* SmallShell::getTimeoutList(){
    return &this->timeout_list;
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
            SmallShell &smash = SmallShell::getInstance();
            smash.setCurrentJob(new JobsList::JobEntry(string(cmd_line), p, false));
            int status;
            waitpid(p, &status, WUNTRACED);
            smash.setCurrentJob(nullptr);

            if(WIFSTOPPED(status))
                smash.getJobList()->addJob(string(cmd_line), p,true);
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
    for(auto iter = this->job_list.begin(); iter != this->job_list.end(); iter++){
        cout << '[' << iter->first << ']';
        iter->second.PrintJob();
    }
}

void JobsList::killAllJobs(bool kill) {
    if(kill)
        cout<<"smash: sending SIGKILL signal to " << this->job_list.size() << " jobs:" << endl;
    for(auto iter = this->job_list.begin(); iter != this->job_list.end(); iter++){
        if(kill)
            cout << iter->second.getPID() << ": " << iter->second.getCommand() << endl;
        iter->second.killJob(SIGKILL,false);
    }
}

void JobsList::removeFinishedJobs() {
    int status;
    pid_t cur_pid;
    while((cur_pid=waitpid(-1, &status, WNOHANG)) > 0){
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

void JobsList::sendSignal(int jobID, int sig_num, bool print){
    if (this->job_list.find(jobID) == this->job_list.end()) {
        cout << "smash error: kill: job-id " << jobID << " does not exist" << endl;
    }
    else {
        this->job_list[jobID].killJob(sig_num, print);
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

int JobsList::lastStoppedJob(){
    for (auto it = this->job_list.rbegin(); it != this->job_list.rend(); it++){
        if (it->second.checkStopped())
            return it->first;
    }
    return BG_NO_JOBS_STOPPPED;
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
    
    JobsList::JobEntry* job_pointer = SmallShell::getInstance().getJobList()->getJobById(this->job_id);
    cout << job_pointer->getCommand() << " : " << job_pointer->getPID() << endl;
    if (SmallShell::getInstance().getJobList()->checkStopped(this->job_id)){
        SmallShell::getInstance().getJobList()->sendSignal(this->job_id, SIGCONT, false);
    }
    pid_t pid = job_pointer->getPID();
    SmallShell::getInstance().setCurrentJob(job_pointer);
    int status;
    waitpid(pid, &status, WUNTRACED);
    SmallShell::getInstance().setCurrentJob(nullptr);
    if(!WIFSTOPPED(status)) {
        SmallShell::getInstance().getJobList()->removeJobById(this->job_id);
    }
    else{
        SmallShell::getInstance().getJobList()->jobStoppedOrResumed(this->job_id, true);
    }
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
    if (SmallShell::getInstance().getJobList()->isEmpty()){
      return FG_LIST_EMPTY;
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


//________quit___________
QuitCommand::QuitCommand(const char* cmd_line,JobsList* jobs , bool kill = false) :
                         BuiltInCommand(cmd_line) , jobs(jobs) , kill(kill){}

void QuitCommand::execute(){
    this->jobs->killAllJobs(this->kill);
    exit(0);
}

//________RedirectionCommand__________
RedirectionCommand::RedirectionCommand(const char* cmd_line,Command* redirected_command, string file, string append):
        Command(cmd_line),redirected_command(redirected_command),file(file){
    if(append== ">>")
        this->append=true;
    else
        this->append=false;
}

// TODO : errors handling?
void RedirectionCommand::execute() {
    int fd;
    int ret=dup(STDOUT_FILENO);
    int file_options = (this->append)? O_APPEND : O_TRUNC;
    file_options=file_options | O_CREAT | O_WRONLY;

    if(ret<0){
        perror("smash error: dup failed");
        return;
    }
    fd = open((this->file).c_str(),file_options);
    if(fd <0){
        perror("smash error: Open failed");
        return;
    }
    int res=dup2(fd,STDOUT_FILENO);
    if(res <0){
        perror("smash error: Redirection failed");
        return;
    }
    this->redirected_command->execute();
    close(fd);
    fflush(stdout);
    dup2(ret,STDOUT_FILENO);
    return;
}
RedirectionCommand::~RedirectionCommand() noexcept {
    delete this->redirected_command;
}

//_______bg___________

BackgroundCommand::BackgroundCommand(const char* cmd_line, int job_id) : BuiltInCommand(cmd_line) {
    if (job_id <= 0){
        this->job_id = SmallShell::getInstance().getJobList()->lastStoppedJob();
    }
    else{
        this->job_id = job_id;
    }
}

void BackgroundCommand::execute(){
    JobsList::JobEntry* job_pointer = SmallShell::getInstance().getJobList()->getJobById(this->job_id);
    cout << job_pointer->getCommand() << " : " << job_pointer->getPID() << endl;
    SmallShell::getInstance().getJobList()->sendSignal(this->job_id, SIGCONT, false);
    SmallShell::getInstance().getJobList()->jobStoppedOrResumed(this->job_id, false);
}

int BackgroundCommand::validLine(vector<string> split){
    if(split[2] != ""){
        return BG_INVALID_ARGS;
    }
    if(split[1] != ""){
        for (auto it = split[1].begin(); it != split[1].end(); it++){
            if (*it < '0' || *it > '9')
                return BG_INVALID_ARGS;
        }
        int job_id = stoi(split[1]);
        if (SmallShell::getInstance().getJobList()->getJobById(job_id) == nullptr){
            return BG_JOB_ID_INVALID;
        }
        if (!SmallShell::getInstance().getJobList()->getJobById(job_id)->checkStopped()){
            return BG_ALREADY_RUNNING;
        }
        return BG_VALID;
    }
    else{
        int last_stopped_id = SmallShell::getInstance().getJobList()->lastStoppedJob();
        if (last_stopped_id == BG_NO_JOBS_STOPPPED){
            return BG_NO_JOBS_STOPPPED;
        }
        return BG_VALID;
    }
}

//________RedirectionCommand__________
PipeCommand::PipeCommand(const char* cmd_line, Command* src,Command* dest,string err_pipe) :
        Command(cmd_line),src_command(src),dest_command(dest){
    if(err_pipe=="|&")
        this->err_pipe=true;
    else
        this->err_pipe=false;
}

void PipeCommand::execute() {
    int the_pipe[2];

    int suc=pipe(the_pipe);
    if(suc==-1){
        perror("smash error: pipe failed");
        return;
    }

    int pid = fork();
    if(pid==0){
        close(the_pipe[1]);
        int ret=dup(STDIN_FILENO);
        dup2(the_pipe[0],STDIN_FILENO);
        this->dest_command->execute();

        dup2(ret,STDIN_FILENO);
        exit(1);
    }
    else if(pid>0){
        close(the_pipe[0]);
        int ret=dup(STDOUT_FILENO);
        dup2(the_pipe[1],STDOUT_FILENO);
        this->src_command->execute();
        waitpid(pid, nullptr,0);
        dup2(ret,STDOUT_FILENO);
    }
    else{
        perror("smash error: fork failed");
        return;
    }
    return;
}
PipeCommand::~PipeCommand() noexcept {
    delete this->dest_command;
    delete this->src_command;
}

//______Timeout list_______
TimeoutList::TimeoutEntry::TimeoutEntry(const string &command, int process_id, int duration) : command(command), process_id(process_id), duration(duration){
    this->insertion_time = time(nullptr);
}

pid_t TimeoutList::TimeoutEntry::getPID(){
    return this->process_id;
}

string TimeoutList::TimeoutEntry::getCommand(){
    return this->command;
}

void TimeoutList::TimeoutEntry::timeout(){
    if (kill(this->process_id, SIGKILL) != 0){
        perror("smash error: kill failed");
        return;
    }
    cout << "smash: " << this->command << " timed out!" << endl;
}

bool TimeoutList::TimeoutEntry::isDoomed(){
    return this->insertion_time + this->duration <= time(nullptr);
}

int TimeoutList::TimeoutEntry::getDuration(){
    return this->duration;
}

int TimeoutList::TimeoutEntry::getInserted(){
    return this->insertion_time;
}

void TimeoutList::addEntry(string cmd, int pid, int duration){
    
    TimeoutEntry entry(cmd, pid, duration);
    this->timed_list.push_back(entry);

    //Finding new alarm time
    int min_time = -1;
    for (auto it = this->timed_list.begin(); it != this->timed_list.end(); it++){
        if (min_time == -1 || (it->getDuration() - (time(nullptr)-it->getInserted()) < min_time)){
            min_time = it->getDuration() - (time(nullptr)-it->getInserted());
        }
    }
    if (min_time != -1){
        alarm(min_time);
    }


    for (auto it = this->timed_list.begin(); it != this->timed_list.end(); it++){
        cout << "CMD: " << it->getCommand() << endl;
    }
}

void TimeoutList::doomEntry(){
    for (auto it = this->timed_list.begin(); it != this->timed_list.end(); it++){
        if (it->isDoomed()){
            it->timeout();
            this->timed_list.erase(it);
            break;
        }
    }

    //Finding new alarm time
    int min_time = -1;
    for (auto it = this->timed_list.begin(); it != this->timed_list.end(); it++){
        if (min_time == -1 || (it->getDuration() - (time(nullptr)-it->getInserted()) < min_time)){
            min_time = it->getDuration() - (time(nullptr)-it->getInserted());
        }
    }
    if (min_time != -1){
        alarm(min_time);
    }
}

//___________Timeout Command_______________
TimeoutCommand::TimeoutCommand(const char* cmd_line, int duration, bool bg_run) : Command(cmd_line), duration(duration), bg_run(bg_run){
    
}



void TimeoutCommand::execute(){

    int p = fork();

    if(p < 0){
        perror("smash error: fork failed");
        return;
    }
    else if (p > 0){

        alarm(this->duration);
        SmallShell::getInstance().getTimeoutList()->addEntry(this->cmd_line, p, this->duration);
        if(!this->bg_run){
            SmallShell &smash = SmallShell::getInstance();
            smash.setCurrentJob(new JobsList::JobEntry(string(cmd_line), p, false));
            int status;
            waitpid(p, &status, WUNTRACED);
            smash.setCurrentJob(nullptr);

            if(WIFSTOPPED(status))
                smash.getJobList()->addJob(string(cmd_line), p,true);
        }
        else{
            SmallShell::getInstance().getJobList()->addJob(string(cmd_line), p, false);
        }
    }
    else {
        setpgrp();

        string temp_line = string(this->cmd_line);
        temp_line += ";";
        string cut_line = temp_line.substr(string("timeout ").size());

        int chars_to_cut = 0;
        for (auto it = cut_line.begin(); it != cut_line.end(); it++){
            if (*it >= '0' && *it <= '9'){
                chars_to_cut ++;
            }
            else{
                chars_to_cut++;
                break;
            }
        }
        cut_line = cut_line.substr(chars_to_cut);

        //Removing '&'
        if (this->bg_run){
          for (auto it = cut_line.begin(); it != cut_line.end(); it++){
            if (*it == '&'){
              cut_line.erase(it);
              break;
            }
          }
        }

        char sent_cmd[200] = "";
        strcat(sent_cmd, cut_line.c_str());

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