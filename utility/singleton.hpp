#pragma once
namespace dimanari
{
	class noncopyable
	{
	public:
		noncopyable() = default;
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator=(const noncopyable&) = delete;
	};

	template<class T>
	class Singleton : private noncopyable
	{
	public:
		static T* GetInstance()
		{
			static T* temp = new T;
			return temp;
		}
	private:
		Singleton() = delete;
	};
}

#define STRINGIFY(X) #X
#define ENUMIFY(X, Y) X##Y##_e

#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)                                    \
  __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))


#define FOR_EACH2(macro, par, ...)                                    \
  __VA_OPT__(EXPAND(FOR_EACH_HELPER_2(macro, par, __VA_ARGS__)))

#define FOR_EACH_HELPER_2(macro, par, a1, ...)                         \
  macro(par, a1)                                                     \
  __VA_OPT__(, FOR_EACH_AGAIN_2 PARENS (macro, par, __VA_ARGS__))

#define FOR_EACH_HELPER(macro, a1, ...)                         \
  macro(a1)                                                     \
  __VA_OPT__(, FOR_EACH_AGAIN PARENS (macro, __VA_ARGS__))

#define FOR_EACH_AGAIN() FOR_EACH_HELPER
#define FOR_EACH_AGAIN_2() FOR_EACH_HELPER_2


#define MAKE_ENUM_AND_STRINGS(name, ...) \
    enum name##_e{ __VA_ARGS__ }; \
    constexpr const char* name##_strings[] = { \
         FOR_EACH(STRINGIFY,__VA_ARGS__) \
    }; constexpr int name##_amount = sizeof(name##_strings) / sizeof(char*);


#define MAKE_ENUM_AND_STRINGS_2(name, pattern, ...) \
    enum name##_e{ FOR_EACH2(ENUMIFY, pattern,__VA_ARGS__) }; \
    constexpr const char* name##_strings[] = { \
         FOR_EACH(STRINGIFY,__VA_ARGS__) \
    }; constexpr int name##_amount = sizeof(name##_strings) / sizeof(char*);