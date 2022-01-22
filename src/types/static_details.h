#pragma once

namespace Vultr
{
	template <typename T>
	inline constexpr bool is_trivial = __is_trivial(T);

	template <class T>
	inline constexpr bool is_l_value = false;

	template <class T>
	inline constexpr bool is_l_value<T &> = true;

	template <class T>
	inline constexpr bool is_r_value = false;
	template <class T>
	inline constexpr bool is_r_value<T &&> = true;

	template <typename T, typename U>
	inline constexpr bool is_same = false;

	template <typename T>
	inline constexpr bool is_same<T, T> = true;

	template <typename Base, typename Derived>
	inline constexpr bool is_base_of = __is_base_of(Base, Derived);

	template <class T>
	using add_const = const T;

	template <class T>
	struct __remove_const
	{
		using Type = T;
	};
	template <class T>
	struct __remove_const<const T>
	{
		using Type = T;
	};
	template <class T>
	using remove_const = typename __remove_const<T>::Type;

	template <class T>
	struct __remove_volatile
	{
		using Type = T;
	};

	template <class T>
	struct __remove_volatile<volatile T>
	{
		using Type = T;
	};

	template <typename T>
	using remove_volatile = typename __remove_volatile<T>::Type;

	template <class T>
	using remove_cv = remove_volatile<remove_const<T>>;

	template <typename... Ts>
	struct IndexOf;

	template <typename T, typename... R>
	struct IndexOf<T, T, R...>
	{
		static constexpr size_t index = 0;
	};

	template <typename T, typename F, typename... R>
	struct IndexOf<T, F, R...>
	{
		static constexpr size_t index = 1 + IndexOf<T, R...>::index;
	};

	template <typename... Ts>
	struct Contains;

	template <typename T, typename... R>
	struct Contains<T, T, R...>
	{
		static constexpr bool contains = true;
	};

	template <typename T, typename F, typename... R>
	struct Contains<T, F, R...>
	{
		static constexpr bool contains = Contains<T, R...>::contains;
	};

	template <typename T, typename F>
	struct Contains<T, F>
	{
		static constexpr bool contains = false;
	};
} // namespace Vultr