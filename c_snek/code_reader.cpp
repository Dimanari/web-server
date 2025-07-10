#include <string>
#include "code_reader.hpp"
namespace dimanari
{
    const char* keyword[]
    {
        "var", "func", "if", "while", "REF", "DEREF", "call", "return", "else"
    };


    Operators oprands[] =
    {
        // multi-character operators go before single character operators
        {"==", OPP_COMP_EQ, {4,3}},
        {"!=", OPP_COMP_NEQ, {4,3}},
        {">=", OPP_COMP_GTE, {4,3}},
        {"<=", OPP_COMP_LTE, {4,3}},
        {">", OPP_COMP_GT, {4,3}},
        {"<", OPP_COMP_LT, {4,3}},
        {"=", OPP_ASSIGN, {2,1}},
        {"+", OPP_ADD, {11,10}},
        {"-", OPP_SUB, {11,10}},
        {"*", OPP_MUL, {21,20}},
        {"/", OPP_DIV, {21,20}},
        {"(", OPP_BRACKET_OPEN, {0,0}},
        {")", OPP_BRACKET_CLOSE, {0,0}},
        {"{", OPP_BLOCK_START, {0,0}},
        {"}", OPP_BLOCK_END, {0,0}},
        {",", OPP_LINE_END_SOFT,{0,0}},
        {";", OPP_LINE_END,{0,0}},
        {"&", OPP_REF_TOKEN,{0,0}},
        // utility/parser-side operators
        {".", OPP_DEREF_TOKEN,{0,0}},
        {".ret", OPP_RETURN_FUNC, {0,0} },
        {".call", OPP_CALL_FUNC, {0,0} },
        {".if", OPP_IF, {0,0} },
        {".while", OPP_WHILE, {0,0} }
    };
    // OPP_COMP_EQ, OPP_COMP_NEQ, OPP_COMP_GT, OPP_COMP_LT, OPP_COMP_GTE, OPP_COMP_LTE, OPP_REF_TOKEN, OPP_DEREF_TOKEN,

    constexpr int num_oprands = sizeof(oprands) / sizeof(Operators);
    constexpr int num_oprands_usable = num_oprands - 5;

    bool IsStringValue(char character)
    {
        constexpr bool lut[256] =
        {
            //00-09
            false, false, false, false, false,
            false, false, false, false, false,

            //10-19
            false, false, false, false, false,
            false, false, false, false, false,

            //20-29
            false, false, false, false, false,
            false, false, false, false, false,

            //30-39
            false, false, false, false, false,
            false, false, false, false, false,

            //40-49
            false, false, false, false, false,
            false, true, false, true, true,

            //50-59
            true, true, true, true, true,
            true, true, true, false, false,

            //60-69
            false, false, false, false, false,
            true, true, true, true, true,

            //70-79
            true, true, true, true, true,
            true, true, true, true, true,

            //80-89
            true, true, true, true, true,
            true, true, true, true, true,

            //90-99
            true, true, false, true, false,
            true, false, true, true, true,

            //100-109
            true, true, true, true, true,
            true, true, true, true, true,

            //110-119
            true, true, true, true, true,
            true, true, true, true, true,

            //120-129
            true, true, true, false, false,
            false, false, false, false, false
        };

        return lut[character];
    }

    char EscapeChar(char src)
    {
        switch (src)
        {
        case '0':
            return 0;
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case '\\':
            return '\\';
        case '\'':
            return '\'';
        case '"':
            return '"';
        case '?':
            return '\?';
        }
        return src;
    }

