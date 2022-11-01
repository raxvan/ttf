
#include "ttf.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

#ifdef _WIN32
#	include <windows.h>
#endif

namespace ttf
{
	Context* Context::context = nullptr;
	class ContextImpl : public Context
	{
	public:
		std::size_t			active_assert_count = 0;
		std::vector<char>	active_error_log;
		ITestInstance*		active = nullptr;
		
		std::size_t 	started_count = 0;
		std::size_t 	passed_count = 0;
		std::size_t 	failed_count = 0;
		std::size_t 	skipped_count = 0;
		
	public:
		virtual void run_test_instance(ITestInstance& t) override
		{
			active_assert_count = 0;
			active_error_log.clear();

			t.parent = active;
			active = &t;

			run_active_test();

			active = active->parent;
		}
		virtual bool intercept_assert(const bool expr_failed, const char* expr, const char* file, const int line) override
		{
			active_assert_count++;
			if (expr_failed)
			{
				//assert failed:
				log_error_line("ASSERT failed at:%s(%d)", file, line);
				log_error_line("File:%s", expr);
				
				if (is_interractive())
				{
					flush_error_log();
					//user is running with debugger, let him handle the situation
					return true;
				}
				else
				{
					throw AssertInterceptedException{};
				}
			}
			return false;
		}
		virtual bool intercept_assert(const bool expr_failed, const char* expr, const char* file, const int line, const char * format, ...) override
		{
			active_assert_count++;
			if (expr_failed)
			{
				//assert failed:
				log_error_line("ASSERT failed at:%s(%d)", file, line);
				log_error_line("File:%s", expr);

				char	buffer[2048];
				va_list args;
				va_start(args, format);
				std::vsnprintf(buffer, sizeof(buffer), format, args);
				va_end(args);

				log_error_line("Info:%s", buffer);
				
				if (is_interractive())
				{
					flush_error_log();
					//user is running with debugger, let him handle the situation
					return true;
				}
				else
				{
					throw AssertInterceptedException{};
				}
			}
			return false;
		}
	public:
		ContextImpl(const char** /*argv*/, std::size_t /*argc*/)
		{
			Context::context = this;
		}
		~ContextImpl()
		{
			Context::context = nullptr;
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
		bool execute_active_test()
		{
			try
			{
				active->run();
			}
			catch (AssertInterceptedException&)
			{
				//failiure_message is populated in interception function
				return false;
			}
			catch (...)
			{
				log_error_line("Unknown exception!");
				return false;
			}
			return true;
		}

		void run_active_test()
		{
			std::cout << "\033[1;34m";
			std::cout << "\n-------------------------------------------------------" << std::endl;
			std::cout << "#" << started_count++ << " [";
			print_recursive(active);
			std::cout << "]";
			std::cout << "\033[0m";
			std::cout << std::endl;

			if (accept_active_test())
			{
				auto start = std::chrono::high_resolution_clock::now();
				bool r = execute_active_test();
				auto end = std::chrono::high_resolution_clock::now();
				uint64_t ns = std::chrono::duration<uint64_t, std::nano>(end - start).count();
				double ms = double(ns) / 1e+6;

				/*
				std::cout << "\033[1;34m";
				std::cout << "[";
				print_recursive(active);
				std::cout << "]";
				*/

				std::cout << std::endl;

				if (r)
				{
					passed_count++;
					std::cout << "\033[1;32m";
					std::cout << " - OK.      {" << std::fixed << std::setprecision(4) << ms << " ms}" << std::endl;
					
				}
				else
				{
					failed_count++;
					std::cout << "\033[1;31m";
					std::cout << " - FAILED!  {" << std::fixed << std::setprecision(4) << ms << " ms}" << std::endl;
				}

				flush_error_log();

			}
			else
			{
				skipped_count++;
				std::cout << "\033[1;33m";
				std::cout << " - SKIPPED ..." << std::endl;
			}

			std::cout << "\033[0m";
		}
		void flush_error_log()
		{
			if (active_error_log.size())
			{
				active_error_log.push_back('\0');
				std::cout << active_error_log.data();
				active_error_log.clear();
			}
		}
		bool accept_active_test()
		{
			return failed_count == 0;
		}
		void log_error_line(const char* fmt, ...)
		{
			va_list args_len;
			va_list args_data;

			va_start(args_len, fmt);
			va_copy(args_data, args_len);
			std::size_t sz = std::vsnprintf(nullptr, 0, fmt, args_len);
			va_end(args_len);

			active_error_log.push_back('\t');
			std::size_t index = active_error_log.size();
			active_error_log.resize(index + sz);
			std::vsnprintf(active_error_log.data() + index, sz, fmt, args_data);
			va_end(args_data);
			active_error_log.push_back('\n');
		}
		
	public:
		static void print_recursive(ITestInstance* f)
		{
			if (f->parent != nullptr)
			{
				print_recursive(f->parent);
				std::cout << "." << f->name;
			}
			else
			{
				std::cout << f->name;
			}
		}
	};
	int Context::entrypoint(const char** argv, std::size_t argc, void(*main_func)())
	{
		ContextImpl ctx(argv, argc);
		main_func();
		std::cout << std::endl << "---------------------------------------" << std::endl << std::endl;

		std::cout << "Asserts: " << ctx.active_assert_count << std::endl;
		std::cout << "Started: " << ctx.started_count << std::endl;
		std::cout << "Skipped: " << ctx.skipped_count << std::endl;
		std::cout << "Passed: " << ctx.passed_count << std::endl;
		std::cout << "Failed: " << ctx.failed_count << std::endl;

		std::cout << std::endl << "---------------------------------------" << std::endl;

		if (ctx.failed_count > 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}



	instance_counter::instance_counter()
	{
		m_share_ptr = new std::atomic<int>(0);
		*m_share_ptr++;
	}
	instance_counter::~instance_counter()
	{
		if (--(*m_share_ptr) == 0)
		{
			delete[] m_share_ptr;
		}
	}
	instance_counter::instance_counter(const instance_counter& other)
	{
		m_share_ptr = other.m_share_ptr;
		*m_share_ptr++;
	}
	instance_counter& instance_counter::operator = (const instance_counter& other)
	{
		this->~instance_counter();
		m_share_ptr = other.m_share_ptr;
		*m_share_ptr++;
		return (*this);
	}
	instance_counter::instance_counter(instance_counter&& other)
		:instance_counter()
	{
		swap(other);
	}
	instance_counter& instance_counter::operator = (instance_counter&& other)
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
