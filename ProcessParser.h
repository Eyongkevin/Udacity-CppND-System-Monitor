#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"


using namespace std;

class ProcessParser{
private:
    std::ifstream stream;
    public:
    static string getCmd(string pid);
    static vector<string> getPidList();
    static string getVmSize(string pid);
    static string getCpuPercent(string pid);
    static long int getSysUpTime();
    static string getProcUpTime(string pid);
    static string getProcUser(string pid);
    static vector<string> getSysCpuPercent(string coreNumber = "");
    static float getSysRamPercent();
    static string getSysKernelVersion();
    static int getNumberOfCores();
    static int getTotalThreads();
    static int getTotalNumberOfProcesses();
    static int getNumberOfRunningProcesses();
    static string getOSName();
    static string PrintCpuStats(std::vector<string> values1, std::vector<string>values2);
    static bool isPidExisting(string pid);
};

// TODO: Define all of the above functions below:
std::string ProcessParser::getVmSize(string pid){
    string line;
    //Declaring search attribute for file
    string name = "VmData"; // this string states the process's memory usage in kilobytes.
    string value;
    string word;
    std::vector<string> keepStr;
    float result;
    //Opening stream for specific file
    std::ifstream stream; 
    Util::getStream(Path::basePath() + pid + Path::statusPath(), stream);

    while(std::getline(stream, line)){
        // compare the string object to the length of strings passed to it as arguments
        if(line.compare(0, name.size(), name) == 0){
            
            // Slicing string line on ws for values using sstream
            /* std::istringstream buf(line);
            while(buf >> word)
                keepStr.push_back(word);

            result = (stof(keepStr[1])/float(1024)); */

            std::istringstream buf(line);
            std::istream_iterator<string> beg(buf), end;
            std::vector<string> values(beg, end);

            result = (stof(values[1])/float(1024));

            break;
        }
    };
    return to_string(result);
}

std::string ProcessParser::getProcUpTime(string pid)
{
    string line;
    string value;
    float result;

    //Opening stream for specific file
    std::ifstream stream; 
    Util::getStream(Path::basePath() + pid + "/" + Path::statPath(), stream);
    getline(stream, line);
    string str = line;
    istringstream buf(str);
    istream_iterator<string> beg(buf), end;
    vector<string> values(beg, end);

    return to_string(float(stof(values[13])/sysconf(_SC_CLK_TCK)));
    //return to_string(result);

}
long int ProcessParser::getSysUpTime()
{
    string line;
    ifstream stream;
    Util::getStream(Path::basePath() + Path::upTimePath(), stream);
    getline(stream, line);
    istringstream buf(line);
    istream_iterator<string> beg(buf), end;
    vector<string> values(beg, end);
    return stoi(values[0]);

}

std::string ProcessParser::getCpuPercent(string pid)
{
    string line;
    string value;
    float result;

    //Opening stream for specific file
    std::ifstream stream; 
    Util::getStream(Path::basePath() + pid + "/" + Path::statPath(), stream);
    getline(stream, line);
    string str = line;
    istringstream buf(str);
    istream_iterator<string> beg(buf), end;
    vector<string> values(beg, end);

    float utime = stof(ProcessParser::getProcUpTime(pid));
    float stime = stof(values[14]);
    float cutime = stof(values[15]);
    float cstime = stof(values[16]);
    float starttime = stof(values[21]);
    float uptime = ProcessParser::getSysUpTime();
    // Use sysconf to get the clock ticks of the host machine.
    float freq = sysconf(_SC_CLK_TCK);
    float total_time = utime + stime + cutime + cstime;
    float seconds = uptime - (starttime/freq);
    result = 100.0*((total_time/freq)/seconds);

    return to_string(result);
    
}

std::string ProcessParser::getProcUser(string pid)
{
    string line;
    string name= "Uid:";
    string result = "";
    string word;
    ifstream stream;
    // this path contain the Uid which is the User Identifier.
    Util::getStream((Path::basePath() + pid + Path::statusPath()), stream);

    while(std::getline(stream, line)){
        // compare the string object to the length of strings passed to it as arguments
        if(line.compare(0, name.size(), name) == 0){
            
            // Slicing string line on ws for values using sstream
            std::istringstream buf(line);
            std::vector<string> keepStr;
            while(buf >> word)
                keepStr.push_back(word);

            result = keepStr[1];
            break;
        }
    };
    if(!result.empty()){
        // This path contain the user name associated with the Uid.
        Util::getStream("/etc/passwd", stream);
        // It is of the format 'x:Uid:Gid...' so we want to find for 'x:Uid'.
        name  = ("x:"+result);
        while(std::getline(stream, line)){
            if(line.find(name) != string::npos){
                result = line.substr(0, line.find(":"));
                return result;
                break;

            }
        }

    }
    return result;
}

std::vector<string> ProcessParser::getPidList()
{
    DIR* dir;
    std::vector<string> container;
    if(!(dir = opendir("/proc")))
        throw std::runtime_error(std::strerror(errno));

    while(dirent* dirp = readdir(dir)){
        //is this a director?
        if(dirp->d_type != DT_DIR)
            continue;
        //is every character of the name a digit?
        //if(all_of(dirp->d_name, dirp->d_name + std::strlen(dirp->d_name), [](char c){return std::isdigit(c);})){
        if(to_string(atoi(dirp->d_name)).size() == std::strlen(dirp->d_name) && dirp->d_name[0] != '.'){
            container.push_back(dirp->d_name);
        } 
    }
    // Validating process of directory closing
    if(closedir(dir))
        throw std::runtime_error(std::strerror(errno));
    
    return container;
}

std::string ProcessParser::getCmd(string pid)
{
    string line;
    ifstream stream;
    Util::getStream((Path::basePath() + pid + Path::cmdPath()), stream);
    std::getline(stream, line);
    return line;
}