    void GenTokenCurrent(Vector<Token>& LineTokens, Vector<Symbol>& symbols, std::string& immediate, int& processing, int64_t& value, double after_point, double divisor, bool& was_point)
    {
        switch (processing)
        {
        case TOK_SYMBOL:
        {
            bool found = false;
            // search known symbols
            for (int i = 0; i < symbols.Size(); ++i)
            {
                if (immediate == symbols[i].name)
                {
                    Token tok;
                    tok.symbol_value = i;
                    tok.float_value = 0;
                    tok.type = TOK_SYMBOL;

                    if (SYM_KEYWORD == symbols[i].sym_type)
                    {
                        switch (symbols[i].symbol_value)
                        {
                        case KEY_RETURN:
                        {
                            int l = 0;
                            for (; l < num_oprands; ++l)
                            {
                                if (oprands[l].func == OPP_RETURN_FUNC)
                                    break;
                            }
                            tok = Token{ TOK_OPERATOR, (int64_t)l };
                        }
                        break;
                        }
                    }

                    LineTokens.PushBack(tok);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                Symbol sym;
                memset(sym.name, 0, sizeof(sym.name));
                strncpy_s(sym.name, immediate.c_str(), sizeof(sym.name));
                sym.sym_type = SYM_MAX; // CURRENTLY UNIDENTIFIED SYMBOL - will use previous token for symbol identification

                Token last_tok = LineTokens.Peek();
                if ((TOK_SYMBOL == last_tok.type) && (SYM_KEYWORD == symbols[last_tok.symbol_value].sym_type))
                {
                    switch (symbols[last_tok.symbol_value].symbol_value)
                    {
                    case KEY_FUNC:
                        sym.sym_type = SYM_FUNCTION; // resolved in parsing
                        sym.symbol_value = 0;
                        break;
                    case KEY_INT:
                        sym.sym_type = SYM_LOCAL_VAR; // resolved in parsing
                        sym.symbol_value = 0;
                        break;
                    }
                    symbols.PushBack(sym);

                    Token tok;
                    tok.symbol_value = symbols.Size() - 1;
                    tok.float_value = 0;
                    tok.type = TOK_SYMBOL;
                    LineTokens.PushBack(tok);
                }
                else
                {
                    // unidentified symbol
                    Token tok;
                    tok.symbol_value = -1;
                    tok.type = TOK_MAX;
                    LineTokens.PushBack(tok);
                }
            }
        }
        break;
        case TOK_NUMBER:
        {
            Token tok;
            if (was_point)
            {
                tok.type = TOK_FLOAT;
                tok.float_value = double(value) + after_point;
            }
            else
            {
                tok.type = TOK_NUMBER;
                tok.symbol_value = value;
            }
            LineTokens.PushBack(tok);
        }
        break;
        case TOK_OPERATOR:
        {
            Token tok;
            tok.type = TOK_OPERATOR;
            tok.symbol_value = value;
            LineTokens.PushBack(tok);
        }
        break;
        }

        immediate = "";
        processing = TOK_MAX;
        value = 0;
        was_point = false;
    }

    void AddKeywords(Vector<Symbol>& symbols)
    {
        constexpr int num_keywords = (sizeof(keyword) / sizeof(char*));
        for (int i = 0; i < num_keywords; ++i)
        {
            Symbol sym;
            memset(sym.name, 0, sizeof(sym.name));
            strncpy_s(sym.name, keyword[i], sizeof(sym.name));
            sym.sym_type = SYM_KEYWORD;
            sym.symbol_value = i;
            symbols.PushBack(sym);
        }
    }

    void TokenizeLine(const char* line, Vector<Token>& LineTokens, Vector<Symbol>& symbols, Stack* _stack)
    {
        bool is_escape_char = false, is_string_literal = false;

        std::string immediate;
        int processing = TOK_MAX;
        int64_t value = 0;
        double after_point = -1;
        double divisor = 1;
        bool was_point = false;

        while (*line)
        {
            if (is_string_literal)
            {
                if (is_escape_char)
                {
                    immediate += EscapeChar(*line);
                    is_escape_char = false;
                }
                else  if ('\\' == *line)
                {
                    is_escape_char = true;
                }
                else if ('"' == *line)
                {
                    Symbol sym;

                    char* string_ptr = (char*)_stack->Allocate(immediate.size() + 1);
                    strncpy_s(string_ptr, (size_t)immediate.size() + 1, immediate.c_str(), (size_t)immediate.size() + 1);
                    sym.sym_type = SYM_LITERAL;
                    memset(sym.name, 0, sizeof(sym.name));
                    strncpy_s(sym.name, "_literal_", sizeof(sym.name));
                    sym.symbol_value = (size_t)string_ptr - (size_t)_stack->RAW();

                    LineTokens.PushBack({ TOK_SYMBOL, (int64_t)symbols.Size() });
                    symbols.PushBack(sym);

                    is_string_literal = false;
                    immediate = "";
                    processing = TOK_MAX;
                }
                else
                {
                    immediate += *line;
                }
            }
            else
            {
                bool failed = false;
                for (int i = 0; i < num_oprands_usable; ++i)
                {
                    failed = false;
                    const char* opp_test = line;
                    int j = 0;
                    while (!failed && oprands[i].opp[j])
                    {
                        if (opp_test[j] != oprands[i].opp[j])
                        {
                            failed = true;
                            break;
                        }
                        ++j;
                    }

                    if (!failed && j)
                    {
                        while (--j)
                            line++;

                        // TODO: GEN LAST TOKEN
                        GenTokenCurrent(LineTokens, symbols, immediate, processing, value, after_point, divisor, was_point);

                        Token tok;
                        tok.type = TOK_OPERATOR;
                        tok.symbol_value = i;
                        if (OPP_BRACKET_OPEN == oprands[i].func)
                        {
                            Token tok_pre = LineTokens.Peek();
                            if (TOK_SYMBOL == tok_pre.type && SYM_KEYWORD != symbols[tok_pre.symbol_value].sym_type)
                            {

                                LineTokens.PopBack();
                                //check if not function decleration
                                int sym_func = 0;
                                for (; sym_func < symbols.Size(); ++sym_func)
                                    if (SYM_KEYWORD == symbols[sym_func].sym_type && KEY_FUNC == symbols[sym_func].symbol_value)
                                        break;
                                if (LineTokens.Peek().type != TOK_SYMBOL || sym_func != LineTokens.Peek().symbol_value)
                                {
                                    Token tok2;
                                    tok2.symbol_value = KEY_CALL;
                                    tok2.float_value = 0;
                                    tok2.type = TOK_SYMBOL;
                                    LineTokens.PushBack(tok2);
                                }
                                LineTokens.PushBack(tok_pre);
                            }
                        }
                        LineTokens.PushBack(tok);
                        break;
                    }
                }
                if (failed)
                {
                    if (*line == ' ' || *line == '\n' || *line == '\r' || *line == '\0')
                    {
                        // TODO: GEN LAST TOKEN
                        GenTokenCurrent(LineTokens, symbols, immediate, processing, value, after_point, divisor, was_point);
                    }
                    else if (*line == '"')
                    {
                        // TODO: GEN LAST TOKEN
                        GenTokenCurrent(LineTokens, symbols, immediate, processing, value, after_point, divisor, was_point);
                        is_string_literal = true;
                    }
                    else if (*line == '.')
                    {
                        if (TOK_NUMBER == processing)
                        {
                            if (!was_point)
                            {
                                was_point = true;
                                after_point = 0;
                                divisor = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            if (TOK_MAX != processing)
                            {
                                // TODO: GEN LAST TOKEN
                                GenTokenCurrent(LineTokens, symbols, immediate, processing, value, after_point, divisor, was_point);
                            }
                        }
                    }
                    else if (*line >= '0' && *line <= '9')
                    {
                        if (TOK_NUMBER != processing)
                        {
                            if (TOK_SYMBOL == processing)
                            {
                                immediate += *line;
                            }
                            else
                            {
                                if (TOK_MAX != processing)
                                {
                                    // TODO: GEN LAST TOKEN
                                    GenTokenCurrent(LineTokens, symbols, immediate, processing, value, after_point, divisor, was_point);
                                }
                                processing = TOK_NUMBER;
                            }
                        }
                        if (TOK_NUMBER == processing)
                        {
                            if (was_point)
                            {
                                divisor *= 10;
                                after_point += (*line - '0') / divisor;
                            }
                            else
                            {
                                value *= 10;
                                value += *line - '0';
                            }
                        }
                    }
                    else if (IsStringValue(*line))
                    {
                        if (TOK_SYMBOL != processing)
                        {
                            if (TOK_MAX != processing)
                            {
                                // TODO: GEN LAST TOKEN
                                GenTokenCurrent(LineTokens, symbols, immediate, processing, value, after_point, divisor, was_point);
                            }
                            processing = TOK_SYMBOL;
                        }
                        immediate += *line;
                    }
                    else
                    {
                        if (TOK_MAX != processing)
                        {
                            // TODO: GEN LAST TOKEN
                            GenTokenCurrent(LineTokens, symbols, immediate, processing, value, after_point, divisor, was_point);
                        }
                    }
                }

            }
            line++;
        }
    }

    void SplitTokens(Vector<Token>& LineTokens, Vector<Vector<Token>>& TokenTokens)
    {
        size_t line = 0;
        TokenTokens.ClearCplx();
        TokenTokens.PushBack(Vector<Token>());
        for (int i = 0; i < LineTokens.Size(); ++i)
        {
            TokenTokens[line].PushBack(LineTokens[i]);
            if (TOK_OPERATOR == LineTokens[i].type)
            {
                switch (oprands[LineTokens[i].symbol_value].func)
                {
                case OPP_BLOCK_START:
                case OPP_BLOCK_END:
                case OPP_LINE_END:
                    line++;
                    TokenTokens.PushBack(Vector<Token>());
                    break;
                case OPP_LINE_END_SOFT:
                {
                    int return_val = 0;
                    for (; return_val < num_oprands; ++return_val)
                        if (OPP_LINE_END == oprands[return_val].func)
                            break;
                    TokenTokens[line].Peek().symbol_value = return_val;

                }
                break;
                }
            }
        }
    }

    bool IsAtomicType(TokType_e tp)
    {
        //TOK_NUMBER, TOK_FLOAT, TOK_OPERATOR, TOK_SYMBOL, TOK_MAX
        switch (tp)
        {
        case TOK_NUMBER:
        case TOK_FLOAT:
        case TOK_SYMBOL:
            return true;
        }
        return false;
    }

    bool OperatorMath(OperatorType_e tp)
    {
        //	OPP_ASSIGN, OPP_ADD, OPP_SUB, OPP_MUL, OPP_DIV, OPP_BLOCK_START, OPP_BLOCK_END, 
        // OPP_BRACKET_OPEN, OPP_BRACKET_CLOSE, OPP_LINE_END, OPP_CALL_FUNC, OPP_CALLING_FUNC, OPP_RETURN_FUNC, OPP_MAX

        switch (tp)
        {
        case OPP_ASSIGN:
        case OPP_ADD:
        case OPP_SUB:
        case OPP_MUL:
        case OPP_DIV:
        case OPP_COMP_EQ:
        case OPP_COMP_NEQ:
        case OPP_COMP_LT:
        case OPP_COMP_GT:
        case OPP_COMP_LTE:
        case OPP_COMP_GTE:
            return true;
        }
        return false;
    }

    void ParseParams(Vector<Token>& current_line, size_t& index, Vector<Symbol>& symbols, Vector<Function>& calls, Vector<Instruction>& program, size_t& function_base, Stack* _stack)
    {
        const Token& current_token = current_line[index++];
        if (TOK_OPERATOR == current_token.type && OPP_BRACKET_OPEN == oprands[current_token.symbol_value].func)
        {
            Token* active_token = &current_line[index++];
            while (index < current_line.Size() && (TOK_OPERATOR != active_token->type || OPP_BRACKET_CLOSE != oprands[active_token->symbol_value].func))
            {
                if (TOK_SYMBOL == active_token->type)
                {
                    if (SYM_KEYWORD == symbols[active_token->symbol_value].sym_type)
                    {
                        switch (symbols[active_token->symbol_value].symbol_value)
                        {
                        case KEY_INT:
                            if (TOK_SYMBOL == current_line[index].type)
                            {
                                Symbol my_sym = symbols[current_line[index].symbol_value];

                                if (SYM_LOCAL_VAR == my_sym.sym_type)
                                {
                                    if (-1 == function_base)
                                    {
                                        my_sym.sym_type = SYM_GLOBAL_VAR;
                                        my_sym.symbol_value = _stack->Size();
                                        Token* tok = (Token*)_stack->Allocate(sizeof(Token));
                                        memset(tok, 0, sizeof(Token));

                                    }
                                    else
                                    {
                                        my_sym.symbol_value = calls[function_base].stack_size;
                                        calls[function_base].stack_size += sizeof(Token);
                                        calls[function_base].param_amount++;

                                        TokenReplacement toks = { current_line[index].symbol_value  , symbols.Size() };
                                        calls[function_base].local_tokens.PushBack(toks);
                                        symbols.PushBack(my_sym);

                                        current_line[index].symbol_value = toks.local_symbol_value;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
                active_token = &current_line[index++];
            }
        }
    }

    size_t ParseParamCall(Vector<Token>& current_line, size_t& index, Vector<Symbol>& symbols, Vector<Function>& calls, Vector<Instruction>& program, size_t& function_base, Stack* _stack, size_t call_instruction)
    {
        const Token& current_token = current_line[index++];
        size_t instruction_ind = 0;
        size_t instruction_ind_active = 0;
        size_t instruction_ind_pre = 0;

        if (TOK_OPERATOR == current_token.type && OPP_BRACKET_OPEN == oprands[current_token.symbol_value].func)
        {
            Token* active_token = &current_line[index];
            while (index < current_line.Size() && (TOK_OPERATOR != active_token->type || OPP_BRACKET_CLOSE != oprands[active_token->symbol_value].func))
            {
                // 
                instruction_ind_active = ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, 0);
                if (0 == instruction_ind)
                    instruction_ind = instruction_ind_active;

                if (0 != instruction_ind_pre)
                    program[instruction_ind_pre].next = instruction_ind_active;

                if (0 != instruction_ind_active)
                    instruction_ind_pre = instruction_ind_active;

                active_token = &current_line[index++];
            }
        }
        return instruction_ind;
    }

    size_t ParseCall(Vector<Token>& current_line, size_t& index, Vector<Symbol>& symbols, Vector<Function>& calls, Vector<Instruction>& program, size_t& function_base, Stack* _stack, size_t call_instruction)
    {
        Token token_first = current_line[index];
        Instruction base_instruction;
        size_t chain_index = 0;

        Instruction& function_instruction = program[call_instruction];

        if (TOK_SYMBOL == token_first.type)
        {
            Symbol my_sym = symbols[token_first.symbol_value];

            base_instruction.token_instruct = token_first;
            base_instruction.num_param = 0;
            base_instruction.next = 0;

            program.PushBack(base_instruction);
            chain_index = program.Size() - 1;
            size_t instruction = 0;

            instruction = ParseParamCall(current_line, ++index, symbols, calls, program, function_base, _stack, call_instruction);

            if (instruction)
            {
                program[chain_index].num_param = 1;
                program[chain_index].params[0] = instruction;
            }
        }
        else
        {
            printf("Calling not a symbol\n");
        }
        return chain_index;
    }

    size_t ParseLine(Vector<Token>& current_line, size_t& index, size_t last_inst, Vector<Symbol>& symbols, Vector<Function>& calls, Vector<Instruction>& program, size_t function_base, Stack* _stack, int _power)
    {
        // code sequence added to fix compiler optimization not aggregating values on release mode:
        // size_t parsed_instruction = ParseLine(current_line, ++index, 0, symbols, calls, program, function_base, _stack, 0);
        // program[chain_index].params[0] = parsed_instruction;
        // for some goddamn readon MSVC compiler doesn't set values inside structure with this line when flag /O2 is set(default release optimization flag):
        // program[chain_index].params[0] = ParseLine(current_line, ++index, 0, symbols, calls, program, function_base, _stack, 0);
        Token token_first = current_line[index];
        Instruction base_instruction;
        size_t chain_index = 0;
        bool is_call = false;
        bool is_declaration = false;

        if (TOK_SYMBOL == token_first.type)
        {
            if (SYM_KEYWORD == symbols[token_first.symbol_value].sym_type)
            {
                switch (symbols[token_first.symbol_value].symbol_value)
                {
                case KEY_FUNC:
                    // function declaration
                {
                    Function func;
                    if (TOK_SYMBOL == current_line[index + 1].type)
                    {
                        int64_t function_index = current_line[index + 1].symbol_value;
                        Symbol& my_sym = symbols[function_index];

                        if (SYM_FUNCTION == my_sym.sym_type)
                        {
                            if (-1 == function_base)
                            {
                                my_sym.symbol_value = calls.Size();
                                calls.PushBack(func);
                                index += 2;
                                // Parse Params

                                ParseParams(current_line, index, symbols, calls, program, function_base, _stack);

                                base_instruction.token_instruct = Token{ TOK_SYMBOL, function_index };
                                base_instruction.num_param = 0;
                                base_instruction.next = 0;

                                program.PushBack(base_instruction);

                                chain_index = program.Size() - 1;
                                calls.Peek().start_instruction = chain_index;
                                size_t parsed_instruction = ParseLine(current_line, index, 0, symbols, calls, program, calls.Size() - 1, _stack, _power);
                                program[chain_index].next = parsed_instruction;

                                if (0 == chain_index)
                                {
                                    printf("Error Chained 0\n");
                                }
                                return chain_index;
                            }
                            else
                            {
                                if (0 == chain_index)
                                {
                                    printf("Error no function base when expecting function\n");
                                }
                                // ERROR
                            }

                        }
                    }
                }
                break;
                case KEY_INT:
                    if (TOK_SYMBOL == current_line[index + 1].type)
                    {
                        Symbol& my_sym = symbols[current_line[index + 1].symbol_value];

                        if (SYM_LOCAL_VAR == my_sym.sym_type)
                        {
                            if (-1 == function_base)
                            {
                                my_sym.sym_type = SYM_GLOBAL_VAR;
                                my_sym.symbol_value = _stack->Size();
                                Token* tok = (Token*)_stack->Allocate(sizeof(Token));
                                memset(tok, 0, sizeof(Token));
                            }
                            else
                            {
                                Symbol cpy_sym = my_sym;
                                cpy_sym.symbol_value = calls[function_base].stack_size;
                                calls[function_base].stack_size += sizeof(Token);

                                TokenReplacement toks = { current_line[index + 1].symbol_value  , symbols.Size() };
                                calls[function_base].local_tokens.PushBack(toks);
                                symbols.PushBack(cpy_sym);
                                current_line[index + 1].symbol_value = toks.local_symbol_value;
                            }

                            index += 1;

                            return ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, _power);
                        }
                    }
                    break;

                case KEY_IF:
                {
                    index += 1;
                    int l = 0;
                    for (; l < num_oprands; ++l)
                    {
                        if (oprands[l].func == OPP_IF)
                            break;
                    }
                    base_instruction.token_instruct = Token{ TOK_OPERATOR, l };
                    base_instruction.num_param = 2;
                    base_instruction.next = 0;
                    chain_index = program.PushBack(base_instruction);

                    Token out = current_line[index];
                    if (TOK_OPERATOR == out.type && OPP_BRACKET_OPEN == oprands[out.symbol_value].func)
                    {
                        index++;
                    }
                    size_t parsed_instruction = ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, 0);
                    program[chain_index].params[0] = parsed_instruction;

                    out = current_line[index];
                    if (TOK_OPERATOR == out.type && OPP_BRACKET_CLOSE == oprands[out.symbol_value].func)
                    {
                        index++;
                    }
                    else
                    {
                        printf("attempting oprand %s when expecting ')'\n", oprands[out.symbol_value].opp);
                        return 0;
                    }
                    parsed_instruction = ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, 0);
                    program[chain_index].params[1] = parsed_instruction;

                    return chain_index;
                }
                break;

                case KEY_ELSE:
                {
                    index += 1;
                    int l = 0;
                    for (; l < num_oprands; ++l)
                    {
                        if (oprands[l].func == OPP_IF)
                            break;
                    }
                    Instruction& last_instruct = program[last_inst];
                    if (TOK_OPERATOR == last_instruct.token_instruct.type && l == last_instruct.token_instruct.symbol_value)
                    {
                        last_instruct.num_param = 3;
                        size_t parsed_instruction = ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, 0);
                        last_instruct.params[2] = parsed_instruction;
                    }
                    return last_inst;
                }
                break;
                case KEY_WHILE:
                {
                    index += 1;
                    int l = 0;
                    for (; l < num_oprands; ++l)
                    {
                        if (oprands[l].func == OPP_WHILE)
                            break;
                    }
                    base_instruction.token_instruct = Token{ TOK_OPERATOR, l };
                    base_instruction.num_param = 2;
                    base_instruction.next = 0;
                    chain_index = program.PushBack(base_instruction);

                    Token out = current_line[index];
                    if (TOK_OPERATOR == out.type && OPP_BRACKET_OPEN == oprands[out.symbol_value].func)
                    {
                        index++;
                    }
                    size_t parsed_instruction = ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, 0);
                    program[chain_index].params[0] = parsed_instruction;
                    out = current_line[index];
                    if (TOK_OPERATOR == out.type && OPP_BRACKET_CLOSE == oprands[out.symbol_value].func)
                    {
                        index++;
                    }
                    else
                    {
                        if (TOK_OPERATOR == out.type)
                            printf("attempting oprand %s when expecting ')'\n", oprands[out.symbol_value].opp);
                        else
                            printf("symbol expected operand ')' not an operand\n");
                        return 0;
                    }
                    parsed_instruction = ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, 0);
                    program[chain_index].params[1] = parsed_instruction;
                    return chain_index;
                }
                break;

                case KEY_CALL:
                {
                    int l = 0;
                    for (; l < num_oprands; ++l)
                    {
                        if (oprands[l].func == OPP_CALL_FUNC)
                            break;
                    }
                    base_instruction.token_instruct = Token{ TOK_OPERATOR, l };
                    base_instruction.num_param = 1;
                    base_instruction.next = 0;
                    chain_index = program.PushBack(base_instruction);
                    size_t parsed_instruction = ParseCall(current_line, ++index, symbols, calls, program, function_base, _stack, chain_index);
                    program[chain_index].params[0] = parsed_instruction;
                    is_call = true;
                }
                break;
                }
            }
        }

        {
            if (IsAtomicType(token_first.type) || is_call)
            {
                if (!is_call)
                {
                    if (-1 != function_base)
                    {
                        if (TOK_SYMBOL == token_first.type)
                        {
                            for (int k = 0; k < calls[function_base].local_tokens.Size(); ++k)
                            {
                                if (calls[function_base].local_tokens[k].source_symbol_value == token_first.symbol_value)
                                {
                                    token_first.symbol_value = calls[function_base].local_tokens[k].local_symbol_value;
                                    break;
                                }
                            }
                        }
                    }
                    base_instruction.token_instruct = token_first;
                    base_instruction.num_param = 0;
                    base_instruction.next = 0;
                    program.PushBack(base_instruction);

                    chain_index = program.Size() - 1;
                    index++;
                }

                while (index < current_line.Size())
                {
                    Token op = current_line[index];
                    if (TOK_OPERATOR == op.type)
                    {
                        if (OperatorMath((OperatorType_e)oprands[op.symbol_value].func))
                        {
                            binding_power powers = oprands[op.symbol_value].powers;
                            if (powers.lbp < _power)
                            {
                                return chain_index;
                            }
                            index += 1;
                            size_t rhs = ParseLine(current_line, index, 0, symbols, calls, program, function_base, _stack, powers.rbp);
                            if (0 == rhs) // error
                            {
                                printf("symbol instruction expected, none found\n");
                                return 0;
                            }

                            Instruction added;

                            added.params[0] = chain_index;
                            added.params[1] = rhs;
                            added.num_param = 2;
                            added.token_instruct = op;
                            added.next = 0;
                            program.PushBack(added);

                            chain_index = program.Size() - 1;
                        }
                        else
                        {
                            if ((oprands[op.symbol_value].func == OPP_BRACKET_CLOSE) || (oprands[op.symbol_value].func == OPP_LINE_END))
                            {
                                return chain_index;
                            }
                            else
                            {
                                printf("attempting oprand %s\n", oprands[op.symbol_value].opp);
                                return 0; // unhandled operator
                            }
                        }
                    }
                    else
                    {
                        return chain_index;
                    }
                }
            }
            else
            {
                if (TOK_OPERATOR == token_first.type)
                {
                    switch (oprands[token_first.symbol_value].func)
                    {
                    case OPP_BRACKET_OPEN:
                    {
                        chain_index = ParseLine(current_line, ++index, 0, symbols, calls, program, function_base, _stack, 0);

                        Token out = current_line[index];
                        if (TOK_OPERATOR == out.type && OPP_BRACKET_CLOSE == oprands[out.symbol_value].func)
                        {
                            index++;
                            return chain_index;
                        }
                        else
                        {
                            if (TOK_OPERATOR == out.type)
                                printf("attempting oprand %s when expecting ')'\n", oprands[out.symbol_value].opp);
                            else
                                printf("symbol expected operand ')' not an operand\n");

                            return 0; // expected close bracket
                        }
                    }
                    break;
                    case OPP_RETURN_FUNC:
                    {
                        base_instruction.token_instruct = token_first;
                        base_instruction.num_param = 1;
                        base_instruction.next = 0;
                        chain_index = program.PushBack(base_instruction);
                        size_t parsed_instruction = ParseLine(current_line, ++index, 0, symbols, calls, program, function_base, _stack, 0);
                        program[chain_index].params[0] = parsed_instruction;
                    }
                    break;
                    case OPP_MUL:
                    case OPP_DEREF_TOKEN:
                    {
                        // find deref token
                        int l = 0;
                        for (; l < num_oprands; ++l) if (oprands[l].func == OPP_DEREF_TOKEN) break;

                        base_instruction.token_instruct = Token(TOK_OPERATOR, l);
                        base_instruction.num_param = 1;
                        base_instruction.next = 0;
                        chain_index = program.PushBack(base_instruction);
                        size_t parsed_instruction = ParseLine(current_line, ++index, 0, symbols, calls, program, function_base, _stack, oprands[base_instruction.token_instruct.symbol_value].powers.rbp);
                        program[chain_index].params[0] = parsed_instruction;
                    }
                    break;
                    case OPP_REF_TOKEN:
                    {
                        base_instruction.token_instruct = token_first;
                        base_instruction.num_param = 1;
                        base_instruction.next = 0;
                        chain_index = program.PushBack(base_instruction);
                        size_t parsed_instruction = ParseLine(current_line, ++index, 0, symbols, calls, program, function_base, _stack, oprands[base_instruction.token_instruct.symbol_value].powers.rbp);
                        //printf("%d", (int)parsed_instruction);
                        program[chain_index].params[0] = parsed_instruction;
                    }
                    break;
                    }
                }
            }
        }
        return chain_index;
    }

    void ParseProgramBlock(int& extern_i, size_t instruction_block_start, Vector<Instruction>& program, Vector<Symbol>& symbols, Stack* _stack, Vector<Vector<Token>>& token_instruction, Vector<Function>& calls)
    {
        size_t current_function = calls.Size() - 1;
        size_t last_inst = 0;
        program.PushBack({});
        for (int i = extern_i + 1; i < token_instruction.Size(); ++i)
        {
            extern Operators oprands[];
            size_t ind = 0;
            //size_t functions = calls.Size();
            Vector<Token>& local_line = token_instruction[i];
            if (local_line.Size())
            {
                size_t current_inst = ParseLine(local_line, ind, last_inst, symbols, calls, program, current_function, _stack, 0);

                Instruction& my_block = program[instruction_block_start];

                int if_ind, while_ind;
                int l = 0;
                for (; l < num_oprands; ++l)
                {
                    if (oprands[l].func == OPP_IF)
                        if_ind = l;
                    if (oprands[l].func == OPP_WHILE)
                        while_ind = l;
                }
                if (0 == my_block.num_param && 0 != current_inst)
                {
                    my_block.params[0] = current_inst;
                    my_block.num_param = 1;
                }

                Token& end_tok = local_line.Peek();

                if (TOK_OPERATOR == end_tok.type && OPP_BLOCK_START == oprands[end_tok.symbol_value].func)
                {
                    // add program block instruction
                    Instruction block_inst;
                    block_inst.token_instruct = Token(TOK_BLOCK);
                    block_inst.num_param = 0;
                    block_inst.params[0] = 0;
                    block_inst.next = 0;

                    program.PushBack(block_inst);

                    if (TOK_OPERATOR == program[current_inst].token_instruct.type && if_ind == program[current_inst].token_instruct.symbol_value)
                    {
                        program[current_inst].params[program[current_inst].num_param - 1] = program.Size() - 1;

                        ParseProgramBlock(i, program.Size() - 1, program, symbols, _stack, token_instruction, calls);

                        if (program[current_inst].num_param == 2)
                        {
                            if (last_inst)
                                program[last_inst].next = current_inst;
                            else
                                if (i == 0)
                                    program[last_inst].next = current_inst;
                        }
                    }
                    else if (TOK_OPERATOR == program[current_inst].token_instruct.type && while_ind == program[current_inst].token_instruct.symbol_value)
                    {
                        program[current_inst].params[program[current_inst].num_param - 1] = program.Size() - 1;

                        ParseProgramBlock(i, program.Size() - 1, program, symbols, _stack, token_instruction, calls);

                        if (program[current_inst].num_param == 2)
                        {
                            if (last_inst)
                                program[last_inst].next = current_inst;
                            else
                                if (i == 0)
                                    program[last_inst].next = current_inst;
                        }
                    }
                    else
                    {
                        if (0 == current_inst)
                        {
                            printf("Error Chained 0\n");
                        }
                        program[current_inst].next = program.Size() - 1;

                        ParseProgramBlock(i, program.Size() - 1, program, symbols, _stack, token_instruction, calls);
                    }
                }
                else
                {
                    if (last_inst)
                        program[last_inst].next = current_inst;
                    else
                        if (i == 0)
                            program[last_inst].next = current_inst;
                }

                if (TOK_OPERATOR == local_line[0].type && OPP_BLOCK_END == oprands[local_line[0].symbol_value].func)
                {
                    extern_i = i;
                    return;
                }

                last_inst = current_inst;
            }
        }
    }

    void ParseProgram(Vector<Instruction>& program, Vector<Symbol>& symbols, Stack* _stack, Vector<Vector<Token>>& token_instruction, Vector<Function>& calls)
    {
        size_t current_function = -1;
        size_t last_inst = 0;
        program.PushBack({});
        bool has_declared_function = false;
        for (int i = 0; i < token_instruction.Size(); ++i)
        {
            extern Operators oprands[];
            size_t ind = 0;
            size_t functions = calls.Size();
            Vector<Token>& local_line = token_instruction[i];
            if (local_line.Size())
            {
                size_t current_inst = ParseLine(local_line, ind, last_inst, symbols, calls, program, current_function, _stack, 0);
                Token& end_tok = local_line.Peek();

                if (functions != calls.Size())
                {
                    has_declared_function = true;
                }

                if (TOK_OPERATOR == end_tok.type && OPP_BLOCK_START == oprands[end_tok.symbol_value].func)
                {
                    // add program block instruction
                    Instruction block_inst;
                    block_inst.token_instruct = Token(TOK_BLOCK);
                    block_inst.num_param = 0;
                    block_inst.params[0] = 0;
                    block_inst.next = 0;

                    if (has_declared_function)
                    {
                        current_function = calls.Size() - 1;
                    }

                    program.PushBack(block_inst);
                    program[current_inst].next = program.Size() - 1;

                    current_inst = program[current_inst].next;
                    ParseProgramBlock(i, program.Size() - 1, program, symbols, _stack, token_instruction, calls);

                    current_function = -1;
                }
                else
                {
                    if (!has_declared_function)
                    {
                        if (0 != last_inst)
                            program[last_inst].next = current_inst;
                        else
                            if (i == 0)
                                program[last_inst].next = current_inst;
                    }
                }
                if (!has_declared_function)
                {
                    last_inst = current_inst;
                }
            }
        }
    }

    Token& ResolveToken(Token& target, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        char* stack_raw_ptr = (char*)_stack->RAW();
        size_t base_pointer = stack_base.Peek();

        if (target.type != TOK_SYMBOL)
            return (Token&)target;
        switch (symbols[target.symbol_value].sym_type)
        {
            /*
        case SYM_LITERAL:
        case SYM_FUNCTION:
        case SYM_KEYWORD:
            return (Token&)target;
            */
        case SYM_GLOBAL_VAR:
            return *(Token*)(stack_raw_ptr + symbols[target.symbol_value].symbol_value);
        case SYM_LOCAL_VAR:
            return *(Token*)(stack_raw_ptr + base_pointer + symbols[target.symbol_value].symbol_value);
        }
        return (Token&)target;
    }

    int64_t GetValueI(Token t)
    {
        if (TOK_FLOAT == t.type)
            return t.float_value;
        return t.symbol_value;
    }

    double GetValueF(Token t)
    {
        if (TOK_FLOAT == t.type)
            return t.float_value;
        if (TOK_NUMBER == t.type)
            return t.symbol_value;
    }

    std::string GetValueS(Token t, const Vector<Symbol>& symbols, Stack* _stack)
    {
        if (TOK_FLOAT == t.type)
            return std::to_string(t.float_value);
        if (TOK_NUMBER == t.type)
            return std::to_string(t.symbol_value);
        if (TOK_SYMBOL == t.type)
            return ((char*)_stack->RAW() + symbols[t.symbol_value].symbol_value);
        return "";
    }

    void AddTokens(Token& target, Token source, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        Token target_real = ResolveToken(target, _stack, symbols, stack_base);
        Token source_real = ResolveToken(source, _stack, symbols, stack_base);

        if (TOK_SYMBOL == target_real.type || TOK_SYMBOL == source_real.type)
        {
            return;
        }
        if (TOK_FLOAT == target_real.type || TOK_FLOAT == source_real.type)
        {
            double value = GetValueF(target_real) + GetValueF(source_real);
            target = { TOK_FLOAT, 0, value };
            return;
        }
        if (TOK_NUMBER == target_real.type && TOK_NUMBER == source_real.type)
        {
            int64_t value = GetValueI(target_real) + GetValueI(source_real);
            target = { TOK_NUMBER, value };
            return;
        }
        return;
    }

    void SubTokens(Token& target, Token source, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        Token target_real = ResolveToken(target, _stack, symbols, stack_base);
        Token source_real = ResolveToken(source, _stack, symbols, stack_base);

        if (TOK_SYMBOL == target_real.type || TOK_SYMBOL == source_real.type)
        {
            return;
        }
        if (TOK_FLOAT == target_real.type || TOK_FLOAT == source_real.type)
        {
            double value = GetValueF(target_real) - GetValueF(source_real);
            target = { TOK_FLOAT, 0, value };
            return;
        }
        if (TOK_NUMBER == target_real.type && TOK_NUMBER == source_real.type)
        {
            int64_t value = GetValueI(target_real) - GetValueI(source_real);
            target = { TOK_NUMBER, value };
            return;
        }
        return;
    }

    void MulTokens(Token& target, Token source, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        Token target_real = ResolveToken(target, _stack, symbols, stack_base);
        Token source_real = ResolveToken(source, _stack, symbols, stack_base);

        if (TOK_SYMBOL == target_real.type || TOK_SYMBOL == source_real.type)
        {
            return;
        }
        if (TOK_FLOAT == target_real.type || TOK_FLOAT == source_real.type)
        {
            double value = GetValueF(target_real) * GetValueF(source_real);
            target = { TOK_FLOAT, 0, value };
            return;
        }
        if (TOK_NUMBER == target_real.type && TOK_NUMBER == source_real.type)
        {
            int64_t value = GetValueI(target_real) * GetValueI(source_real);
            target = { TOK_NUMBER, value };
            return;
        }
        return;
    }

    void DivTokens(Token& target, Token source, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        Token target_real = ResolveToken(target, _stack, symbols, stack_base);
        Token source_real = ResolveToken(source, _stack, symbols, stack_base);

        if (TOK_SYMBOL == target_real.type || TOK_SYMBOL == source_real.type)
        {
            return;
        }
        if (TOK_FLOAT == target_real.type || TOK_FLOAT == source_real.type)
        {
            double value = GetValueF(target_real) / GetValueF(source_real);
            target = { TOK_FLOAT, 0, value };
            return;
        }
        if (TOK_NUMBER == target_real.type && TOK_NUMBER == source_real.type)
        {
            int64_t value = GetValueI(target_real) / GetValueI(source_real);
            target = { TOK_NUMBER, value };
            return;
        }
        return;
    }

    Token RefTokens(Token target, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        Token target_real = ResolveToken(target, _stack, symbols, stack_base);

        if (TOK_NUMBER == target_real.type)
        {
            target_real.type = TOK_SYMBOL;
            return target_real;
        }
        return target_real;
    }

    Token DerefTokens(Token target, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        if (TOK_SYMBOL == target.type)
        {
            if (SYM_LOCAL_VAR == symbols[target.symbol_value].sym_type)
            {
                int64_t target_ref = symbols[target.symbol_value].symbol_value += stack_base.Peek();
                int64_t found_target = -1;
                // reuse existing symbols - used to pass local variables out of the function
                for (int i = 0; i < symbols.Size(); ++i)
                {
                    if (SYM_LITERAL == symbols[i].sym_type || SYM_GLOBAL_VAR == symbols[i].sym_type)
                    {
                        if (symbols[i].symbol_value == target_ref)
                        {
                            found_target = i;
                            break;
                        }
                    }
                }
                if (-1 == found_target)
                {
                    symbols.PushBack(symbols[target.symbol_value]);
                    symbols.Peek().sym_type = SYM_GLOBAL_VAR;
                    symbols.Peek().symbol_value = target_ref;
                    found_target = symbols.Size() - 1;
                }
                target.symbol_value = found_target;
            }
            target.type = TOK_NUMBER;
            return target;
        }
        return target;
    }

    Token CompTokens(Token tok1, Token tok2, OperatorType_e compare_type, Stack* _stack, Vector<Symbol>& symbols, Vector<size_t>& stack_base)
    {
        Token target_real = ResolveToken(tok1, _stack, symbols, stack_base);
        Token source_real = ResolveToken(tok2, _stack, symbols, stack_base);
        Token result = Token(TOK_NUMBER, 0);
        if (TOK_SYMBOL == target_real.type || TOK_SYMBOL == source_real.type)
        {
            switch (compare_type)
            {
            case OPP_COMP_EQ:
            case OPP_COMP_NEQ:
                result.symbol_value = (target_real.type == source_real.type) && (target_real.symbol_value == source_real.symbol_value);
                break;
            }
            if (OPP_COMP_NEQ == compare_type)
                result.symbol_value = (int64_t)!result.symbol_value;
        }
        else if (TOK_FLOAT == target_real.type || TOK_FLOAT == source_real.type)
        {
            double value = GetValueF(target_real) - GetValueF(source_real);
            switch (compare_type)
            {
            case OPP_COMP_EQ:
                result.symbol_value = value == 0.;
                break;
            case OPP_COMP_NEQ:
                result.symbol_value = value != 0.;
                break;
            case OPP_COMP_GT:
                result.symbol_value = value > 0.;
                break;
            case OPP_COMP_LT:
                result.symbol_value = value < 0.;
                break;
            case OPP_COMP_GTE:
                result.symbol_value = value >= 0.;
                break;
            case OPP_COMP_LTE:
                result.symbol_value = value <= 0.;
                break;

            }
        }
        else if (TOK_NUMBER == target_real.type && TOK_NUMBER == source_real.type)
        {
            int64_t value = GetValueI(target_real) - GetValueI(source_real);
            switch (compare_type)
            {
            case OPP_COMP_EQ:
                result.symbol_value = value == 0;
                break;
            case OPP_COMP_NEQ:
                result.symbol_value = value != 0;
                break;
            case OPP_COMP_GT:
                result.symbol_value = value > 0;
                break;
            case OPP_COMP_LT:
                result.symbol_value = value < 0;
                break;
            case OPP_COMP_GTE:
                result.symbol_value = value >= 0;
                break;
            case OPP_COMP_LTE:
                result.symbol_value = value <= 0;
                break;
            }
        }

        return result;
    }

    Token OperatorFunc(Operators oprand, int param_amount, size_t* params, Stack* _stack, Vector<Instruction>& program, Vector<Function>& calls, Vector<Symbol>& symbols, Vector<size_t>& stack_base, bool& was_return_called)
    {
        Token zero = { TOK_NUMBER, 0, 0 };
        switch (oprand.func)
        {
        case OPP_ASSIGN:
            //for (int i = 0; i < param_amount; ++i)
            if (2 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                inst_local = params[1];
                Token my = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                ResolveToken(zero, _stack, symbols, stack_base) = ResolveToken(my, _stack, symbols, stack_base);
            }
            return zero;
        case OPP_ADD:
            //for (int i = 0; i < param_amount; ++i)
            if (2 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                inst_local = params[1];
                Token par2 = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                AddTokens(zero, par2, _stack, symbols, stack_base);
            }
            return zero;
        case OPP_SUB:
            //for (int i = 0; i < param_amount; ++i)
            if (2 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                inst_local = params[1];
                Token local1 = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                SubTokens(zero, local1, _stack, symbols, stack_base);
            }
            return zero;
        case OPP_MUL:
            //for (int i = 0; i < param_amount; ++i)
            if (2 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                inst_local = params[1];
                Token local1 = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                MulTokens(zero, local1, _stack, symbols, stack_base);
            }
            return zero;
        case OPP_DIV:
            //for (int i = 0; i < param_amount; ++i)
            if (2 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                inst_local = params[1];
                Token local1 = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                DivTokens(zero, local1, _stack, symbols, stack_base);
            }
            return zero;
        case OPP_CALL_FUNC:
        {
            size_t inst_local = params[0];
            zero = RunFunctionCall(program, calls, symbols, _stack, inst_local, stack_base);
        }
        return zero;
        case OPP_IF:
        {
            size_t inst_local = params[0];
            zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
            //ResolveToken(zero, _stack, symbols, stack_base);
            if (zero.type != TOK_FLOAT)
            {
                if (0 != zero.symbol_value)
                {
                    inst_local = params[1];
                }
                else if (param_amount == 3)
                {
                    inst_local = params[2];
                }

            }
            else
            {
                if (0. < zero.float_value)
                {
                    inst_local = params[1];
                }
                else if (param_amount == 3)
                {
                    inst_local = params[2];
                }
            }
            if (inst_local)
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
        }
        return zero;

        case OPP_WHILE:
        {
            bool go_again = true;
            while (go_again)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                zero = ResolveToken(zero, _stack, symbols, stack_base);
                go_again = false;
                if (zero.type != TOK_FLOAT)
                {
                    if (0 != zero.symbol_value)
                    {
                        inst_local = params[1];
                        go_again = true;
                    }

                }
                else
                {
                    if (0. < zero.float_value)
                    {
                        inst_local = params[1];
                        go_again = true;
                    }
                }
                if (inst_local)
                    zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
            }
        }
        return zero;

        case OPP_COMP_EQ:
        case OPP_COMP_NEQ:
        case OPP_COMP_GT:
        case OPP_COMP_LT:
        case OPP_COMP_GTE:
        case OPP_COMP_LTE:
            //for (int i = 0; i < param_amount; ++i)
            if (2 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                inst_local = params[1];
                Token local1 = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                zero = CompTokens(zero, local1, (OperatorType_e)oprand.func, _stack, symbols, stack_base);
            }
            return zero;


        case OPP_REF_TOKEN:
            if (1 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                zero = RefTokens(zero, _stack, symbols, stack_base);
            }
            return zero;
        case OPP_DEREF_TOKEN:
            //for (int i = 0; i < param_amount; ++i)
            if (1 == param_amount)
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);

                zero = DerefTokens(zero, _stack, symbols, stack_base);
            }
            return zero;
        case OPP_RETURN_FUNC:
            if (params[0])
            {
                size_t inst_local = params[0];
                zero = RunInstruction(program, calls, symbols, _stack, inst_local, stack_base, was_return_called);
                was_return_called = true;
            }
            return zero;
        }
    }

    Token RunCodeBlock(Vector<Instruction>& program, Vector<Function>& calls, Vector<Symbol>& symbols, Stack* _stack, size_t instruction, Vector<size_t>& stack_base, bool& was_return_called)
    {
        Token return_line = Token();
        do
        {
            return_line = RunInstruction(program, calls, symbols, _stack, instruction, stack_base, was_return_called);
        } while (0 != instruction && !was_return_called);

        return return_line;
    }

    Token RunInstruction(Vector<Instruction>& program, Vector<Function>& calls, Vector<Symbol>& symbols, Stack* _stack, size_t& instruction, Vector<size_t>& stack_base, bool& was_return_called)
    {
        Instruction& running = program[instruction];

        running.token_result = running.token_instruct;

        switch (running.token_instruct.type)
        {
        case TOK_NUMBER:
        case TOK_FLOAT:
        case TOK_SYMBOL:
            running.token_result = running.token_instruct;
            break;
        case TOK_OPERATOR:
            running.token_result = OperatorFunc(oprands[running.token_instruct.symbol_value], running.num_param, running.params, _stack, program, calls, symbols, stack_base, was_return_called);
            break;
        case TOK_BLOCK:
            if (running.num_param)
                running.token_result = RunCodeBlock(program, calls, symbols, _stack, running.params[0], stack_base, was_return_called);
            break;
        }
        instruction = running.next;
        return running.token_result;
    }

    Token RunFunctionCall(Vector<Instruction>& program, Vector<Function>& calls, Vector<Symbol>& symbols, Stack* _stack, size_t instruction, Vector<size_t>& stack_base)
    {
        bool was_return_called = false;
        Token return_line = Token();
        Instruction& function_inst = program[instruction];
        Token functoion_to_call = function_inst.token_instruct;
        Symbol sym = symbols[functoion_to_call.symbol_value];

        if (sym.sym_type != SYM_FUNCTION)
        {
            functoion_to_call = ResolveToken(function_inst.token_instruct, _stack, symbols, stack_base);

            sym = symbols[functoion_to_call.symbol_value];

            function_inst.next = calls[sym.symbol_value].start_instruction;
        }

        const Function& call = calls[sym.symbol_value];

        // call stack
        size_t next_stack_base = _stack->Size();

        size_t param_inst = function_inst.params[0];

        Token* params = (Token*)_stack->Allocate(call.stack_size);
        Token* params_runner = params;
        int param_counter = 0;
        while (0 != param_inst)
        {
            return_line = RunInstruction(program, calls, symbols, _stack, param_inst, stack_base, was_return_called);

            *params_runner++ = ResolveToken(return_line, _stack, symbols, stack_base);
            param_counter++;
        }
        stack_base.PushBack(next_stack_base);

        if (!call.is_syscall)
        {
            instruction = function_inst.next;

            do
            {
                return_line = RunInstruction(program, calls, symbols, _stack, instruction, stack_base, was_return_called);
            } while (0 != instruction && !was_return_called);
        }
        else
        {
            SysCall_f func = (SysCall_f)call.start_instruction;
            return_line = func(_stack, stack_base, symbols, param_counter, params);
        }
        stack_base.PopBack();
        _stack->Release(call.stack_size);

        return return_line;
    }

    void AddSyscall(SysCall_f _syscall, Vector<Symbol>& symbols, Vector<Function>& calls, const char* name)
    {
        Function func;

        func.is_syscall = 1;
        func.start_instruction = (size_t)_syscall;

        calls.PushBack(func);
        int index_call = calls.Size() - 1;

        Symbol sym;
        sym.sym_type = SYM_FUNCTION;
        sym.symbol_value = index_call;
        memset(sym.name, 0, sizeof(sym.name));
        strncpy_s(sym.name, name, sizeof(sym.name));

        symbols.PushBack(sym);
    }
}