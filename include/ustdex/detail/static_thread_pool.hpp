#pragma once

#include "thread_context.hpp"
#include <vector>
#include <random>

#  include "prologue.hpp"

namespace ustdex
{
	struct USTDEX_TYPE_VISIBILITY_DEFAULT static_thread_pool
	{
		static_thread_pool(int num = std::thread::hardware_concurrency()) noexcept
		{
			_loops_.resize(num);
			_thrds_.resize(num);

			for (int i = 0; i < num; i++)
			{
				_thrds_[i] = std::thread{ [this, i]()
					{
						_loops_[i].run();
					} };
			}
		}

		~static_thread_pool() noexcept
		{
			join();
		}

		void join() noexcept
		{
			for (auto& it : _thrds_)
			{
				if (it.joinable())
				{
					it.join();
				}
			}
		}

		auto get_scheduler()
		{
			int i = mt() % _loops_.size();
			return _loops_[i].get_scheduler();
		}

	private:
		std::mt19937 mt{ std::default_random_engine{}() };
		std::vector<std::thread> _thrds_;
		std::vector<run_loop> _loops_;
	};
} // namespace ustdex

#  include "epilogue.hpp"