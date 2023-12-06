#ifndef BLOCKS_EXTRACT_VITIS_H
#define BLOCKS_EXTRACT_VITIS_H

#include "blocks/annotation_finder.h"
#include "blocks/block.h"
#include "blocks/block_visitor.h"
#include "blocks/expr.h"
#include "blocks/stmt.h"
#include <algorithm>
#include <iostream>
#include <vector>

#define VITIS_KERNEL "kernel:vitis:auto"
#define VITIS_KERNEL_END "kernel:vitis:end"

namespace block {

void extract_vitis_from(block::Ptr from);
std::vector<var::Ptr> extract_function_args(block::Ptr function);
extern int vitis_device_mem_ports;

class gather_func_args : public block_visitor {
public:
	using block_visitor::visit;
	std::vector<var::Ptr> args;
	virtual void visit(func_decl::Ptr);
};

} // namespace block

#endif // BLOCKS_EXTRACT_VITIS_H