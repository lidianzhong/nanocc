#include <emscripten/bind.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <cstdio>

#include "nanocc/frontend/AST.h"
#include "nanocc/frontend/DumpVisitor.h"
#include "nanocc/ir/IRGenVisitor.h"
#include "nanocc/ir/IRSerializer.h"
#include "nanocc/ir/Module.h"
#include "nanocc/backend/CodeGen.h"

// Flex API
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern int yyparse(std::unique_ptr<nanocc::BaseAST> &ast);
extern int yylex_destroy(void);

using namespace emscripten;

std::string json_escape(const std::string& s) {
    std::string output;
    for (char c : s) {
        switch (c) {
            case '\"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    output += "\\u00";
                    char hex[3];
                    snprintf(hex, sizeof(hex), "%02x", c);
                    output += hex;
                } else {
                    output += c;
                }
        }
    }
    return output;
}

std::string compile(std::string source_code, std::string mode) {
    if (source_code.empty()) return "{}";

    // 1. Parse
    YY_BUFFER_STATE buffer = yy_scan_string(source_code.c_str());
    std::unique_ptr<nanocc::BaseAST> ast;
    int res = yyparse(ast);
    yy_delete_buffer(buffer);

    if (res != 0 || !ast) {
        return "{\"error\": \"Parse failed\"}";
    }

    // Capture AST
    std::string ast_output;
    {
        nanocc::DumpVisitor dumper("/dump.ast");
        ast->Accept(dumper);
    } // dumper closes file on destruction? Hopefully. If not, DumpVisitor usually flushes in dtor.
    
    std::ifstream ifs("/dump.ast");
    if (ifs) {
        ast_output.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    }
    remove("/dump.ast");


    // 2. IR Gen
    nanocc::Module module;
    try {
        nanocc::IRGenVisitor irgen(module);
        ast->Accept(irgen);
    } catch (std::exception &e) {
        return std::string("{\"error\": \"IR Gen Error: ") + json_escape(e.what()) + "\"}";
    }

    // 3. Output
    std::string code_output;
    bool is_error = false;

    if (mode == "-koopa") {
        code_output = nanocc::IRSerializer::ToIR(module);
    } else if (mode == "-riscv") {
        std::stringstream ss;
        std::streambuf *old_cout = std::cout.rdbuf(ss.rdbuf());
        
        try {
            ProgramCodeGen codegen;
            codegen.Emit(nanocc::IRSerializer::ToProgram(module));
        } catch (std::exception &e) {
            code_output = std::string("Error during RISC-V generation: ") + e.what();
            is_error = true;
        }
        
        std::cout.rdbuf(old_cout);
        if (!is_error) code_output = ss.str();
    } else {
        return "{\"error\": \"Unknown mode\"}";
    }

    std::stringstream json;
    json << "{";
    if (is_error) {
        json << "\"error\": \"" << json_escape(code_output) << "\",";
    } else {
        json << "\"code\": \"" << json_escape(code_output) << "\",";
    }
    json << "\"ast\": \"" << json_escape(ast_output) << "\"";
    json << "}";

    return json.str();
}

EMSCRIPTEN_BINDINGS(sysy_compiler) {
    function("compile", &compile);
}
