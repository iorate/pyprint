
// pyprint
//
// Copyright iorate 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef PYPRINT_HPP
#define PYPRINT_HPP

#include <cstddef>
#include <iostream>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pyprint {

namespace detail {

template <class Stream, class T> // !is_reference_v<Stream>
struct is_printable;

template <class T, class Char>
struct is_char_array : std::is_same<std::remove_const_t<std::remove_extent_t<T>>, Char> {};

template <class Stream, class T, class = void>
struct is_insertable : std::false_type {};

template <class Stream, class T>
struct is_insertable<
  Stream,
  T,
  std::void_t<decltype(std::declval<Stream &>() << std::declval<T>())>
> :
  std::bool_constant<
    !std::is_array_v<std::remove_reference_t<T>> ||
    is_char_array<std::remove_reference_t<T>, typename Stream::char_type>::value ||
    is_char_array<std::remove_reference_t<T>, char>::value ||
    is_char_array<std::remove_reference_t<T>, signed char>::value ||
    is_char_array<std::remove_reference_t<T>, unsigned char>::value
  > {
};

template <class Stream, class T, class = void>
struct is_printable_range : std::false_type {};

template <class Stream, class T>
struct is_printable_range<
  Stream,
  T,
  std::void_t<typename std::iterator_traits<decltype(std::begin(std::declval<T>()))>::reference>
> :
  is_printable<
    Stream,
    typename std::iterator_traits<decltype(std::begin(std::declval<T>()))>::reference
  > {
};

template <class T, class = void> // !is_reference_v<T>
struct is_set : std::false_type {};

template <class T>
struct is_set<T, std::void_t<typename T::key_type, typename T::value_type>> :
  std::is_same<typename T::key_type, typename T::value_type> {
};

template <class Stream, class T>
struct is_printable_set :
  std::bool_constant<
    is_printable<Stream, T>::value && is_set<typename std::remove_reference_t<T>>::value
  > {
};

template <class T, class = void> // !is_reference_v<T>
struct is_map : std::false_type {};

template <class T>
struct is_map<
  T,
  std::void_t<typename T::key_type, typename T::mapped_type, typename T::value_type>
> :
  std::bool_constant<
    std::is_same_v<
      std::pair<typename T::key_type const, typename T::mapped_type>,
      typename T::value_type
    > ||
    std::is_same_v<
      std::pair<typename T::key_type, typename T::mapped_type>,
      typename T::value_type
    >
  > {
};

template <class Stream, class T>
struct is_printable_map :
  std::bool_constant<
    is_printable<Stream, T>::value && is_map<typename std::remove_reference_t<T>>::value
  > {
};

using std::get;

template <std::size_t I, class T>
constexpr decltype(get<I>(std::declval<T>())) tuple_get(T &&t) {
  return get<I>(std::forward<T>(t));
}

template <class Stream, class T, class I, class = void>
struct is_printable_tuple_i : std::false_type {};

template <class Stream, class T, std::size_t ...I>
struct is_printable_tuple_i<
  Stream,
  T,
  std::index_sequence<I...>,
  std::void_t<decltype(tuple_get<I>(std::declval<T>()))...>
> :
  std::bool_constant<
    (is_printable<Stream, decltype(tuple_get<I>(std::declval<T>()))>::value && ...)
  > {
};

template <class Stream, class T, class = void>
struct is_printable_tuple : std::false_type {};

template <class Stream, class T>
struct is_printable_tuple<
  Stream,
  T,
  std::void_t<decltype(std::tuple_size<std::remove_reference_t<T>>::value)>
> :
  is_printable_tuple_i<
    Stream,
    T,
    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>
  > {
};

template <class Stream, class T>
struct is_printable :
  std::bool_constant<
    is_insertable<Stream, T>::value ||
    is_printable_range<Stream, T>::value ||
    is_printable_tuple<Stream, T>::value
  > {
};

template <
  class Stream,
  class T,
  bool Insertable = is_insertable<Stream, T>::value,
  bool Set = is_printable_set<Stream, T>::value,
  bool Map = is_printable_map<Stream, T>::value,
  bool Range = is_printable_range<Stream, T>::value,
  bool Tuple = is_printable_tuple<Stream, T>::value
>
struct print_one_i;

template <class Stream, class T>
inline void print_one(Stream &stream, T &&t) {
  print_one_i<Stream, T>()(stream, std::forward<T>(t));
}

// is_insertable<Stream, T>::value
template <class Stream, class T, bool Set, bool Map, bool Range, bool Tuple>
struct print_one_i<Stream, T, true, Set, Map, Range, Tuple> {
  void operator()(Stream &stream, T &&t) const {
    stream << std::forward<T>(t);
  }
};

// is_printable_set<Stream, T>::value
template <class Stream, class T, bool Map, bool Range, bool Tuple>
struct print_one_i<Stream, T, false, true, Map, Range, Tuple> {
  void operator()(Stream &stream, T &&t) const {
    stream << '{';
    std::size_t index = 0;
    for (auto &&e : t) {
      if (index++ > 0) {
        stream << ", ";
      }
      print_one(stream, e); // !std::is_rvalue_refernce_v<decltype(e)>
    }
    stream << '}';
  }
};

// is_printable_map<Stream, T>::value
template <class Stream, class T, bool Range, bool Tuple>
struct print_one_i<Stream, T, false, false, true, Range, Tuple> {
  void operator()(Stream &stream, T &&t) const {
    stream << '{';
    std::size_t index = 0;
    for (auto &&e : t) {
      if (index++ > 0) {
        stream << ", ";
      }
      print_one(stream, e.first); // !std::is_rvalue_reference_v<decltype(e)>
      stream << ": ";
      print_one(stream, e.second);
    }
    stream << '}';
  }
};

// is_printable_range<Stream, T>::value
template <class Stream, class T, bool Tuple>
struct print_one_i<Stream, T, false, false, false, true, Tuple> {
  void operator()(Stream &stream, T &&t) const {
    stream << '[';
    std::size_t index = 0;
    for (auto &&e : t) {
      if (index++ > 0) {
        stream << ", ";
      }
      print_one(stream, std::forward<decltype(e)>(e));
    }
    stream << ']';
  }
};

// is_printable_tuple<Stream, T>::value
template <class Stream, class T>
struct print_one_i<Stream, T, false, false, false, false, true> {
  template <std::size_t I>
  void call_ii(Stream &stream, T &&t) const {
    if constexpr (I > 0) {
      stream << ", ";
    }
    print_one(stream, tuple_get<I>(std::forward<T>(t)));
  }

