#ifndef VITIS_METADATA_H
#define VITIS_METADATA_H

#include <string>
#include <map>

namespace vitis{

class hls_pragma {
        std::string name;
        std::map<std::string, std::string> optMap;
    public:
        hls_pragma(std::string n) : name(n) {}
        void setOption(std::string optName, std::string optVal){
            if(optMap.count(optName)){
                optMap[optName] = optVal;
            } else {
                optMap.insert(std::make_pair(optName, optVal));
            }
        } 
        std::string getOption(std::string optName) {
            if(optMap.count(optName)) return optMap[optName];
            else return "";
        }
        std::string getFullPragma() {
            std::string pragmaStr = "#pragma HLS " + name;
            for(auto opt: optMap){
                if(!opt.second.size())
                    pragmaStr += " " +  opt.first;
                else
                    pragmaStr += " " +  opt.first + " = " + opt.second;
            }
            return pragmaStr;
        }
};

} // vitis namespace
#endif // VITIS_METADATA_H