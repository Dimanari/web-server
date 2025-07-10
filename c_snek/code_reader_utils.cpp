#include <chrono>
#include "code_reader_utils.hpp"
using std::string;

#ifdef FALSE
#define _PRINT(...) printf(__VA_ARGS__); if(pfile_out) fprintf(pfile_out, __VA_ARGS__);
#else
#define _PRINT(...) if(pfile_out) fprintf(pfile_out, __VA_ARGS__);
#endif

namespace dimanari
{
	string readFile(const char* path)
	{
		FILE* pfile;
		std::string content = "";
		int character;
		int err = fopen_s(&pfile, path, "rt");
		if (nullptr == pfile)
			return content;

		while (EOF != (character = fgetc(pfile)))
		{
			content += char(character);
		}

		fclose(pfile);

		return content;
	}

	string dirnameOf(const string& fname)
	{
		size_t pos = fname.find_last_of("\\/");
		return (string::npos == pos)
			? ""
			: fname.substr(0, pos) + "\\";
	}
	void AddVariable(std::string name, std::string value, Vector<Token>& tokens, Vector<Symbol>& symbols, Stack* _stack)
	{

		Symbol sym;
		char* string_ptr = (char*)_stack->Allocate(value.size() + 1);
		strncpy_s(string_ptr, (size_t)value.size() + 1, value.c_str(), (size_t)value.size() + 1);
		sym.sym_type = SYM_LITERAL;
		strncpy_s(sym.name, name.c_str(), sizeof(sym.name));
		sym.symbol_value = (size_t)string_ptr - (size_t)_stack->RAW();

		symbols.PushBack(sym);
	}

	void AddVariables(const std::vector<string>& strings, Vector<Token>& tokens, Vector<Symbol>& symbols, Stack* _stack)
	{
		for (int i = 0; i < strings.size(); ++i)
		{
			string var_name, var_value;
			bool is_value = false;
			const char* str = strings[i].c_str();
			while (*str)
			{
				if (!is_value && '=' == *str)
				{
					is_value = true;
				}
				else
				{
					if (is_value)
						var_value += *str;
					else
						var_name += *str;
				}
				str++;
			}

			AddVariable(var_name, var_value, tokens, symbols, _stack);
		}
	}

	void AddPointerToken(const std::string& name, size_t value, Vector<Symbol>& symbols)
	{
		Symbol sym;
		strncpy_s(sym.name, name.c_str(), sizeof(sym.name));
		sym.symbol_value = value;
		sym.sym_type = SYM_SYS_ADDRESS;

		symbols.PushBack(sym);
	}


	void PrintToken(int i, FILE* pfile_out, Token tok, const Vector<Symbol>& symbols)
	{
		const char* tok_type_strings[] =
		{
			"TOK_NUMBER", "TOK_FLOAT", "TOK_OPERATOR", "TOK_SYMBOL", "TOK_BLOCK", "TOK_MAX"
		};
		_PRINT("%d, Token type %s, val %d", i, tok_type_strings[tok.type], (int)tok.symbol_value);
		if (tok.type == TOK_SYMBOL)
		{
			_PRINT("<%s>", symbols[tok.symbol_value].name);
		}
		if (tok.type == TOK_OPERATOR)
		{
			extern Operators oprands[];
			_PRINT("<%s>", oprands[tok.symbol_value].opp);
		}
		_PRINT("\n");
	}

	string Preprocessor(const string& code, const string& root)
	{
		string processed_code;
		processed_code.reserve(code.size());
		int index = 0;
		bool is_mono_line_comment = false;
		bool is_multi_line_comment = false;
		while (index < code.size())
		{
			char now = code[index++];

			if ('/' == now)
			{
				if (is_multi_line_comment)
				{
					if ('*' == code[index - 2])
						is_multi_line_comment = false;
					continue;
				}
				else if ('/' == code[index])
				{
					is_mono_line_comment = true;
				}
				else if ('*' == code[index])
				{
					is_multi_line_comment = true;
				}
			}
			if (is_mono_line_comment && '\n' == now)
			{
				is_mono_line_comment = false;
			}
			if (is_mono_line_comment || is_multi_line_comment)
				;
			else
				processed_code += now;
		}
		return processed_code;
	}

	extern const char* default_code;

	void CompileAndRun(const char* path, const std::vector<std::string>& external_vars)
	{
		string g_code = default_code;
		string root = "";

		if (path)
		{
			g_code = readFile(path);
			root = dirnameOf(path);
		}

		CompileStringAndRun(g_code.c_str(), root.c_str(), external_vars);
	}

