#ifndef C_CODE_GENERATOR_H
#define C_CODE_GENERATOR_H
#include "blocks/block_visitor.h"
#include "blocks/stmt.h"
#include "util/printer.h"
#include <unordered_set>
#include <unordered_map>

namespace block {
class c_code_generator: public block_visitor {
private:
	void emit_binary_expr(binary_expr::Ptr, std::string);
public:
	c_code_generator(std::ostream &_oss): oss(_oss) {}
	std::ostream &oss;
	int curr_indent = 0;
	virtual void visit(not_expr::Ptr);
	virtual void visit(and_expr::Ptr);
	virtual void visit(or_expr::Ptr);
	virtual void visit(plus_expr::Ptr);
	virtual void visit(minus_expr::Ptr);
	virtual void visit(mul_expr::Ptr);
	virtual void visit(div_expr::Ptr);
	virtual void visit(lt_expr::Ptr);
	virtual void visit(gt_expr::Ptr);
	virtual void visit(lte_expr::Ptr);
	virtual void visit(gte_expr::Ptr);
	virtual void visit(equals_expr::Ptr);
	virtual void visit(ne_expr::Ptr);
	virtual void visit(var_expr::Ptr);
	virtual void visit(int_const::Ptr);
	virtual void visit(assign_expr::Ptr);
	virtual void visit(expr_stmt::Ptr);
	virtual void visit(stmt_block::Ptr);
	virtual void visit(decl_stmt::Ptr);
	virtual void visit(if_stmt::Ptr);
	virtual void visit(while_stmt::Ptr);
	virtual void visit(break_stmt::Ptr);
	virtual void visit(var::Ptr);
	virtual void visit(scalar_type::Ptr);
	virtual void visit(pointer_type::Ptr);

};
}
#endif
