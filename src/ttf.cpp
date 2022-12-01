
#include "ttf.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <mutex>
#ifdef _WIN32
#	include <windows.h>
#endif

namespace ttf
{

	struct active_test_data
	{
		std::ostringstream log;
		std::ostringstream err;

		ITestInstance* active = nullptr;

	public:
		void reset()
		{
			log.str("");
			log.clear();
			err.str("");
			err.clear();
		}

		void log_start(std::size_t index)
		{
			log << "\033[1;34m";
			log << "\n-------------------------------------------------------" << std::endl;
			log << "#" << index << " [";
			log_name_recursive(active);
			log << "]";
			log << "\033[0m";
		}

		void log_success(double ms)
		{
			log << "\033[1;32m";
			log << " - OK.      {" << std::fixed << std::setprecision(4) << ms << " ms}" << std::endl;
			log << "\033[0m";
		}

		void log_failiure(double ms)
		{
			log << "\033[1;31m";
			log << " - FAILED!  {" << std::fixed << std::setprecision(4) << ms << " ms}" << std::endl;
			log << "\033[0m";
		}

		void log_skip()
		{
			log << "\033[1;33m";
			log << " - SKIPPED ..." << std::endl;
			log << "\033[0m";
		}

		void flush_log()
		{
			std::cout << log.str() << std::endl;

			do
			{
				// log errors if any
				std::streampos pos = err.tellp(); // store current location
				err.seekp(0, std::ios_base::end); // go to end
				bool empty = (err.tellp() == 0);  // check size == 0 ?
				err.seekp(pos);

				if (empty)
					break;

				std::cerr << "ERRORS:\n" << err.str() << std::endl;
			} while (false);

			reset();
		}

	public:
		void log_name_recursive(ITestInstance* f)
		{
			if (f->parent != nullptr)
			{
				log_name_recursive(f->parent);
				log << "." << f->name;
			}
			else
			{
				log << f->name;
			}
		}
	};

	static active_test_data& get_active_test_state()
	{
		thread_local active_test_data s;
		return s;
	}

	class ContextImpl : public Context
	{
	public:
		std::atomic<std::size_t> active_assert_count = 0;
		std::atomic<std::size_t> started_count = 0;

		std::atomic<std::size_t> passed_count = 0;
		std::atomic<std::size_t> failed_count = 0;
		std::atomic<std::size_t> skipped_count = 0;

		std::mutex context_lock;

	public:
		ContextImpl()
		{
		}
		~ContextImpl()
		{
		}

		inline bool initialize(const char** /*argv*/, std::size_t /*argc*/)
		{
			static bool					initialized = false;
			std::lock_guard<std::mutex> _(context_lock);
			if (initialized == true)
			{
				std::cerr << "Internal testing error ..." << std::endl;
				failed_count++;
				return false;
			}
			initialized = true;
			return true;
		}

	public:
		inline bool is_interractive() const
		{
#ifdef _WIN32
			return IsDebuggerPresent();
#else
			return false;
#endif
		}
		bool execute_active_test(active_test_data& ac)
		{
			try
			{
				ac.active->run();
			}
			catch (AssertInterceptedException&)
			{
				// failiure_message is populated in interception function
				return false;
			}
			catch (...)
			{
				ac.err << "Unknown exception!" << std::endl;
				return false;
			}
			return true;
		}

		bool accept_test_instance(ITestInstance*)
		{
			return failed_count == 0;
		}
		void run_active_test(active_test_data& ac)
		{
			ac.log_start(started_count++);

			if (accept_test_instance(ac.active))
			{
				auto	 start = std::chrono::high_resolution_clock::now();
				bool	 r = execute_active_test(ac);
				auto	 end = std::chrono::high_resolution_clock::now();
				uint64_t ns = std::chrono::duration<uint64_t, std::nano>(end - start).count();
				double	 ms = double(ns) / 1e+6;

				if (r)
				{
					passed_count++;
					ac.log_success(ms);
				}
				else
				{
					failed_count++;
					ac.log_failiure(ms);
				}
			}
			else
			{
				skipped_count++;
				ac.log_skip();
			}

			{
				std::lock_guard<std::mutex> _(context_lock);
				ac.flush_log();
			}
		}
	};

	static ContextImpl& get_active_context()
	{
		static ContextImpl s {};
		return s;
	}

