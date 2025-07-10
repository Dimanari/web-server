#include <inet/http_parser.hpp>
#include <database/query.hpp>
#include "code_reader_utils.hpp"
namespace dimanari
{
	Token SysPrintClean(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens); // print
	Token SysPrintString(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens);// sprint
	Token SysStringSet(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens);// sprint
	Token SysStrCmp(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens);	 // strcmp
	Token SysSetCookie(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens);	 // cookie
	Token SysDatabaseQuery(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens);	 // cookie
	Token SysLoadResult(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens);	 // cookie

	void AddSyscalls(Vector<Symbol>& symbols, Vector<Function>& calls)
	{
		AddSyscall(SysPrintString, symbols, calls, "print");
		AddSyscall(SysStringSet, symbols, calls, "sprint");
		AddSyscall(SysStrCmp, symbols, calls, "strcmp");
		AddSyscall(SysSetCookie, symbols, calls, "set_cookie");
		AddSyscall(SysDatabaseQuery, symbols, calls, "query");
		AddSyscall(SysLoadResult, symbols, calls, "get_query");
	}


	Token SysPrint(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		constexpr const char* tok_type_strings[] =
		{
			"TOK_NUMBER", "TOK_FLOAT", "TOK_OPERATOR", "TOK_SYMBOL", "TOK_MAX"
		};
		printf("Syscall: Token type %s, val %d", tok_type_strings[tokens->type], (int)tokens->symbol_value);
		if (tokens->type == TOK_SYMBOL)
		{
			Symbol sym = symbols[tokens->symbol_value];
			printf("<%s>", sym.name);
			if (SYM_LITERAL == sym.sym_type)
			{
				printf("<%s>", ((char*)_stack->RAW() + sym.symbol_value));
			}
		}
		if (tokens->type == TOK_OPERATOR)
		{
			extern Operators oprands[];
			printf("<%s>", oprands[tokens->symbol_value].opp);
		}
		printf("\n");
		return *tokens;
	}

	inline std::string TokenString(Token tok, const Vector<Symbol>& symbols, Stack* _stack)
	{
		std::string string_out = "";
		if (tok.type == TOK_SYMBOL)
		{
			Symbol sym = symbols[tok.symbol_value];
			if (SYM_SYS_ADDRESS == sym.sym_type)
			{
				std::string* sym_string = (std::string*)(sym.symbol_value);
				string_out += *sym_string;
			}
			else if (SYM_LITERAL == sym.sym_type)
				string_out += ((char*)_stack->RAW() + sym.symbol_value);
			else
				string_out += sym.name;
		}
		else if (tok.type == TOK_OPERATOR)
		{
			extern Operators oprands[];
			string_out += oprands[tok.symbol_value].opp;
		}
		else if (tok.type == TOK_FLOAT)
			string_out += std::to_string(tok.float_value);
		else
			string_out += std::to_string(tok.symbol_value);

		return string_out;
	}

	Token SysPrintClean(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		Token* ret_tok = tokens;
		while (n_token--)
		{
			printf("%s", TokenString(*tokens, symbols, _stack).c_str());

			tokens++;
		}
		printf("\n");

		return *ret_tok;
	}

	Token SysStrCmp(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		std::string str1, str2;

		if (tokens->type == TOK_SYMBOL)
		{
			str1 = TokenString(*tokens, symbols, _stack);
		}
		tokens++;
		if (tokens->type == TOK_SYMBOL)
		{
			str2 = TokenString(*tokens, symbols, _stack);
		}
		tokens++;

		*tokens = Token(TOK_NUMBER, strcmp(str1.c_str(), str2.c_str()));
		return *tokens;
	}

