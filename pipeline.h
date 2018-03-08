/**
    @file pipeline.h

    @author Connor Bray

    @date 23 Feb. 2018

    @brief Simulation automation tools repo

    This file contains a variety of useful functions for automating simulation execution.

    The following arguments are required to compile: `-std=c++11 -lX11 -lXtst -pthread -ldl -ldw -g`

    The following arguments are recommended: `-Ofast -Wall -g`

    To precompile the header: `g++ -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -g -Ofast -Wall -c pipeline.h`
*/
#pragma once
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <ctime>
#include <chrono>
#include <limits.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <cstring>

/**
 @brief Call the given command in bash

 ## Call the given command in bash

 ### Arguments
 * `std::string command` - Command to run. Note that non-interpreters wrappers like nice may not work with aliased commands.

 * `int nice` - nice value to run command at (added to avoid issues with nice not working with aliases). Defualts to zero.

  ### Returns the return value of the given function

  ### Notes:
  * Your bashrc must be configured to only run commands that require interactivity if interactivity is present, otherwise problems may insue.
*/
int bash(std::string command,int nice=0){
    std::cout << command + "\n";
    std::string shell = "\
    #!/bin/bash \n\
    source ~/.bashrc\n\
    renice -n "+std::to_string(nice)+" -p $$\n\
    ";
    int i,ret = system((shell+command).c_str());
    i=WEXITSTATUS(ret); // Get return value.
    return i;
}

/**
 @brief Returns a human-readable string of a duration.

    ## Returns a human-readable string of a duration. Credit to TankorSmash on Stackoverflow for the code provided freely in answer to a question asked by sorush-r

    ### Arguments:
    * `std::chrono::seconds input_seconds` - Duration to convert to human readable string.

    ### Returns:
    * `std::string` - Human readable time duration.
*/
std::string beautify_duration(std::chrono::seconds input_seconds)
{
    using namespace std::chrono;
    typedef duration<int, std::ratio<86400>> days;
    auto d = duration_cast<days>(input_seconds);
    input_seconds -= d;
    auto h = duration_cast<hours>(input_seconds);
    input_seconds -= h;
    auto m = duration_cast<minutes>(input_seconds);
    input_seconds -= m;
    auto s = duration_cast<seconds>(input_seconds);

    auto dc = d.count();
    auto hc = h.count();
    auto mc = m.count();
    auto sc = s.count();

    std::stringstream ss;
    ss.fill('0');
    if (dc) {
        ss << d.count() << "d";
    }
    if (dc || hc) {
        if (dc) { ss << std::setw(2); } //pad if second set of numbers
        ss << h.count() << "h";
    }
    if (dc || hc || mc) {
        if (dc || hc) { ss << std::setw(2); }
        ss << m.count() << "m";
    }
    if (dc || hc || mc || sc) {
        if (dc || hc || mc) { ss << std::setw(2); }
        ss << s.count() << 's';
    }

    return ss.str();
}

/**
@brief Executes GRSISort with given command

## Executes GRSISort with given command.

### Arguments:
* `string command` - GRSISort command.

* `string windowName` - command terminal name (because keyboard emulation is used to type in command).
*/
void gs(std::string command,std::string windowName){
    bash("xdotool windowactivate $(xdotool search --name '"+windowName+"') && xdotool windowfocus $(xdotool search --name '"+windowName+"') ; sleep 2 && xdotool type '"+command+"' && xdotool key KP_Enter &");
    system("grsisort -l");
}

/**
@brief Quits GRSISort when done file appears

## Quits GRSISort when done file appears

### Notes:
The gs macro needs to write its PID to the `done` file in order for this command to work. For gs commands that exit properly, just create an empty file `done` to continue.

Continuously looks for file "done" without the quotes. When it appears, it waits a few seconds and quits GRSTsort with the kill command.

This is a workaround for GRSISort's crashing at the end of the simulation. All of the analysis is complete, so data integrity is preserved.
*/
void quit(){
    bool done=0;
    while(!done){
        struct stat buf;
        done=(stat("done", &buf)== 0);
    }
    sleep(3);
    bash("kill -s INT $(cat done) ; kill -s INT $(cat done) ; kill -s INT $(cat done) ; kill -s INT $(cat done) ; kill -s INT $(cat done) ; kill -s INT $(cat done)");
    bash("rm done");
    // Change window name to prevent confusion
}

/**
@brief Executes, then quits, command in GRSISort

 ## Executes command in GRSISort

 ### Arguments:
 * `string command` - GRSISort command.

 * `string windowName` - command terminal name (because keyboard emulation is used to type in command).

 ### Notes:
 * Runs whichever version of GRSISort is the "default" (AKA the one returned by `which grsisort` in your bourne shell (sh shell))
 * To exit, the gs macro must write the command's pid to a file called 'done' so that the quit command knows which to quit.
*/
void grsisort(std::string command,std::string windowName){
    /// Vector of treads initiated to simplify rejoining
    std::vector<std::thread> threadpool;
    threadpool.push_back(std::thread(gs,command,windowName)); // Run GRSISort macro
    threadpool.push_back(std::thread(quit)); // Quit GRSISort after it opens windowfocus
    for(unsigned int j=0;j<threadpool.size();j++) threadpool[j].join(); //Rejoin threads
}

