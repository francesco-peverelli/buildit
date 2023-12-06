#include "vitis/vitis_code_generator.h"

namespace vitis {

    void hls_code_generator::visit(block::decl_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
		replace_var_name(s->decl_var->var_name, s->annotation);
    	generate_user_annotation(block::to<block::stmt>(s), true);
    }
	void hls_code_generator::visit(block::expr_stmt::Ptr s){
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::for_stmt::Ptr s){
		if(s->annotation != ""){
            // @root@hls_pragma: dataflow#hls_pragma: pipeline off
            if(s->annotation.find("@root@") != std::string::npos){
                // If explicitly annotated as root, keep it at the top
                auto stripped = s->annotation.substr(6, s->annotation.size());
                auto delim_pos = stripped.find("#");
                // If annotations for the next level is also present, move it down
                if(delim_pos != std::string::npos){
                    s->annotation = stripped.substr(0, delim_pos);
                    parent_loop_annotation = stripped.substr(delim_pos+1, stripped.size());
                } else { 
                    s->annotation = stripped;
                }      
            } else {
                parent_loop_annotation = s->annotation;
                s->annotation = "";
            }
        }
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }

	void hls_code_generator::visit(block::if_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::label_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::goto_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::while_stmt::Ptr s) {
		if(s->annotation != ""){
            // @root@hls_pragma: dataflow#hls_pragma: pipeline off
            if(s->annotation.find("@root@") != std::string::npos){
                // If explicitly annotated as root, keep it at the top
                auto stripped = s->annotation.substr(6, s->annotation.size());
                auto delim_pos = stripped.find("#");
                // If annotations for the next level is also present, move it down
                if(delim_pos != std::string::npos){
                    s->annotation = stripped.substr(0, delim_pos);
                    parent_loop_annotation = stripped.substr(delim_pos+1, stripped.size());
                } else { 
                    s->annotation = stripped;
                }      
            } else {
                parent_loop_annotation = s->annotation;
                s->annotation = "";
            }
        }
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
	}
	void hls_code_generator::visit(block::break_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::func_decl::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::return_stmt::Ptr s) {
    	expand_pragmas(block::to<block::stmt>(s));
    	generate_user_annotation(block::to<block::stmt>(s));
    	block::c_code_generator::visit(s);
    }
	void hls_code_generator::visit(block::stmt_block::Ptr s) {
		oss << "{";
		curr_indent += 1;
		if(parent_loop_annotation != ""){
			printer::indent(oss, curr_indent);
			generate_user_annotation(parent_loop_annotation, true);
			parent_loop_annotation = "";
			oss << std::endl;
		} else {
			oss << std::endl;
		}
		for (auto stmt : s->stmts) {
			printer::indent(oss, curr_indent);
			stmt->accept(this);
			oss << std::endl;
		}
		curr_indent -= 1;
		printer::indent(oss, curr_indent);

		oss << "}";
	}
	void hls_code_generator::replace_var_name(std::string name, std::string& s){
		std::string::size_type n = 0;
		std::string placeholder = "$var$";
		while ( ( n = s.find( placeholder, n ) ) != std::string::npos )
		{
		    s.replace( n, placeholder.size(), name );
		    n += name.size();
		}
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
	void hls_code_generator::generate_user_annotation(std::string annotation_line, bool post) {
		std::string pragma_prefix("hls_pragma: ");
		if (!annotation_line.compare(0, pragma_prefix.size(), pragma_prefix)) {
			size_t pos = 0;
			std::string str = annotation_line.substr(pragma_prefix.size());
			std::string token;
			while((pos = str.find(',')) != std::string::npos){
				token = str.substr(0, pos);
				if(post){
					oss << std::endl;
					printer::indent(oss, curr_indent);
					oss << "#pragma HLS " << token;
				} else {
					oss << "#pragma HLS " << token << std::endl;
					printer::indent(oss, curr_indent);
				}
				str.erase(0, pos + 1);
			}
			if(post){
				oss << std::endl;
				printer::indent(oss, curr_indent);
				oss << "#pragma HLS " << str;
			} else {
				oss << "#pragma HLS " << str << std::endl;
				printer::indent(oss, curr_indent);
			}
		}
	}
	void hls_code_generator::generate_user_annotation(block::stmt::Ptr s, bool post) {
		std::string pragma_prefix("hls_pragma: ");
		std::string annotation_line = s->annotation;
		generate_user_annotation(annotation_line, post);
		s->annotation = "";
	}
}