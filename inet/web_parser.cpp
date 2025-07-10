#include <c_snek/code_compiler.hpp>
#include "web_parser.hpp"
namespace dimanari
{
    int CompareLower(std::string source, std::string contains)
    {
        if (source.size() < contains.size())
            return false;
        for (int i = 0; i < contains.size(); ++i)
        {
            if (tolower(source[i]) != contains[i])
                return false;
        }
        return true;
    }

    int WebParser::Parse(std::string& file_content, std::vector<Cookie>& cookies, std::vector<std::string> data)
    {
        std::string answer;
        int marker = 0;
        while (marker < file_content.size())
        {
            // copy HTML until next Code block
            std::string temp_buffer = "";
            while (marker < file_content.size() && '<' != file_content[marker])
            {
                temp_buffer += file_content[marker++];
            }
            answer += temp_buffer;
            std::string tag = "<";
            //read tag
            while (marker < file_content.size() && '>' != file_content[marker])
            {
                tag += file_content[++marker];
            }
            marker++;

            if (CompareLower(tag, "<server"))
            {
                std::string CodeContent = "";
                bool code_continue = true;
                while (code_continue && marker < file_content.size())
                {
                    while (marker < file_content.size() && '<' != file_content[marker])
                    {
                        CodeContent += file_content[marker++];
                    }
                    std::string sub_tag = "<";
                    //read tag
                    const char* str_tag = "</server";
                    while (marker < file_content.size() && '>' != file_content[marker])
                    {
                        sub_tag += file_content[++marker];
                        if (sub_tag.size() < sizeof(str_tag) && sub_tag[sub_tag.size() - 1] != str_tag[sub_tag.size() - 1])
                        {
                            break;
                        }
                    }
                    marker++;

                    if (CompareLower(sub_tag, "</server"))
                    {
                        code_continue = false;
                    }
                    else
                    {
                        CodeContent += sub_tag;
                    }
                }
                if (code_continue)
                {
                    // broken code - do not execute
                    answer += "<p>UNEXPECTED END OF FILE</p>";
                }
                else
                {
                    answer += Execute(CodeContent, cookies, data);
                }
            }
            else
            {
                answer += tag;
            }
        }

        file_content = answer;
        return answer.size();
    }

    std::string WebParser::Execute(std::string CodeContent, std::vector<Cookie>& cookies, std::vector<std::string> data)
    {
        Compiler resolve_code;
        std::string out_string = "";
        std::string query_string = "";
        std::vector<std::string> query_results;
        resolve_code.Init();

        // add specialized syscalls

        resolve_code.AddExternHandler("out_string", (size_t)&out_string);
        resolve_code.AddExternHandler("cookies", (size_t)&cookies);
        //OpenDatabase
        resolve_code.AddExternHandler("query_prompt", (size_t)&query_string);
        resolve_code.AddExternHandler("query_results", (size_t)&query_results);
        //resolve_code.AddExternHandler("database", (size_t)g_database_handler);

        resolve_code.AddDefSyscalls();
        for (auto& cookie : cookies)
            resolve_code.AddVaraible(cookie.name, cookie.value);
        resolve_code.AddVaraibles(data);

        resolve_code.CompileCode(CodeContent);
        FILE* debug_log;
        int err = fopen_s(&debug_log, "execute_code.log", "wt");
        resolve_code.DebugViewer(debug_log);
        if (debug_log)
            fclose(debug_log);
        resolve_code.Run();


        resolve_code.Clean();
        return out_string;
    }
}