  template <std::size_t ...I>
  void call_i(Stream &stream, T &&t, std::index_sequence<I...>) const {
    (call_ii<I>(stream, std::forward<T>(t)), ...);
  }

  void operator()(Stream &stream, T &&t) const {
    stream << '(';
    call_i(
      stream,
      std::forward<T>(t),
      std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>()
    );
    stream << ')';
  }
};

constexpr std::size_t npos = static_cast<std::size_t>(-1);

template <class Arg>
struct sep_arg {
  Arg &&arg;
};

template <class Arg>
struct end_arg {
  Arg &&arg;
};

template <class Arg>
struct file_arg {
  Arg &&arg;
};

template <class Arg>
struct flush_arg {
  Arg &&arg;
};

template <std::size_t SepI, class Args>
inline decltype(auto) get_sep([[maybe_unused]] Args &&args) {
  if constexpr (SepI == npos) {
    return " ";
  } else {
    return std::forward<decltype(std::get<SepI>(std::move(args)).arg)>(
      std::get<SepI>(std::move(args)).arg
    );
  }
}

template <std::size_t EndI, class Args>
inline decltype(auto) get_end([[maybe_unused]] Args &&args) {
  if constexpr (EndI == npos) {
    return "\n";
  } else {
    return std::forward<decltype(std::get<EndI>(std::move(args)).arg)>(
      std::get<EndI>(std::move(args)).arg
    );
  }
}

template <std::size_t FileI, class Args>
inline decltype(auto) get_file([[maybe_unused]] Args &&args) {
  if constexpr (FileI == npos) {
    return (std::cout); // is_same_v<decltype((std::cout)), std::ostream &>
  } else {
    return std::forward<decltype(std::get<FileI>(std::move(args)).arg)>(
      std::get<FileI>(std::move(args)).arg
    );
  }
}

template <std::size_t FlushI, class Args>
inline decltype(auto) get_flush([[maybe_unused]] Args &&args) {
  if constexpr (FlushI == npos) {
    return false;
  } else {
    return std::forward<decltype(std::get<FlushI>(std::move(args)).arg)>(
      std::get<FlushI>(std::move(args)).arg
    );
  }
}

template <class Result, std::size_t I, class ...Args>
struct analyze_args_i;

// Workaround for silly MSVC
template <class Result, std::size_t I, class ...Args>
struct analyze_args_ii;

template <
  std::size_t SepI,
  std::size_t EndI,
  std::size_t FileI,
  std::size_t FlushI,
  std::size_t ...ObjectI,
  std::size_t I,
  class ObjectArg,
  class ...Args
>
struct analyze_args_ii<
  std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI...>,
  I,
  ObjectArg,
  Args...
> :
  analyze_args_i<std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI..., I>, I + 1, Args...> {
  static_assert(
    SepI == npos && EndI == npos && FileI == npos && FlushI == npos,
    "positional argument after keyword argument"
  );
};

template <class Result, std::size_t I, class ...Args>
struct analyze_args_i :
  analyze_args_ii<Result, I, Args...> {
};

template <class Result, std::size_t I>
struct analyze_args_i<Result, I> {
  using type = Result;
};

template <
  std::size_t SepI,
  std::size_t EndI,
  std::size_t FileI,
  std::size_t FlushI,
  std::size_t ...ObjectI,
  std::size_t I,
  class SepArg,
  class ...Args
>
struct analyze_args_i<
  std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI...>,
  I,
  sep_arg<SepArg>,
  Args...
> :
  analyze_args_i<std::index_sequence<I, EndI, FileI, FlushI, ObjectI...>, I + 1, Args...> {
  static_assert(SepI == npos, "keyword argument repeated");
};

template <
  std::size_t SepI,
  std::size_t EndI,
  std::size_t FileI,
  std::size_t FlushI,
  std::size_t ...ObjectI,
  std::size_t I,
  class EndArg,
  class ...Args
>
struct analyze_args_i<
  std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI...>,
  I,
  end_arg<EndArg>,
  Args...
> :
  analyze_args_i<std::index_sequence<SepI, I, FileI, FlushI, ObjectI...>, I + 1, Args...> {
  static_assert(EndI == npos, "keyword argument repeated");
};

template <
  std::size_t SepI,
  std::size_t EndI,
  std::size_t FileI,
  std::size_t FlushI,
  std::size_t ...ObjectI,
  std::size_t I,
  class FileArg,
  class ...Args
>
struct analyze_args_i<
  std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI...>,
  I,
  file_arg<FileArg>,
  Args...
> :
  analyze_args_i<std::index_sequence<SepI, EndI, I, FlushI, ObjectI...>, I + 1, Args...> {
  static_assert(FileI == npos, "keyword argument repeated");
};

template <
  std::size_t SepI,
  std::size_t EndI,
  std::size_t FileI,
  std::size_t FlushI,
  std::size_t ...ObjectI,
  std::size_t I,
  class FlushArg,
  class ...Args>
struct analyze_args_i<
  std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI...>,
  I,
  flush_arg<FlushArg>,
  Args...
> :
  analyze_args_i<std::index_sequence<SepI, EndI, FileI, I, ObjectI...>, I + 1, Args...> {
  static_assert(FlushI == npos, "keyword argument repeated");
};

template <class ...Args>
struct analyze_args :
  analyze_args_i<std::index_sequence<npos, npos, npos, npos>, 0, Args...> {
};

template <class Args, class ObjectI, class Sep, class End, class File, class Flush, class = void>
struct is_printable_args_ii;

template <class Args, std::size_t ...ObjectI, class Sep, class End, class File, class Flush>
struct is_printable_args_ii<Args, std::index_sequence<ObjectI...>, Sep, End, File, Flush> :
  std::bool_constant<
    (
      is_printable<std::remove_reference_t<File>, std::tuple_element_t<ObjectI, Args>>::value &&
      ...
    ) &&
    is_printable<std::remove_reference_t<File>, Sep &>::value &&
    is_printable<std::remove_reference_t<File>, End &>::value &&
    std::is_convertible_v<Flush &, bool>
  > {
};

template <class Args, class AnalyzedI>
struct is_printable_args_i;

template <
  class Args,
  std::size_t SepI,
  std::size_t EndI,
  std::size_t FileI,
  std::size_t FlushI,
  std::size_t ...ObjectI
>
struct is_printable_args_i<Args, std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI...>> :
  is_printable_args_ii<
    Args,
    std::index_sequence<ObjectI...>,
    decltype(get_sep<SepI>(std::declval<Args>())),
    decltype(get_end<EndI>(std::declval<Args>())),
    decltype(get_file<FileI>(std::declval<Args>())),
    decltype(get_flush<FlushI>(std::declval<Args>()))
  > {
};

template <class ...Args>
struct is_printable_args :
  is_printable_args_i<std::tuple<Args...>, typename analyze_args<Args...>::type> {
};

template <std::size_t ObjectI, class Args, class Sep, class File>
inline void print_iv(Args &&args, [[maybe_unused]] Sep &sep, File &file) {
  if constexpr (ObjectI > 0) {
    file << sep;
  }
  print_one(file, std::get<ObjectI>(std::move(args)));
}

template <class Args, std::size_t ...ObjectI, class Sep, class End, class File, class Flush>
inline void print_iii(
  Args &&args,
  std::index_sequence<ObjectI...>,
  Sep &&sep,
  End &&end,
  File &&file,
  Flush &&flush
) {
  (print_iv<ObjectI>(std::move(args), sep, file), ...);
  file << end;
  if (flush) {
    file << std::flush;
  }
}

template <
  class Args,
  std::size_t SepI,
  std::size_t EndI,
  std::size_t FileI,
  std::size_t FlushI,
  std::size_t ...ObjectI
>
inline void print_ii(Args &&args, std::index_sequence<SepI, EndI, FileI, FlushI, ObjectI...>) {
  print_iii(
    std::move(args),
    std::index_sequence<ObjectI...>(),
    get_sep<SepI>(std::move(args)),
    get_end<EndI>(std::move(args)),
    get_file<FileI>(std::move(args)),
    get_flush<FlushI>(std::move(args))
  );
}

template <class ...Args>
inline void print_i(Args &&...args) {
  print_ii(
    std::forward_as_tuple(std::forward<Args>(args)...),
    typename analyze_args<Args...>::type()
  );
}

} // namespace detail

inline namespace keywords {

constexpr struct {
  template <class Arg>
  constexpr detail::sep_arg<Arg> operator=(Arg &&arg) const {
    return { std::forward<Arg>(arg) };
  }
} _sep = {};

constexpr struct {
  template <class Arg>
  constexpr detail::end_arg<Arg> operator=(Arg &&arg) const {
    return { std::forward<Arg>(arg) };
  }
} _end = {};

constexpr struct {
  template <class Arg>
  constexpr detail::file_arg<Arg> operator=(Arg &&arg) const {
    return { std::forward<Arg>(arg) };
  }
} _file = {};

constexpr struct {
  template <class Arg>
  constexpr detail::flush_arg<Arg> operator=(Arg &&arg) const {
    return { std::forward<Arg>(arg) };
  }
} _flush = {};

} // namespace keywords

template <class ...Args>
inline std::enable_if_t<detail::is_printable_args<Args...>::value> print(Args &&...args) {
  return detail::print_i(std::forward<Args>(args)...);
}

} // namespace pyprint

#endif
