#pragma once

#include "completion_signatures.hpp"
#include "config.hpp"
#include "cpos.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include "prologue.hpp"

namespace ustdex
{
    namespace detail
    {
        // A type that describes a sender's metadata
        template <class _Tag, class _Data, class... _Child>
        struct __desc
        {
            using __tag = _Tag;
            using __data = _Data;
            using __children = _m_list<_Child...>;

            template <class _Fn>
            using call = _m_call<_Fn, _Tag, _Data, _Child...>;
        };
    }

    template <
        class _Descriptor,
        auto _DescriptorFn =
        []
        {
            return _Descriptor();
        }
    >
    inline constexpr auto __descriptor_fn_v = _DescriptorFn;

    template <class _Tag, class _Data, class... _Child>
    inline constexpr auto __descriptor_fn()
    {
        return __descriptor_fn_v<detail::__desc<_Tag, _Data, _Child...>>;
    }

    namespace
    {
        //! A struct template to aid in creating senders.
        //! This struct closely resembles P2300's [_`basic-sender`_](https://eel.is/c++draft/exec#snd.expos-24),
        //! but is not an exact implementation.
        //! Note: The struct named `__basic_sender` is just a dummy type and is also not _`basic-sender`_.
        template <auto _DescriptorFn>
        struct __sexpr
        {
            using sender_concept = sender_t;

            // See MAINTAINERS.md#class-template-parameters for `__id` and `__t`.
            /*using __id = __sexpr;
            using __t = __sexpr;
            using __desc_t = decltype(_DescriptorFn());
            using __tag_t = typename __desc_t::__tag;
            using __captures_t = __minvoke<__desc_t, __q<__detail::__captures_t>>;

            mutable __captures_t __impl_;*/

            template <class _Tag, class _Data, class... _Child>
            explicit __sexpr(_Tag, _Data&& __data, _Child&&... __child)
                /*: __impl_(
                    __detail::__captures(
                        _Tag(),
                        static_cast<_Data&&>(__data),
                        static_cast<_Child&&>(__child)...))*/
            {}

            //template <class _Self>
            //using __impl = __sexpr_impl<__meval<__msecond, _Self, __tag_t>>;

            //template <class _Self = __sexpr>
            //STDEXEC_ATTRIBUTE(always_inline)
            //    auto get_env() const noexcept
            //    -> __result_of<__sexpr_apply, const _Self&, __get_attrs_fn<__tag_t>>
            //{
            //    return __sexpr_apply(*this, __detail::__drop_front(__impl<_Self>::get_attrs));
            //}

            //template <__decays_to<__sexpr> _Self, class... _Env>
            //STDEXEC_ATTRIBUTE(always_inline)
            //    static auto get_completion_signatures(_Self&&, _Env&&...) noexcept -> __msecond<
            //    __if_c<__decays_to<_Self, __sexpr>>,
            //    __result_of<__impl<_Self>::get_completion_signatures, _Self, _Env...>
            //    >
            //{
            //    return {};
            //}

            //// BUGBUG fix receiver constraint here:
            //template <__decays_to<__sexpr> _Self, /*receiver*/ class _Receiver>
            //STDEXEC_ATTRIBUTE(always_inline)
            //    static auto connect(_Self&& __self, _Receiver&& __rcvr)
            //    noexcept(__noexcept_of<__impl<_Self>::connect, _Self, _Receiver>) -> __msecond<
            //    __if_c<__decays_to<_Self, __sexpr>>,
            //    __result_of<__impl<_Self>::connect, _Self, _Receiver>
            //    >
            //{
            //    return __impl<_Self>::connect(
            //        static_cast<_Self&&>(__self), static_cast<_Receiver&&>(__rcvr));
            //}

            //template <__decays_to<__sexpr> _Self, /*receiver*/ class _Receiver>
            //STDEXEC_ATTRIBUTE(always_inline)
            //    static auto submit(_Self&& __self, _Receiver&& __rcvr)
            //    noexcept(__noexcept_of<__impl<_Self>::submit, _Self, _Receiver>) -> __msecond<
            //    __if_c<__decays_to<_Self, __sexpr>>,
            //    __result_of<__impl<_Self>::submit, _Self, _Receiver>
            //    >
            //{
            //    return __impl<_Self>::submit(
            //        static_cast<_Self&&>(__self), static_cast<_Receiver&&>(__rcvr));
            //}

            //template <class _Sender, class _ApplyFn>
            //STDEXEC_ATTRIBUTE(always_inline)
            //    static auto apply(_Sender&& __sndr, _ApplyFn&& __fun) noexcept(
            //        __nothrow_callable<__detail::__impl_of<_Sender>, __copy_cvref_fn<_Sender>, _ApplyFn>)
            //    -> __call_result_t<__detail::__impl_of<_Sender>, __copy_cvref_fn<_Sender>, _ApplyFn>
            //{
            //    return static_cast<_Sender&&>(__sndr)
            //        .__impl_(__copy_cvref_fn<_Sender>(), static_cast<_ApplyFn&&>(__fun));
            //}

            //template <std::size_t _Idx, __decays_to_derived_from<__sexpr> _Self>
            //STDEXEC_ATTRIBUTE(always_inline)
            //    friend auto get(_Self&& __self) noexcept -> decltype(auto)
            //    requires __detail::__in_range<_Idx, __desc_t>
            //{
            //    if constexpr (_Idx == 0)
            //    {
            //        return __tag_t();
            //    }
            //    else
            //    {
            //        return __self.__impl_(__copy_cvref_fn<_Self>(), __nth_pack_element<_Idx>);
            //    }
            //}
        };

        template <class _Tag, class _Data, class... _Child>
        __sexpr(_Tag, _Data, _Child...)->__sexpr<__descriptor_fn<_Tag, _Data, _Child...>()>;
    } // namespace

	template <class _Tag, class _Data, class... _Child>
	using _sexpr_t = __sexpr<__descriptor_fn<_Tag, _Data, _Child...>()>;

	namespace detail
	{
		template <class _Tag>
		struct _make_sexpr_t
		{
			template <class _Data = __, class... _Child>
			constexpr auto operator()(_Data _data = {}, _Child... _child) const
			{
				return _sexpr_t<_Tag, _Data, _Child...>{
					_Tag(), static_cast<_Data&&>(_data), static_cast<_Child&&>(_child)...};
			}
		};
	} // namespace __detail

	template <class _Tag>
	inline constexpr detail::_make_sexpr_t<_Tag> _make_sexpr{};
}

#include "epilogue.hpp"