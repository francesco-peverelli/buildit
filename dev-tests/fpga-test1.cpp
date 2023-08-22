#include "blocks/c_code_generator.h"
#include "blocks/extract_cuda.h"
#include "vitis/extract_vitis.h"
#include "vitis/vitis_metadata.h"
#include "vitis/vitis_code_generator.h"
#include "builder/builder.h"
#include "builder/builder_context.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"
#include "blocks/rce.h"
#include <iostream>

using builder::dyn_var;
using builder::static_var;

// A simple vadd code with array types
void vadd(dyn_var<int*> in1, dyn_var<int*> in2, dyn_var<int*> out, dyn_var<int> size) {
	builder::annotate("pragma: pipeline,dataflow,dependence variable=buff_A type=inter false");
	dyn_var<int> i = 0;
	for(i = 0; i < size; i = i + 1){
		out[i] = in1[i] + in2[i];
	}
}

int main(int argc, char *argv[]) {
	builder::builder_context context;
	auto ast = context.extract_function_ast(vadd, "vadd");
	block::eliminate_redundant_vars(ast);
	block::vitis_device_mem_ports = 2;	
	block::extract_vitis_from(block::to<block::func_decl>(ast));
	ast->dump(std::cout, 0);
	std::cout << std::endl;
	vitis::hls_code_generator::generate_code(ast, std::cout, 0);
	return 0;
}
