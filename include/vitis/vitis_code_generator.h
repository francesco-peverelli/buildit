#ifndef VITIS_CODE_GENERATOR_H
#define VITIS_CODE_GENERATOR_H

#include "blocks/c_code_generator.h"
#include "vitis/vitis_metadata.h"

namespace vitis {

    class hls_code_generator: public block::c_code_generator {
	using block::c_code_generator::visit;
	using block::c_code_generator::c_code_generator;

	virtual void visit(block::decl_stmt::Ptr);
	virtual void visit(block::expr_stmt::Ptr);

	void expand_pragmas(block::decl_stmt::Ptr s);
	void expand_pragmas(block::expr_stmt::Ptr s);
	void generate_user_annotation(block::decl_stmt::Ptr s);
	void generate_user_annotation(block::expr_stmt::Ptr s);

public:
	static void generate_code(block::block::Ptr ast, std::ostream &oss, int indent = 0) {
		hls_code_generator generator(oss);
		generator.curr_indent = indent;
		ast->accept(&generator);
		oss << std::endl;
	}
};
} // namespace vitis
#endif // VITIS_CODE_GENERATOR_H