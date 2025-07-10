#include "code_compiler.hpp"

#define V(...)

#ifdef FALSE
#define _PRINT(...) printf(__VA_ARGS__); if(pfile_out) fprintf(pfile_out, __VA_ARGS__);
#else
#define _PRINT(...) if(pfile_out) fprintf(pfile_out, __VA_ARGS__);
#endif
namespace dimanari
{
	void Compiler::Init()
	{
		m_token_instruction.ClearCplx();
		m_calls.ClearCplx();
		m_stack.Init(5 * MUL_KB * MUL_KB);

		m_symbols.Clear();
		AddKeywords(m_symbols);
	}

	void Compiler::Clean()
	{
		m_token_instruction.ClearCplx();
		m_calls.ClearCplx();
		m_stack.Release();

		m_symbols.Clear();
	}

	void Compiler::AddSyscall(const Vector<Syscall_Arr>& _call)
	{
		for (int i = 0; i < _call.Size(); ++i)
		{
			AddSyscall(_call[i]);
		}
	}

	void Compiler::AddSyscall(Syscall_Arr _call)
	{
		AddSyscall(_call._name, _call._syscall);
	}

	void Compiler::AddSyscall(const char* _name, SysCall_f _syscall)
	{
		dimanari::AddSyscall(_syscall, m_symbols, m_calls, _name);
	}

	void Compiler::AddDefSyscalls()
	{
		dimanari::AddSyscalls(m_symbols, m_calls);
	}

	void Compiler::AddExternHandler(const char* _name, size_t _addr)
	{
		dimanari::AddPointerToken(_name, _addr, m_symbols);
	}

	void Compiler::AddVaraibles(const std::vector<std::string>& strings)
	{
		dimanari::AddVariables(strings, m_tokens, m_symbols, &m_stack);
	}

	void Compiler::AddVaraible(const std::string& name, const std::string& value)
	{
		dimanari::AddVariable(name, value, m_tokens, m_symbols, &m_stack);
	}

	void Compiler::CompileCode(const std::string& code, const std::string& root)
	{
		std::string g_code = Preprocessor(code, root);

		TokenizeLine(g_code.c_str(), m_tokens, m_symbols, &m_stack);

		SplitTokens(m_tokens, m_token_instruction);

		ParseProgram(m_program, m_symbols, &m_stack, m_token_instruction, m_calls);

		// Run Global Code
		Vector<size_t> stack_base;
		size_t next = 0;
		stack_base.PushBack(m_stack.Size());
		do
		{
			bool was_return_called = false;
			Token local = RunInstruction(m_program, m_calls, m_symbols, &m_stack, next, stack_base, was_return_called);
		} while (0 != next);
	}

	void Compiler::Run(FILE* pfile_out)
	{
		Vector<size_t> stack_base;
		stack_base.PushBack(m_stack.Size());

		int main_found = -1;
		for (int i = 0; i < m_symbols.Size(); ++i)
		{
			if (0 == strcmp("main", m_symbols[i].name))
			{
				main_found = i;
				break;
			}
		}
		if (-1 == main_found)
		{
			_PRINT("ERROR: no main symbol found");
		}
		else if (SYM_FUNCTION != m_symbols[main_found].sym_type)
		{
			_PRINT("ERROR: no main symbol is not a function");
		}
		else
		{
			size_t call_to = m_calls[m_symbols[main_found].symbol_value].start_instruction;
			RunFunctionCall(m_program, m_calls, m_symbols, &m_stack, call_to, stack_base);
		}
	}

	void Compiler::DebugViewer(FILE* pfile_out)
	{
		_PRINT("DEBUG: SYMBOL LIST\n");
		for (int i = 0; i < m_symbols.Size(); ++i)
		{
			const char* sym_type_strings[] =
			{
				"SYM_LOCAL_VAR", "SYM_GLOBAL_VAR", "SYM_FUNCTION", "SYM_LITERAL", "SYM_KEYWORD", "SYM_MAX"
			};
			_PRINT("Sym(%d): %s :: %s,%d\n", i, m_symbols[i].name, sym_type_strings[m_symbols[i].sym_type], (int)m_symbols[i].symbol_value);
			if (m_symbols[i].sym_type == SYM_LITERAL)
			{
				_PRINT("Holding string:\n%s\nEOS\n", ((char*)m_stack.RAW() + m_symbols[i].symbol_value));
			}
		}
		_PRINT("DEBUG: RAW TOKEN LIST\n");

		for (int i = 0; i < m_tokens.Size(); ++i)
		{
			PrintToken(i, pfile_out, m_tokens[i], m_symbols);
		}

		_PRINT("DEBUG: PER LINE TOKEN LIST\n");
		for (int j = 0; j < m_token_instruction.Size(); ++j)
		{
			_PRINT("TokenLine %d\n", j);
			for (int i = 0; i < m_token_instruction[j].Size(); ++i)
			{
				PrintToken(i, pfile_out, m_token_instruction[j][i], m_symbols);
			}
		}

		_PRINT("generated %d tokens and %d symbols\nallocated %d bytes on the stack\n", (int)m_tokens.Size(), (int)m_symbols.Size(), (int)m_stack.Size());


		_PRINT("DEBUG: PROGRAM TREE\n");

		for (int j = 0; j < m_program.Size(); ++j)
		{
			PrintToken(j, pfile_out, m_program[j].token_instruct, m_symbols);
			_PRINT("Next %d\n", (int)m_program[j].next);
			if (m_program[j].num_param)
			{
				_PRINT("params: ");
				for (int par = 0; par < m_program[j].num_param; ++par)
				{
					_PRINT("%s%d ", (par ? "," : ""), (int)m_program[j].params[par]);
				}
				_PRINT("\n");
			}
		}
	}
}