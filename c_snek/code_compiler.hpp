#pragma once
#include "code_reader_utils.hpp"
namespace dimanari
{
	struct Syscall_Arr
	{
		const char* _name;
		SysCall_f _syscall;
	};

	class Compiler
	{
	private:
		// simple structs - only destructor
		Vector<Token> m_tokens;
		Vector<Symbol> m_symbols;
		Vector<Instruction> m_program;
		// complex structs - manual deallocation
		Vector<Vector<Token>> m_token_instruction;
		Vector<Function> m_calls;
		Stack m_stack;
	public:
		// call before using class
		void Init();

		// call before destroying class
		void Clean();

		// add system functions
		void AddSyscall(const Vector<Syscall_Arr>& _call);
		void AddSyscall(Syscall_Arr _call);
		void AddSyscall(const char* _name, SysCall_f _syscall);

		// add default system functions
		void AddDefSyscalls();

		// add external objects - like the string extern used by sprintf
		void AddExternHandler(const char* _name, size_t _addr);

		void AddVaraibles(const std::vector<std::string>& strings);
		void AddVaraible(const std::string& name, const std::string& value);

		void CompileCode(const std::string& code, const std::string& root = "");

		void Run(FILE* pfile = nullptr);

		void DebugViewer(FILE* pfile = nullptr);
	};
}

/*
 * use order for class
 * Init(includes adding keywords and setting up Stack)
 * AddDefSyscalls(and other Syscalls)
 * AddVaraibles
 * CompileCode
 * Run
 */