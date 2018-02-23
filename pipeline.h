/**
    @file pipeline.h
    @author Connor Bray
    @date 23 Feb. 2018
    @brief Simulation automation tools repo

    This file contains a variety of useful functions for automating simulation execution.

    The following arguments are required to compile: `-std=c++11 -lX11 -lXtst -pthread`

    The following arguments are recommended: `-Ofast -Wall`

    To precompile the header: `g++ -std=c++11 -lX11 -lXtst -pthread -Ofast -Wall -c pipeline.h`
*/

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

using namespace std;


/**
 @brief Call the given command in bash
 ## Call the given command in bash

 ### Arguments
 * `std::string command` - Command to run. Note that non-interpreters wrappers like nice may not work with aliased commands.

 * `int nice` - nice value to run command at (added to avoid issues with nice not working with aliases). Defualts to zero.

  ### Returns the return value of the given function
*/
int bash(std::string command,int nice=0){
    cout << command + "\n";
    string shell = "\
    #!/bin/bash \n\
    source ~/.bashrc\n\
    renice -n "+to_string(nice)+" -p $$\n\
    ";
    int i,ret = system((shell+command).c_str());
    i=WEXITSTATUS(ret); // Get return value.
    return i;
}

/**
 @brie Returns a human-readable string of a duration.
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
### Executes GRSISort with given command.

### Arguments:
* `string command` - GRSISort command.

* `string windowName` - command terminal name (because keyboard emulation is used to type in command).
*/
void gs(string command,string windowName){
    bash("xdotool windowactivate $(xdotool search --name '"+windowName+"') && xdotool windowfocus $(xdotool search --name '"+windowName+"') ; sleep 2 && xdotool type '"+command+"' && xdotool key KP_Enter &");
    system("grsisort -l");
}

/**
@brief Quits GRSISort when done file appears
### Quits GRSISort when done file appears

## Notes:
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
@brief Executes command in GRSISort
 ## Executes command in GRSISort

 ### Arguments:
 * `string command` - GRSISort command.

 * `string windowName` - command terminal name (because keyboard emulation is used to type in command).

 ### Notes:
 * Runs whichever version of GRSISort is the "default" (AKA the one returned by `which grsisort` in your bourne shell (sh shell))
 * To exit, the gs macro must write the command's pid to a file called 'done' so that the quit command knows which to quit.
*/
void grsisort(string command,string windowName){
    /// Vector of treads initiated to simplify rejoining
    vector<thread> threadpool;
    threadpool.push_back(thread(gs,command,windowName)); // Run GRSISort macro
    threadpool.push_back(thread(quit)); // Quit GRSISort after it opens windowfocus
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
bool directoryContains(string dir){
    char run_path[256];
    getcwd(run_path, 255);
    string path = run_path;
    if(path.find(dir)==string::npos){
        cout << "Warning, not running in "+dir+", which may cause out of storage issues. Please consider running from "+dir+". Press enter to quit or c then enter to continue." << endl;
        char input[2];
        cin.getline(input,2);
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
bool directoryCheck(string dir){
    char run_path[256];
    getcwd(run_path, 255);
    string path = run_path;
    if(path != dir){
        cout << "Warning, not running in "+dir+", which may cause out of storage issues. Please consider running from "+dir+". Press enter to quit or c then enter to continue." << endl;
        char input[2];
        cin.getline(input,2);
        if(input[0]!='c'&&input[0]!='C') return 1;
    }
    return 0;
}

/**
@brief Executes command in ROOT
 ## Executes command in ROOT

 ### Arguments:
 * `string command` - ROOT command.
*/
int root(string command){
    return bash("root -q -e '"+command+"'");
}