int ProcessParser::getNumberOfCores()
{
    // Get the number of host cpu cores
    // This method returns just the first processor's cpu cores. It doens't take in to account multiple processors.
    string line;
    string name = "cpu cores";
    ifstream stream;
    Util::getStream((Path::basePath() + "cpuinfo"), stream);
    while (std::getline(stream, line)) {
        if (line.compare(0, name.size(),name) == 0) {
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            return stoi(values[3]);
        }
    }
    return 0;
}


float get_sys_active_cpu_time(vector<string> values)
{
    return (stof(values[S_USER]) +
            stof(values[S_NICE]) +
            stof(values[S_SYSTEM]) +
            stof(values[S_IRQ]) +
            stof(values[S_SOFTIRQ]) +
            stof(values[S_STEAL]) +
            stof(values[S_GUEST]) +
            stof(values[S_GUEST_NICE]));
}

float get_sys_idle_cpu_time(vector<string>values)
{
    return (stof(values[S_IDLE]) + stof(values[S_IOWAIT]));
}

std::string ProcessParser::PrintCpuStats(vector<string> values1, vector<string> values2)
{
/*
Because CPU stats can be calculated only if you take measures in two different time,
this function has two parameters: two vectors of relevant values.
We use a formula to calculate overall activity of processor.
*/
    float activeTime = get_sys_active_cpu_time(values2) - get_sys_active_cpu_time(values1);
    float idleTime = get_sys_idle_cpu_time(values2) - get_sys_idle_cpu_time(values1);
    float totalTime = activeTime + idleTime;
    float result = 100.0*(activeTime / totalTime);
    return to_string(result);
}

// Return system Ram Percentage
float ProcessParser::getSysRamPercent(){
    string line;
    string name1 = "MemAvailable:";
    string name2 = "MemFree:";
    string name3 = "Buffers:";

    float totalMem = 0;
    float freeMem = 0;
    float buffers = 0;

    ifstream stream;
    Util::getStream((Path::basePath() + Path::memInfoPath()), stream);
    while(std::getline(stream, line)){
        // Check for MemAvailable.
        if(line.compare(0, name1.size(), name1) == 0){
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            totalMem = stof(values[1]);
        }
        // Check for MemFree.
        if(line.compare(0, name2.size(), name2) == 0){
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            freeMem = stof(values[1]);
        }
        // Check for Buffer.
        if(line.compare(0, name3.size(), name3) == 0){
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            buffers = stof(values[1]);
        }
    }
    return float(100.0*(1-(freeMem/(totalMem-buffers))));
}

std::string ProcessParser::getSysKernelVersion(){
    string line;
    string name = "Linux version";
    ifstream stream;
    Util::getStream((Path::basePath() + Path::versionPath()), stream);
    // It is just one line of code, so I don't see the need to loop
    std::getline(stream, line);
    if(line.compare(0, name.size(), name) == 0){
        istringstream buf(line);
        istream_iterator<string> beg(buf), end;
        vector<string> values(beg, end);
        return values[2];
    }
    
}

std::string ProcessParser::getOSName(){
    string line;
    string name = "PRETTY_NAME=";

    string Osname = "";
    string OsVersion = "";
    string OsType = "";
    string OsFullname = "";

    ifstream stream;
    Util::getStream(Path::osNamePath(), stream);
    // It is just one line of code, so I don't see the need to loop
    while(std::getline(stream, line)){
        if(line.compare(0, name.size(), name) == 0){
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            Osname =  values[0].substr(values[0].find("U"), values[0].size());
            OsVersion = values[1];
            OsType = values[2].substr(0, values[2].size()-1);
            return (Osname+" "+OsVersion+" "+OsType);
        }

    }
    

}

int ProcessParser::getTotalNumberOfProcesses()
{
    string line;
    int result = 0;
    string name = "processes";
    ifstream stream;
    Util::getStream((Path::basePath() + Path::statPath()), stream);
    while (std::getline(stream, line)) {
        if (line.compare(0, name.size(), name) == 0) {
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            result += stoi(values[1]);
            break;
        }
    }
    return result;
}

int ProcessParser::getNumberOfRunningProcesses()
{
    string line;
    int result = 0;
    string name = "procs_running";
    ifstream stream;
    Util::getStream((Path::basePath() + Path::statPath()), stream);
    while (std::getline(stream, line)) {
        if (line.compare(0, name.size(), name) == 0) {
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            result += stoi(values[1]);
            break;
        }
    }
    return result;
}

std::vector<string> ProcessParser::getSysCpuPercent(string coreNumber)
{
    string line;
    string name = "cpu"+coreNumber;
    ifstream stream;
    Util::getStream((Path::basePath() + Path::statPath()), stream);
    while(std::getline(stream, line)){
        if(line.compare(0, name.size(), name) == 0){
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            return values;
        }
    }

}

int ProcessParser::getTotalThreads(){
    string line;
    int result =0;
    string name = "Threads:";
    vector<string>_list = ProcessParser::getPidList();
    for (int i=0; i<_list.size(); i++){
        string pid = _list[i];
        ifstream stream;
      	Util::getStream((Path::basePath() + pid + Path::statusPath()), stream);
        while (std::getline(stream, line)){
            if(line.compare(0, name.size(), name) == 0){
                istringstream buf(line);
                istream_iterator<string> beg(buf), end;
                vector<string> values(beg, end);
                result += stoi(values[1]);
            }
        }
    }
    return result;
}

bool ProcessParser::isPidExisting(string pid){

    bool result = false;
    vector<string>_list = ProcessParser::getPidList();

    for (int i=0; i<_list.size(); i++){
        if(pid == _list[i]){
            result = true;
            break;
        }
    }

    return result;
}