	void Context::run_test_instance(ITestInstance& t)
	{
		auto& ac = get_active_test_state();

		t.parent = ac.active;
		ac.active = &t;

		get_active_context().run_active_test(ac);

		ac.active = ac.active->parent;
	}
	bool Context::intercept_assert(const bool expr_failed, const char* expr, const char* file, const int line)
	{
		auto& context = get_active_context();

		context.active_assert_count++;
		if (expr_failed)
		{
			auto& ac = get_active_test_state();

			// assert failed:
			{
				ac.err << "ASSERT failed at:" << file << "(" << line << ")" << std::endl;
				ac.err << "EXPR:" << expr << std::endl;

				if (context.is_interractive())
				{
					// user is running with debugger, let him handle the situation
					std::lock_guard<std::mutex> _(context.context_lock);
					ac.flush_log();

					return true;
				}
			}

			{
				throw AssertInterceptedException {};
			}
		}
		return false;
	}
	bool Context::intercept_assert(const bool expr_failed, const char* expr, const char* file, const int line, const char* format, ...)
	{
		auto& context = get_active_context();

		context.active_assert_count++;
		if (expr_failed)
		{
			auto& ac = get_active_test_state();

			// assert failed:
			{
				char	buffer[2048];
				va_list args;
				va_start(args, format);
				std::vsnprintf(buffer, sizeof(buffer), format, args);
				va_end(args);

				ac.err << "ASSERT failed at:" << file << "(" << line << ")" << std::endl;
				ac.err << "EXPR:" << expr << std::endl;
				ac.err << "INFO:" << buffer << std::endl;

				if (context.is_interractive())
				{
					// user is running with debugger, let him handle the situation
					std::lock_guard<std::mutex> _(context.context_lock);
					ac.flush_log();
					return true;
				}
			}

			{
				throw AssertInterceptedException {};
			}
		}
		return false;
	}

	//--------------------------------------------------------------------------------------------------------------------------------
	std::atomic<int>& get_global_instance_counter()
	{
		static std::atomic<int> ginst{0};
		return ginst;
	}

	int Context::entrypoint(const char** argv, std::size_t argc, void (*main_func)())
	{
		auto& ctx = get_active_context();

		if (ctx.initialize(argv, argc))
			main_func();
		else
			std::cerr << "Internal testing error!" << std::endl;

		{
			// global tests
			if (get_global_instance_counter().load() != 0)
			{
				ctx.failed_count++;
				std::cerr << "Leaked instances detected!" << std::endl;
				return -3;
			}
		}

		std::cout << std::endl << "---------------------------------------" << std::endl << std::endl;
		std::cout << "Asserts: " << ctx.active_assert_count << std::endl;
		std::cout << "Started: " << ctx.started_count << std::endl;
		std::cout << "Skipped: " << ctx.skipped_count << std::endl;
		std::cout << "Passed: " << ctx.passed_count << std::endl;
		std::cout << "Failed: " << ctx.failed_count << std::endl;
		std::cout << std::endl << "---------------------------------------" << std::endl;

		int result = 0;
		if (ctx.failed_count > 0)
			result = -1;

		return result;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	instance_counter::instance_counter()
	{
		get_global_instance_counter()++;

		m_share_ptr = new std::atomic<int>(1);
	}
	instance_counter::~instance_counter()
	{
		get_global_instance_counter()--;
		auto ic = --(*m_share_ptr);
		TTF_ASSERT(ic >= 0);
		if (ic == 0)
		{
			delete[] m_share_ptr;
		}
	}
	instance_counter::instance_counter(const instance_counter& other)
	{
		get_global_instance_counter()++;
		m_share_ptr = other.m_share_ptr;
		(*m_share_ptr)++;
	}
	instance_counter::instance_counter(instance_counter&& other)
		: instance_counter()
	{
		swap(other);
	}

	instance_counter& instance_counter::operator=(const instance_counter& other)
	{
		instance_counter tmp(other);
		swap(tmp);
		return (*this);
	}
	instance_counter& instance_counter::operator=(instance_counter&& other)
	{
		instance_counter tmp;
		this->swap(tmp);
		this->swap(other);
		return (*this);
	}

	void instance_counter::swap(instance_counter& other)
	{
		std::swap(m_share_ptr, other.m_share_ptr);
	}

	int instance_counter::share() const
	{
		return m_share_ptr->load();
	}

}
