/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <iostream>

#include "ustdex/ustdex.hpp"
#include <variant>
#include <any>
#include <random>

using namespace ustdex;

template<typename... Args>
void _whatis();

struct sink
{
	using receiver_concept = receiver_t;

	void set_value() noexcept {}

	void set_value(int a) noexcept
	{
		std::printf("%d\n", a);
	}

	template <class... As>
	void set_value(As&&...) noexcept
	{
		std::puts("In sink::set_value(auto&&...)");
	}

	void set_error(std::exception_ptr) noexcept {}

	void set_stopped() noexcept {}

	[[nodiscard]]
	prop<get_stop_token_t, inplace_stop_token> get_env() const noexcept
	{
		static inplace_stop_source _stop_source_;
		return prop{ get_stop_token, _stop_source_.get_token() };
	}
};

template <class>
[[deprecated]] void print()
{}


static_assert(dependent_sender<decltype(read_env(_empty()))>);

struct parallel_sort_t
{
private:

	template<class Rcvr, class Sch, class RandomAccessIterator, typename SortFun>
	struct _opstate_t
	{
		using operation_state_concept = operation_state_t;
		using _env_t = env_of_t<Rcvr>;

		USTDEX_API _opstate_t(Rcvr&& rcvr, Sch sch, RandomAccessIterator begin, RandomAccessIterator end, SortFun sort_fun)
			: _rcvr_{ std::move(rcvr) }, _sch_{ sch }, _begin_{ begin }, _end_{ end }, _sort_fun_{ sort_fun }
			, _opstate_(ustdex::connect(ustdex::schedule(_sch_), ustdex::_rcvr_ref{ *this }))
		{}
		USTDEX_API void start() noexcept
		{
			ustdex::start(_opstate_);
		}
		USTDEX_API void set_value() noexcept;

		USTDEX_API void set_error(std::exception_ptr) noexcept
		{
			ustdex::set_error(std::move(_rcvr_), std::current_exception());
		}
		USTDEX_API void set_stopped() noexcept
		{
			ustdex::set_stopped(std::move(_rcvr_));
		}
		USTDEX_API auto get_env() const noexcept -> _env_t
		{
			return ustdex::get_env(_rcvr_);
		}

		Rcvr _rcvr_;
		Sch _sch_;
		RandomAccessIterator _begin_;
		RandomAccessIterator _end_;
		SortFun _sort_fun_;
		ustdex::connect_result_t<ustdex::schedule_result_t<Sch>, ustdex::_rcvr_ref<_opstate_t, _env_t>> _opstate_;
	};

	template <class Sch, class RandomAccessIterator, typename SortFun>
	struct _sndr_t
	{
		using sender_concept = sender_t;

		template <class Self, class... Env>
		static constexpr auto get_completion_signatures() noexcept
		{
			return completion_signatures<set_value_t(RandomAccessIterator, RandomAccessIterator), set_error_t(::std::exception_ptr)>();
		}
		template <class Rcvr>
		auto connect(Rcvr&& rcvr) && noexcept(ustdex::_nothrow_decay_copyable<Sch, RandomAccessIterator, SortFun>)
			-> _opstate_t<Rcvr, Sch, RandomAccessIterator, SortFun>
		{
			return _opstate_t<Rcvr, Sch, RandomAccessIterator, SortFun>(
				std::move(rcvr), std::move(_sch_), _begin_, _end_, std::move(_sort_fun_));
		}

		template <class Rcvr>
		auto connect(Rcvr&& rcvr) const& noexcept(ustdex::_nothrow_decay_copyable<Sch, RandomAccessIterator, SortFun>)
			-> _opstate_t<Rcvr, Sch, RandomAccessIterator, SortFun>
		{
			return _opstate_t<Rcvr, Sch, RandomAccessIterator, SortFun>(
				std::move(rcvr), _sch_, _begin_, _end_, _sort_fun_);
		}

		Sch _sch_;
		RandomAccessIterator _begin_;
		RandomAccessIterator _end_;
		SortFun _sort_fun_;
	};

	template <class Sch, class RandomAccessIterator, typename SortFun>
	auto operator()(Sch sch, RandomAccessIterator begin, RandomAccessIterator end, SortFun sort_fun) const noexcept -> _sndr_t<Sch, RandomAccessIterator, SortFun>
	{
		return _sndr_t<Sch, RandomAccessIterator, SortFun>{std::move(sch), begin, end, std::move(sort_fun)};
	}
};