	void CompileStringAndRun(const char* code, const char* root, const std::vector<std::string>& external_vars)
	{
		string g_code = default_code;
		FILE* pfile_out = nullptr;
		bool output = true;
		int err = fopen_s(&pfile_out, "interp.log", "wt");

		if (nullptr == pfile_out)
			output = false;

		if (!root)
			root = "";
		g_code = Preprocessor(g_code, root);

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		Vector<Token> tokens;
		Vector<Vector<Token>> token_instruction;
		Vector<Symbol> symbols;
		Vector<Function> calls;
		Vector<Instruction> program;

		Vector<size_t> stack_base;
		// Get String variables from post/get request
		AddKeywords(symbols);

		Stack my_stack;
		my_stack.Init(5 * MUL_KB * MUL_KB);

		AddVariables(external_vars, tokens, symbols, &my_stack);

		AddSyscalls(symbols, calls);

		TokenizeLine(g_code.c_str(), tokens, symbols, &my_stack);

#ifdef _DEBUG
		_PRINT("Tokens RAW\n");
		for (int i = 0; i < tokens.Size(); ++i)
		{
			PrintToken(i, pfile_out, tokens[i], symbols);
		}
#endif
		SplitTokens(tokens, token_instruction);

#ifdef _DEBUG
		_PRINT("\nToken Lines\n");

		for (int j = 0; j < token_instruction.Size(); ++j)
		{
			_PRINT("TokenLine %d\n", j);
			for (int i = 0; i < token_instruction[j].Size(); ++i)
			{
				PrintToken(i, pfile_out, token_instruction[j][i], symbols);
			}
		}
#endif

		ParseProgram(program, symbols, &my_stack, token_instruction, calls);

		size_t next = 0;
		int token_index = 0;
		stack_base.PushBack(my_stack.Size());


		_PRINT("generated %d tokens and %d symbols\nallocated %d bytes on the stack\n", (int)tokens.Size(), (int)symbols.Size(), (int)my_stack.Size());
#ifdef _DEBUG
		for (int i = 0; i < symbols.Size(); ++i)
		{
			const char* sym_type_strings[] =
			{
				"SYM_LOCAL_VAR", "SYM_GLOBAL_VAR", "SYM_FUNCTION", "SYM_LITERAL", "SYM_KEYWORD", "SYM_MAX"
			};
			_PRINT("Sym(%d): %s :: %s,%d\n", i, symbols[i].name, sym_type_strings[symbols[i].sym_type], (int)symbols[i].symbol_value);
			if (symbols[i].sym_type == SYM_LITERAL)
			{
				_PRINT("Holding string:\n%s\nEOS\n", ((char*)my_stack.RAW() + symbols[i].symbol_value));
			}
		}

		_PRINT("\nProgram:\n");

		for (int j = 0; j < program.Size(); ++j)
		{
			PrintToken(j, pfile_out, program[j].token_instruct, symbols);
			_PRINT("Next %d\n", (int)program[j].next);
			if (program[j].num_param)
			{
				_PRINT("params: ");
				for (int par = 0; par < program[j].num_param; ++par)
				{
					_PRINT("%s%d ", (par ? "," : ""), (int)program[j].params[par]);
				}
				_PRINT("\n");
			}
		}
#endif

		_PRINT("\nCompiled Code:\n");
		_PRINT(g_code.c_str());
		_PRINT("\nProgram Init\n");
		do
		{
			bool was_return_called = false;
			Token local = RunInstruction(program, calls, symbols, &my_stack, next, stack_base, was_return_called);
			if (TOK_SYMBOL == local.type)
				local = ResolveToken(local, &my_stack, symbols, stack_base);
#ifdef _DEBUG
			PrintToken(token_index++, pfile_out, local, symbols);
#endif
		} while (0 != next);

		_PRINT("\nAttempting to run main:\n");
		int main_found = -1;
		for (int i = 0; i < symbols.Size(); ++i)
		{
			if (0 == strcmp("main", symbols[i].name))
			{
				main_found = i;
				break;
			}
		}
		if (-1 == main_found)
		{
			_PRINT("ERROR: no main symbol found");
		}
		else if (SYM_FUNCTION != symbols[main_found].sym_type)
		{
			_PRINT("ERROR: no main symbol is not a function");
		}
		else
		{
			size_t call_to = calls[symbols[main_found].symbol_value].start_instruction;
			RunFunctionCall(program, calls, symbols, &my_stack, call_to, stack_base);
		}
		auto stop = timer.now();
		using us = std::chrono::duration<float, std::micro>;
		float deltaTime = std::chrono::duration_cast<us>(stop - start).count();
		_PRINT("Operation took %.1f microseconds\n", deltaTime);


		_PRINT("Closing log file\n");
		if (pfile_out)
			fclose(pfile_out);
		pfile_out = nullptr;
		//execute(g_code, root);
		//system("pause");

	}


	const char* default_code = "\n\
var global_var1 = 32; // commenting about life\n\
var global_string = \"this string is awesome\";/*\n\
func add(var a, var b) return a+b;\n\
int var3 */\n\
func main()\n\
{\n\
	var local_var1 = 3;\n\
	var local_var2 = 6 + global_var1;\n\
	\n\
	print(\"print_works with literals\");\n\
	if(local_var1 + local_var1)\n\
	{\n\
		print(\"if_works\");\n\
		print(\" with multilines\\n\");\n\
	}\n\
	else\n\
	{\n\
		print(\"else_works\");\n\
		print(\" with multilines\\n\");\n\
	}\n\
	\n\
	while(local_var2)\n\
	{\n\
		local_var2 = local_var2 - 1;\n\
		print(\"a while a go\");\n\
		print(local_var2);\n\
	}\n\
	local_var2 = \"my string literally\\nsays so\";\n\
	print(local_var2);\n\
	if(strcmp(\"loppunny\", username)) ; else if(strcmp(\"s3xl1fe4ever\", password)) ; else print(\"logging in loppunny\");\n\
	return local_var2;\n\
}";
}