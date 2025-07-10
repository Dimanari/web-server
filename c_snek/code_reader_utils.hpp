#pragma once
#include <string>
#include <vector>
#include "code_reader.hpp"
namespace dimanari
{
	std::string readFile(const char* path);
	std::string dirnameOf(const std::string& fname);
	void AddVariable(std::string name, std::string value, Vector<Token>& tokens, Vector<Symbol>& symbols, Stack* _stack);
	void AddVariables(const std::vector<std::string>& strings, Vector<Token>& tokens, Vector<Symbol>& symbols, Stack* _stack);
	void AddPointerToken(const std::string& name, size_t value, Vector<Symbol>& symbols);
	void PrintToken(int i, FILE* pfile_out, Token tok, const Vector<Symbol>& symbols);
	std::string Preprocessor(const std::string& code, const std::string& root = "");

	void AddSyscalls(Vector<Symbol>& symbols, Vector<Function>& calls);
	void CompileAndRun(const char* path, const std::vector<std::string>& external_vars);
	void CompileStringAndRun(const char* code, const char* root, const std::vector<std::string>& external_vars);
}