/**
@brief Replace all instances of a string in another string with a third string

 ## Replace all instances of a string in another string with a third string. Credit to Czarek Tomczak on stack overflow for supplying the code in response to a question asked by NullVoxPopuli.

 ### Arguments:
 * `std::string subject` - String to do replacements in.

 * `const std::string& search` - Substring to replace with the `replace` string.

 * `const std::string& replace` - Substring to replace the `search` string with.
*/
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

/**
@brief Check current directory contains the string provided

 ## Check current directory contains the string provided

 ### Arguments:
 * `string dir` - Directory to check if contained.

 ### Notes:
 If it is, it returns zero, otherwise it prompts the user if they want to procede anyway or not. Returns 0 if they want to procede and 1 otherwise.
*/
bool directoryContains(std::string dir){
    char run_path[256];
    getcwd(run_path, 255);
    std::string path = run_path;
    if(path.find(dir)==std::string::npos){
        std::cout << "Warning, not running in "+dir+", which may cause out of storage issues. Please consider running from "+dir+". Press enter to quit or c then enter to continue." << std::endl;
        char input[2];
        std::cin.getline(input,2);
        if(input[0]!='c'&&input[0]!='C') return 1;
    }
    return 0;
}

/**
@brief Check run directory

  ## Check run directory

  ### Arguments:
  * `string dir` - Directory to check if running in.

  ### Notes:
  If it is, it returns zero, otherwise it prompts the user if they want to procede anyway or not. Returns 0 if they want to procede and 1 otherwise.
*/
bool directoryCheck(std::string dir){
    char run_path[256];
    getcwd(run_path, 255);
    std::string path = run_path;
    if(path != dir){
        std::cout << "Warning, not running in "+dir+", which may cause out of storage issues. Please consider running from "+dir+". Press enter to quit or c then enter to continue." << std::endl;
        char input[2];
        std::cin.getline(input,2);
        if(input[0]!='c'&&input[0]!='C') return 1;
    }
    return 0;
}

/**
@brief Check if directory is empty

  ## Check run directory for files

  ### Arguments:
  * `string dir` - Directory to check if running in.

  ### Notes:
  If it is, it returns zero, otherwise it prompts the user for how they want to procede. Returns 0 if they want to procede and 1 otherwise.
*/
bool directoryEmpty(std::string dir){
    int i, ret=system(("DIR='"+dir+"';[ \"$(ls -A $DIR)\" ] && exit 1 || exit 0").c_str());
    i=WEXITSTATUS(ret); // Get return value.
    if(i==0) return 0;
    while(1){
        std::cout << "Directory not empty. Press c then enter to clean, press s then enter to skip, or press e then enter to exit." << std::endl;
        std::string input;
        std::cin >> input;
        if(input[0]=='c'||input[0]=='C'){
            std::cout << "Cleaning directory." << std::endl;
            return bash("rm -f "+dir+"/*");
        }
        if(input[0]=='s'||input[0]=='S'){
            std::cout << "Skipping clean directory." << std::endl;
            return 0;
        }
        if(input[0]=='e'||input[0]=='E'){
            std::cout << "Exiting." << std::endl;
            return 1;
        }
        std::cout << "Error. ";
    }
}


/**
@brief Executes command in ROOT

 ## Executes command in ROOT

 ### Arguments:
 * `string command` - ROOT command.
*/
int root(std::string command){
    return bash("root -q -e '"+command+"'");
}

#define BACKWARD_HAS_DW 1
#include "backward-cpp/backward.hpp"
namespace backward {
backward::SignalHandling sh;
}

/**
@brief Prints backtrace to stdout

 ## Code from https://github.com/bombela/backward-cpp (MIT licenced)

 ### Notes:
 Requires libdw-dev.
*/
void Backtrace(){
    backward::StackTrace st; st.load_here(32);
    backward::Printer p;
    p.object = true;
    p.color_mode = backward::ColorMode::always;
    p.address = true;
    p.print(st, stdout);
}


/**
@brief Emails user

 ## Email user (incomplete, not working.)

 ### Arguments:
 * `string destination` - Email to send to

 * `string subject` - Subject of email

 * `string message` - Message of Email

 * `string key` - GMail API key for sender

 ### Notes:
 To email when program is complete, in your main, add the following line: `std::atexit(email(destination,subject,message,key));`

 You can also set this to email on errors using: ``
*/
void email(std::string destination, std::string subject, std::string message, std::string key){
    return;
}

/**
@brief Slack Message

 ## Sends slack message

 ### Arguments:
 * `string message` - Message to send

 * `string key` - API key - should look something like "https://hooks.slack.com/services/xxxxxxxxx/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

 ### Notes:
 To use, in your main, add the following line: `std::atexit(slack(message,key));`

 To use, you will need to go to api.slack.com (or request I do so), and get an API hook into whichever DM or channel you want to message.

*/
void slack(std::string message, std::string key){
    system(("curl -X POST -H 'Content-type: application/json' --data \"{'text':'"+message+"'}\" "+key).c_str());
    return;
}
