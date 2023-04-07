#pragma once
#pragma once


/*
	The typelist definition
*/
template <typename... Ts>
struct typelist {};

/*
	Number of types in typelist
*/
namespace detail {
	template <typename Seq>
	struct size_impl;

	template <template <typename...> class Seq, typename... Ts>
	struct size_impl<Seq<Ts...>> {
		using type = std::integral_constant<std::size_t, sizeof...(Ts)>;
	};
}

template <typename Seq>
using size = typename detail::size_impl<Seq>::type;

/*
	Get type of first element
*/
namespace detail {
	template <typename Seq>
	struct front_impl;

	template <template <typename...> class Seq, typename T, typename... Ts>
	struct front_impl<Seq<T, Ts...>> {
		using type = T;
	};
}

template <typename Seq>
using front = typename detail::front_impl<Seq>::type;

/*
	Remove first element, creating a new type list
*/
namespace detail {
	template <typename Seq>
	struct pop_front_impl;

	template <template <typename...> class Seq, typename T, typename... Ts>
	struct pop_front_impl<Seq<T, Ts...>> {
		using type = typelist<Ts...>;
	};
}


template <typename Seq>
using pop_front = typename detail::pop_front_impl<Seq>::type;

/*
	Add an element to back of type list
*/
namespace detail {
	template <typename Seq, typename T>
	struct push_back_impl;

	template <template <typename...> class Seq, typename... Ts, typename T>
	struct push_back_impl<Seq<Ts...>, T> {
		using type = typelist<Ts..., T>;
	};
}

template <typename Seq, typename T>
using push_back = typename detail::push_back_impl<Seq, T>::type;