inline constexpr parallel_sort_t parallel_sort{};

int main()
{
	ustdex::static_thread_pool thread_pool{};
	std::vector<int> vec;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(1, 300);
	std::generate(vec.begin(), vec.end(), [&]()
		{
			return dis(gen);
		});

	auto ps = parallel_sort(thread_pool.get_scheduler(), vec.begin(), vec.end(), [](const int& a, const int& b)
		{
			return a < b;
		});
	ustdex::sync_wait(std::move(ps));
	thread_pool.join();

#if 0
	auto task = read_env(get_stop_token)
		| then([](auto stop_token)
			{
				std::cout << "Stop token: " << stop_token.stop_requested() << '\n';
				return 42;
			})
		| then([](int i)
			{
				std::cout << "Value: " << i << '\n';
				return i + 1;
			});

	auto task2 = when_all(task);

	//using TT = completion_signatures_of_t<decltype(task)>;

	//_whatis<TT>();

	auto op = connect(task2, sink{});

	start(op);

	//auto [sch2] = sync_wait(read_env(get_scheduler)).value();
#endif

#if 0
	thread_context ctx;
	auto sch = ctx.get_scheduler();

	auto work = just(1, 2, 3) //
		| then([](int a, int b, int c)
			{
				std::printf("%d %d %d\n", a, b, c);
				return a + b + c;
			});
	auto s = starts_on(sch, std::move(work));
	static_assert(!dependent_sender<decltype(s)>);
	std::puts("Hello, world!");
	sync_wait(s);

	auto s3 = just(42) | let_value([](int a)
		{
			std::puts("here");
			return just(a + 1);
		});
	sync_wait(s3);

	auto [sch2] = sync_wait(read_env(get_scheduler)).value();

	auto [i1, i2] = sync_wait(when_all(just(42), just(43))).value();
	std::cout << i1 << ' ' << i2 << '\n';

	auto s4 = just(42) | then([](int) {}) | upon_error([](auto) { /*return 42;*/ });
	auto s5 = when_all(std::move(s4), just(42, 43), just(+"hello"));
	auto [i, j, k] = sync_wait(std::move(s5)).value();
	std::cout << i << ' ' << j << ' ' << k << '\n';

	auto s6 = sequence(just(42) | then([](int)
		{
			std::cout << "sequence sender 1\n";
		}),
		just(42) | then([](int)
			{
				std::cout << "sequence sender 2\n";
			}));
	sync_wait(std::move(s6));

	auto s7 =
		just(42)
		| conditional(
			[](int i)
			{
				return i % 2 == 0;
			},
			then([](int)
				{
					std::cout << "even\n";
				}),
			then([](int)
				{
					std::cout << "odd\n";
				}));
	sync_wait(std::move(s7));


	{
		inplace_stop_source stop_source;

		auto task = just(100)
			| then([&stop_source](int i)
				{
					stop_source.request_stop();
					std::cout << "111_Value: " << i << '\n';
					return i;
				})
			//| stop_when(stop_source.get_token())
			| stop_with([](const int& i)
				{
					if (i == 100)
					{
						return false;
					}
					else
					{
						return false;
					}
				})
			| then([](int i)
				{
					std::cout << "222_Value: " << i << '\n';
					return i;
				})
			| upon_stopped([]()
				{
					std::cout << "Stopped!\n";
				})
			| upon_error([](std::exception_ptr e)
				{
					try
					{
						std::rethrow_exception(e);
					}
					catch (const std::exception& ex)
					{
						std::cout << "Error: " << ex.what() << '\n';
					}
				});

		auto op = connect(std::move(task), sink{});
		start(op);
		//sync_wait(std::move(task));

	}

	{
		auto task = when_any(just(5), just('a'), just(3.14)) |
			then([](std::any v)
				{
					int aa = 0;
				});

		auto op = connect(std::move(task), sink{});
		start(op);

		//using TA = completion_signatures_of_t<decltype(task)>;
		//_m_self_or<_nil>::call
		//using TB = _value_types<TA, _tupl, _variant>;

		//_whatis<TB>();
	}


#endif
}
