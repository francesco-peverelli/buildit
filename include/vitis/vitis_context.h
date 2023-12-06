#ifndef VITIS_BUILDER_CONTEXT_H
#define VITIS_BUILDER_CONTEXT_H

#include<string>
#include "builder/builder.h"
#include "builder/builder_context.h"
#include "builder/dyn_var.h"
#include "builder/static_var.h"

namespace vitis {

enum class ctxt_phase {
    dataflow ,
    kernel,
    host
};

class vitis_context {
private:
    int num_decl_streams;
    ctxt_phase phase;
    bool gen_phase_pragmas;
public:

    vitis_context() {
        num_decl_streams = 0;
        gen_phase_pragmas = false;
    }
    std::string getNextStreamName() {
        int stream_id = num_decl_streams++;
        return "stream" + std::to_string(stream_id); 
    }
    void resetStreamDeclCount(){ num_decl_streams = 0; }
    void setPhase(ctxt_phase p) { 
        phase=p; 
        if(phase == ctxt_phase::dataflow){
            gen_phase_pragmas = true;
        } 
    }
    bool phasePragmasToGen(){
        return gen_phase_pragmas;
    }
    void phasePragmasDone(){
        gen_phase_pragmas = false;
    }
    ctxt_phase getPhase() { return phase; }
    
};
} // vitis namespace

#endif // VITIS_BUILDER_CONTEXT_H