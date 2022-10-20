
#pragma once

#include <cstddef>

namespace ttf
{

	//---------------------------------------------------------------------------------
	struct AssertInterceptedException
	{
	};

	//---------------------------------------------------------------------------------

	struct ITestInstance
	{
	public:
		virtual void run() = 0;
	public:
		ITestInstance* parent = nullptr;
		const char* name = nullptr;
	};

	//---------------------------------------------------------------------------------

	template <class T>
	struct TestInstance : public ITestInstance
	{
	public:
		const T& func;

		inline TestInstance(const T& _func)
			:func(_func)
		{
		}

		virtual void run() override
		{
			func();
		}
	};

	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------

	class Context
	{
	public:
		static Context* context;
		static int entrypoint(const char** argv, std::size_t argc, void(*main_func)());
	public:
		template <class F>
		inline void test(const char* name, const F& f)
		{
			TestInstance<F> t(f);
			t.name = name;
			run_test_instance(t);
		}

		struct inline_test
		{
			const char* name;
			inline_test(const char* n) : name(n) {}
			template <class F>
			void operator = (const F& _func)
			{
				ttf::Context::context->test(name, _func);
			}
		};
	public:
		virtual void run_test_instance(ITestInstance& t) = 0;
		virtual bool intercept_assert(const bool expr_value, const char* expr, const char* file, const int line) = 0;//returns true if the execution should stop (when a debugger is attached for example)
		virtual bool intercept_assert(const bool expr_value, const char* expr, const char* file, const int line, const char * format, ...) = 0;//returns true if the execution should stop (when a debugger is attached for example)
	};
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	
#define TEST_MAIN(F) \
int main(int argc, const char** argv) \
{ \
	return ttf::Context::entrypoint(argv, std::size_t(argc), F); \
}

#define TEST_FUNCTION(FUNC) \
	ttf::Context::context->test(#FUNC, [&](){FUNC();})

#define TEST_INLINE() \
	ttf::Context::inline_test(__FUNCTION__)

	

#ifdef _MSC_VER
#	define TTF_STOP_DEBUGGER() \
		do                    \
		{                     \
			__debugbreak();   \
		} while (false)
#	pragma warning(disable : 4464) // warning C4464 : relative include path contains '..'
#endif

#if defined(__GNUC__) && !defined(__APPLE__)
#	define TTF_STOP_DEBUGGER() \
		do                    \
		{                     \
			__builtin_trap(); \
		} while (false)
#endif

#define TTF_ASSERT(COND) \
		do \
		{ \
			bool _test_assert_failed = false; \
			if (!(COND)) \
				_test_assert_failed = true; \
			if (ttf::Context::context->intercept_assert(_test_assert_failed, #COND, __FILE__, __LINE__)) { \
				TTF_STOP_DEBUGGER(); \
			} \
		} while (false)

#define TTF_ASSERT_DESC(COND, ...) \
		do \
		{ \
			bool _test_assert_failed = false; \
			if (!(COND)) \
				_test_assert_failed = true; \
			if (ttf::Context::context->intercept_assert(_test_assert_failed, #COND, __FILE__, __LINE__, ##__VA_ARGS__)) { \
				TTF_STOP_DEBUGGER(); \
			} \
		} while (false)


#define TEST_ASSERT TTF_ASSERT


//#define GLOBAL_TTF_STATIC

}