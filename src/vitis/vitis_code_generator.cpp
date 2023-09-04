#include "vitis/vitis_code_generator.h"

namespace vitis {

    void hls_code_generator::visit(block::for_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
    void hls_code_generator::visit(block::decl_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::expr_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }

	void hls_code_generator::visit(block::if_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::label_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::goto_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::while_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
	}
	void hls_code_generator::visit(block::break_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::func_decl::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::return_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(s);
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::expand_pragmas(block::stmt::Ptr s) {
		if(s->hasMetadata<std::vector<vitis::hls_pragma>>("vitis_pragmas")){
			auto hls_pragmas = s->getMetadata<std::vector<vitis::hls_pragma>>("vitis_pragmas");
			for(auto p: hls_pragmas){
				oss << p.getFullPragma() << std::endl;
				printer::indent(oss, curr_indent);
			}
		}
	}
	void hls_code_generator::generate_user_annotation(block::stmt::Ptr s) {
		std::string pragma_prefix("hls_pragma: ");
		std::string annotation_line = s->annotation;
		if (!annotation_line.compare(0, pragma_prefix.size(), pragma_prefix)) {
			size_t pos = 0;
			std::string str = annotation_line.substr(pragma_prefix.size());
			std::string token;
			while((pos = str.find(',')) != std::string::npos){
				token = str.substr(0, pos);
				oss << "#pragma HLS " << token << std::endl;
				printer::indent(oss, curr_indent);
				str.erase(0, pos + 1);
			}
			oss << "#pragma HLS " << str << std::endl;
			printer::indent(oss, curr_indent);
			s->annotation = "";
		}
	}
}