	Token SysSetCookie(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		extern void AddCookie(std::vector<Cookie>&cookies, Cookie cookie);
		std::vector<Cookie>* cookies = nullptr;
		if (TOK_SYMBOL == tokens->type && SYM_SYS_ADDRESS == symbols[tokens->symbol_value].sym_type)
		{
			cookies = (std::vector<Cookie>*)(symbols[tokens->symbol_value].symbol_value);
			tokens++;
			n_token--;
		}
		Token* ret_tok = tokens;
		std::string name = "";
		std::string value = "";
		if (n_token > 1)
		{
			name = TokenString(*tokens, symbols, _stack);

			tokens++;
			n_token--;
		}
		if (n_token > 1)
		{
			value = TokenString(*tokens, symbols, _stack);

			tokens++;
			n_token--;
		}

		if (nullptr != cookies && name != "")
		{
			Cookie cookie;
			cookie.name = name;
			if (value != "")
			{
				cookie.value = value;
				cookie.status = CS_REFRESH;
			}
			else
			{
				cookie.value = value;
				cookie.status = CS_REMOVE;
			}
			AddCookie(*cookies, cookie);
		}

		return *ret_tok;
	}

	Token SysDatabaseQuery(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		std::string my_database = "default_db";
		std::vector<std::string>* my_responses = nullptr;
		std::string string_out = "";
		if (TOK_SYMBOL == tokens->type && SYM_SYS_ADDRESS == symbols[tokens->symbol_value].sym_type)
		{
			my_responses = (std::vector<std::string>*)(symbols[tokens->symbol_value].symbol_value);
			tokens++;
			n_token--;
		}
		else
		{
			return Token(TOK_MAX);
		}

		while (n_token--)
		{
			string_out += TokenString(*tokens, symbols, _stack);
			tokens++;
		}

		QueryParse::Query(my_database, string_out, *my_responses);

		return Token(TOK_NUMBER, my_responses->size());
	}

	Token SysLoadResult(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		std::vector<std::string>* my_responses = nullptr;
		std::string* my_target = nullptr;
		if (0 < n_token && TOK_SYMBOL == tokens->type && SYM_SYS_ADDRESS == symbols[tokens->symbol_value].sym_type)
		{
			my_responses = (std::vector<std::string>*)(symbols[tokens->symbol_value].symbol_value);
			tokens++;
			n_token--;
		}
		else
		{
			return Token(TOK_MAX, -1);
		}

		if (0 < n_token && TOK_SYMBOL == tokens->type && SYM_SYS_ADDRESS == symbols[tokens->symbol_value].sym_type)
		{
			my_target = (std::string*)(symbols[tokens->symbol_value].symbol_value);
			tokens++;
			n_token--;
		}
		else
		{
			return Token(TOK_MAX, -1);
		}

		if(0 == n_token || (nullptr == my_target) || (nullptr == my_responses))
			return Token(TOK_MAX, -1);
		Token tok = ResolveToken(*tokens, _stack, symbols, stack_base);

		if(TOK_NUMBER == tok.type && tok.symbol_value < my_responses->size());
		{
			*my_target = (*my_responses)[tok.symbol_value];
			return tok;
		}

		return Token(TOK_MAX, -1);
	}

	Token SysPrintString(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		Token* tok_org = tokens;
		std::string string_out = "";
		std::string* my_string = nullptr;
		if (TOK_SYMBOL == tokens->type && SYM_SYS_ADDRESS == symbols[tokens->symbol_value].sym_type)
		{
			my_string = (std::string*)(symbols[tokens->symbol_value].symbol_value);
			tokens++;
			n_token--;
		}
		Token* ret_tok = tokens;
		while (n_token--)
		{
			string_out += TokenString(*tokens, symbols, _stack);
			tokens++;
		}
		if (my_string)
			*my_string += string_out;
		else
			printf("%s", string_out.c_str());

		return *ret_tok;
	}

	Token SysStringSet(Stack* _stack, Vector<size_t>& stack_base, Vector<Symbol>& symbols, int n_token, Token* tokens)
	{
		Token* tok_org = tokens;
		std::string string_out = "";
		std::string* my_string = nullptr;
		if (TOK_SYMBOL == tokens->type && SYM_SYS_ADDRESS == symbols[tokens->symbol_value].sym_type)
		{
			my_string = (std::string*)(symbols[tokens->symbol_value].symbol_value);
			tokens++;
			n_token--;
		}
		Token* ret_tok = tokens;
		while (n_token--)
		{
			string_out += TokenString(*tokens, symbols, _stack);
			tokens++;
		}
		if (my_string)
			*my_string = string_out;
		else
			printf("%s", string_out.c_str());

		return *ret_tok;
	}
}