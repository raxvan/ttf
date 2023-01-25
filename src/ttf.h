
#pragma once

#include <cstddef>
#include <atomic>
#include <string>

namespace ttf
{

	struct instance_counter
	{
	protected:
		std::atomic<int>* m_share_ptr = nullptr;

	public:
		instance_counter();
		~instance_counter();

		instance_counter(const instance_counter& other);
		instance_counter& operator=(const instance_counter& other);

		instance_counter(instance_counter&& other);
		instance_counter& operator=(instance_counter&& other);

		void swap(instance_counter& other);
		int	 share() const;
	};

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
		const char*	   name = nullptr;
	};

	//---------------------------------------------------------------------------------

	template <class T>
	struct TestInstance : public ITestInstance
	{
	public:
		const T& func;

		inline TestInstance(const T& _func)
			: func(_func)
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
		static int	entrypoint(const char** argv, std::size_t argc, void (*main_func)());
		static bool intercept_assert(const bool expr_value, const char* expr, const char* file, const int line);
		static bool intercept_assert(const bool expr_value, const char* expr, const char* file, const int line, const char* format, ...);
		static void run_test_instance(ITestInstance& t);


		static std::string read_text_file(const char* abs_path_to_file);
	public:
		template <class F>
		static void function_test(const char* name, const F& f)
		{
			TestInstance<F> t(f);
			t.name = name;
			run_test_instance(t);
		}

		struct inline_test
		{
			const char* name;
			inline_test(const char* n)
				: name(n)
			{
			}
			template <class F>
			void operator=(const F& _func)
			{
				Context::function_test(name, _func);
			}
		};
	};
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------

#define TEST_MAIN(F)                                                 \
	int main(int argc, const char** argv)                            \
	{                                                                \
		return ttf::Context::entrypoint(argv, std::size_t(argc), F); \
	}

#define TEST_FUNCTION(FUNC) ttf::Context::function_test(#FUNC, [&]() { FUNC(); })

#define TEST_INLINE() ttf::Context::inline_test(__FUNCTION__)

#ifdef _MSC_VER
#	define TTF_STOP_DEBUGGER() \
		do                      \
		{                       \
			__debugbreak();     \
		} while (false)
#	pragma warning(disable : 4464) // warning C4464 : relative include path contains '..'
#endif

#if defined(__GNUC__) && !defined(__APPLE__)
#	define TTF_STOP_DEBUGGER() \
		do                      \
		{                       \
			__builtin_trap();   \
		} while (false)
#endif

#define TTF_ASSERT(COND)                                                                    \
	do                                                                                      \
	{                                                                                       \
		bool _test_assert_failed = false;                                                   \
		if (!(COND))                                                                        \
			_test_assert_failed = true;                                                     \
		if (ttf::Context::intercept_assert(_test_assert_failed, #COND, __FILE__, __LINE__)) \
		{                                                                                   \
			TTF_STOP_DEBUGGER();                                                            \
		}                                                                                   \
	} while (false)

#define TTF_ASSERT_DESC(COND, ...)                                                                         \
	do                                                                                                     \
	{                                                                                                      \
		bool _test_assert_failed = false;                                                                  \
		if (!(COND))                                                                                       \
			_test_assert_failed = true;                                                                    \
		if (ttf::Context::intercept_assert(_test_assert_failed, #COND, __FILE__, __LINE__, ##__VA_ARGS__)) \
		{                                                                                                  \
			TTF_STOP_DEBUGGER();                                                                           \
		}                                                                                                  \
	} while (false)

#define TEST_ASSERT TTF_ASSERT

	//#define GLOBAL_TTF_STATIC

}
