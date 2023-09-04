#ifndef VITIS_BUILDER_CONTEXT_H
#define VITIS_BUILDER_CONTEXT_H

#include<string>

namespace vitis {

class vitis_context {
private:
    int num_decl_streams;
public:

    vitis_context() {
        num_decl_streams = 0;
    }
    std::string getNextStreamName() {
        int stream_id = num_decl_streams++;
        return "stream" + std::to_string(stream_id); 
    }
    void resetStreamDeclCount(){ num_decl_streams = 0; }
    
};
} // vitis namespace

#endif // VITIS_BUILDER_CONTEXT_H