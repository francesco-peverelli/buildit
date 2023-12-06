#include "vitis/extract_vitis.h"
#include "vitis/vitis_metadata.h"
#include "blocks/c_code_generator.h"
#include "builder/dyn_var.h"
#include <sstream>
#include <iostream>

namespace block {

std::vector<var::Ptr> extract_function_args(block::Ptr function) {
    // This visitor finds the arguments of the target function
    gather_func_args func_args;
    if(isa<func_decl>(function)){
        to<func_decl>(function)->accept(&func_args);
    } 

    return func_args.args;
}

void gather_func_args::visit(func_decl::Ptr stmt) {
	for (auto arg : stmt->args) {
		args.push_back(arg);
	}
}

void extract_vitis_from(block::Ptr from) {

    assert(isa<func_decl>(from));

    std::vector<var::Ptr> args;
    args = extract_function_args(from);

    func_decl::Ptr func = to<func_decl>(from);
    assert(isa<stmt_block>(func->body));
    stmt_block::Ptr body = to<stmt_block>(func->body);
    std::string interface_mtd = "interface";  
    std::vector<vitis::hls_pragma> interface_pragmas;
    int mem_port_idx = 0;
    for(auto arg: args){
        vitis::hls_pragma interface_pragma(interface_mtd);
        if(isa<pointer_type>(arg->var_type) || isa<array_type>(arg->var_type)) {
            interface_pragma.setOption("mode", "m_axi");
            interface_pragma.setOption("port", arg->var_name);
            auto bundle_number = std::to_string(mem_port_idx);
            interface_pragma.setOption("bundle", "gmem" + bundle_number);
        } 
        if(isa<reference_type>(arg->var_type)) {
            interface_pragma.setOption("mode", "s_axilite");
            interface_pragma.setOption("port", arg->var_name);
            interface_pragma.setOption("bundle", "control");
        } else if(isa<scalar_type>(arg->var_type)) {
            interface_pragma.setOption("mode", "s_axilite");
            interface_pragma.setOption("port", arg->var_name);
            interface_pragma.setOption("bundle", "control");
        } else if(isa<named_type>(arg->var_type)) {
            interface_pragma.setOption("mode", "s_axilite");
            interface_pragma.setOption("port", arg->var_name);
            interface_pragma.setOption("bundle", "control"); 
        }
        interface_pragmas.push_back(interface_pragma);
        mem_port_idx++;
    }
    vitis::hls_pragma ret_interface_pragma("interface");
    ret_interface_pragma.setOption("mode", "s_axilite");
    ret_interface_pragma.setOption("port", "return");
    ret_interface_pragma.setOption("bundle", "control");
    interface_pragmas.push_back(ret_interface_pragma);

    assert(body->stmts.size() > 0);
    body->stmts[0]->setMetadata<std::vector<vitis::hls_pragma>>("vitis_pragmas", interface_pragmas);
}

} // namespace block