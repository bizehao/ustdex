#pragma once

#include <ustdex/ustdex.hpp>

struct parallel_sort_t
{
private:
	struct exec_then
	{
		template<typename... Args>
		void operator()(Args&&... args)
		{
			_fun_();
		}
		std::function<void()> _fun_;
	};

	template<class Rcvr, class Sch, class RandomAccessIterator, typename SortFun>
	struct _opstate_t
	{
		using operation_state_concept = ustdex::operation_state_t;
		using _env_t = ustdex::env_of_t<Rcvr>;

		_opstate_t(Rcvr&& rcvr, Sch sch, RandomAccessIterator begin, RandomAccessIterator end, SortFun sort_fun)
			: _rcvr_{ std::move(rcvr) }, _sch_{ sch }, _begin_{ begin }, _end_{ end }, _sort_fun_{ sort_fun }
			, _opstate_(ustdex::connect(ustdex::schedule(_sch_), ustdex::_rcvr_ref{ *this }))
		{}
		void start() noexcept
		{
			ustdex::start(_opstate_);
		}
		void set_value() noexcept;

		void set_error(std::exception_ptr) noexcept
		{
			ustdex::set_error(std::move(_rcvr_), std::current_exception());
		}
		void set_stopped() noexcept
		{
			ustdex::set_stopped(std::move(_rcvr_));
		}
		auto get_env() const noexcept -> _env_t
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
		using sender_concept = ustdex::sender_t;

		template <class Self, class... Env>
		static constexpr auto get_completion_signatures() noexcept
		{
			return ustdex::completion_signatures<ustdex::set_value_t(RandomAccessIterator, RandomAccessIterator), ustdex::set_error_t(::std::exception_ptr)>();
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
public:
	template <class Sch, class RandomAccessIterator, typename SortFun>
	auto operator()(Sch sch, RandomAccessIterator begin, RandomAccessIterator end, SortFun sort_fun) const noexcept -> _sndr_t<Sch, RandomAccessIterator, SortFun>
	{
		return _sndr_t<Sch, RandomAccessIterator, SortFun>{std::move(sch), begin, end, std::move(sort_fun)};
	}
};

inline constexpr parallel_sort_t parallel_sort{};

template<class Rcvr, class Sch, class RandomAccessIterator, typename SortFun>
inline void parallel_sort_t::_opstate_t<Rcvr, Sch, RandomAccessIterator, SortFun>::set_value() noexcept
{
	auto n = _end_ - _begin_;
	if (n <= 16384)
	{
		std::sort(_begin_, _end_, _sort_fun_);
		ustdex::set_value(std::move(_rcvr_), std::move(_begin_), std::move(_end_));
	}
	else
	{
		auto mid = _begin_ + n / 2;
		auto left = parallel_sort(_sch_, _begin_, mid, _sort_fun_);
		auto right = parallel_sort(_sch_, mid, _end_, _sort_fun_);

		std::function<void()> fun = [begin = _begin_, mid, end = _end_, rcvr = std::move(_rcvr_)]() mutable
			{
				std::inplace_merge(begin, mid, end);
				ustdex::set_value(std::move(rcvr), begin, end);
			};
		auto task = ustdex::when_all(std::move(left), std::move(right)) | ustdex::then(exec_then{ std::move(fun) });

		ustdex::start_detached(std::move(task));
	}
}
