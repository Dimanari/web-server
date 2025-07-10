#pragma once
#include <stdint.h>
#include "containers.hpp"
#define MAX_CHILD 5
namespace dimanari
{
	enum TokType_e
	{
		TOK_NUMBER, TOK_FLOAT, TOK_OPERATOR, TOK_SYMBOL, TOK_BLOCK, TOK_MAX
	};

	enum SymType_e
	{
		SYM_LOCAL_VAR, SYM_GLOBAL_VAR, SYM_FUNCTION, SYM_LITERAL, SYM_KEYWORD, SYM_SYS_ADDRESS, SYM_MAX
	};

	enum KeywordType_e
	{
		KEY_INT, KEY_FUNC, KEY_IF, KEY_WHILE, KEY_REF, KEY_DEREF, KEY_CALL, KEY_RETURN, KEY_ELSE, KEY_MAX
	};

	enum OperatorType_e
	{
		OPP_ASSIGN, OPP_ADD, OPP_SUB, OPP_MUL, OPP_DIV, OPP_BLOCK_START, OPP_BLOCK_END, OPP_BRACKET_OPEN, OPP_BRACKET_CLOSE,
		OPP_LINE_END, OPP_LINE_END_SOFT, OPP_CALL_FUNC, OPP_RETURN_FUNC, OPP_IF, OPP_WHILE,
		OPP_COMP_EQ, OPP_COMP_NEQ, OPP_COMP_GT, OPP_COMP_LT, OPP_COMP_GTE, OPP_COMP_LTE, OPP_REF_TOKEN, OPP_DEREF_TOKEN,
		OPP_MAX
	};


	struct Token
	{
		TokType_e type;
		int64_t symbol_value;
		double float_value;

		Token(TokType_e _type = TOK_MAX, int64_t _symbol_value = 0, double _float_value = 0) : type(_type), symbol_value(_symbol_value), float_value(_float_value) {}
	};

	struct binding_power
	{
		int lbp;
		int rbp;
	};

	struct Symbol
	{
		int64_t symbol_value;
		int sym_type;
		char name[60];
		const Symbol& operator=(const Symbol& other)
		{
			memcpy(this, &other, sizeof(Symbol));
			return *this;
		}

		Symbol()
		{
			memset(this, 0, sizeof(Symbol));
		}

		Symbol(const Symbol& other)
		{
			memcpy(this, &other, sizeof(Symbol));
		}
	};

	struct Operators
	{
		char opp[8];
		int func;
		binding_power powers;
	};

	struct Instruction
	{
		Token token_instruct;
		Token token_result;

		int num_param;

		size_t next;
		size_t params[MAX_CHILD];

		Instruction() :
			token_instruct(Token{ TOK_MAX }),
			token_result(Token{ TOK_MAX }),
			num_param(0),
			next(0),
			params{ 0 }
		{
		}
	};

	struct TokenReplacement
	{
		int64_t source_symbol_value;
		int64_t local_symbol_value;
	};

	struct Function
	{
		int stack_size;
		int param_amount;
		int is_syscall;
		int is_token_call;
		size_t start_instruction;
		Vector<TokenReplacement> local_tokens;

		Function() : stack_size(0), param_amount(0), is_syscall(0), is_token_call(0), start_instruction(0), local_tokens() {}
	};

	void AddKeywords(Vector<Symbol>& symbols);
	// 
	void TokenizeLine(const char* line, Vector<Token>& LineTokens, Vector<Symbol>& symbols, Stack* _stack);

	void SplitTokens(Vector<Token>& LineTokens, Vector<Vector<Token>>& TokenTokens);

	size_t ParseLine(Vector<Token>& current_line, size_t& index, size_t last_inst, Vector<Symbol>& symbols, Vector<Function>& calls, Vector<Instruction>& program, size_t function_base, Stack* _stack, int binding_power);

	void ParseProgram(Vector<Instruction>& program, Vector<Symbol>& symbols, Stack* _stack, Vector<Vector<Token>>& TokenTokens, Vector<Function>& calls);

	Token RunInstruction(Vector<Instruction>& program, Vector<Function>& calls, Vector<Symbol>& symbols, Stack* _stack, size_t& instruction, Vector<size_t>& stack_base, bool& was_return_called);

	Token& ResolveToken(Token& target, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base);

	Token RunFunctionCall(Vector<Instruction>& program, Vector<Function>& calls, Vector<Symbol>& symbols, Stack* _stack, size_t instruction, Vector<size_t>& stack_base);

	typedef Token(*SysCall_f)(Stack*, Vector<size_t>&, Vector<Symbol>&, int, Token*);

	void AddSyscall(SysCall_f _syscall, Vector<Symbol>& symbols, Vector<Function>& calls, const char* name);

}
// call from inside itself for function calls

// seperate code to lines

// make each line into Token Tree

// run token list
