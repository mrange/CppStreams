// ----------------------------------------------------------------------------
// Copyright 2015 Mårten Rånge
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------
#ifndef CPP_STREAMS__INCLUDE_GUARD
# define CPP_STREAMS__INCLUDE_GUARD
// ----------------------------------------------------------------------------
# define CPP_STREAMS__CHECK_SOURCE(src)               \
  static_assert (                                     \
      detail::is_source<decltype (src)>::value        \
    , #src " must be a a cpp_streams source"          \
    )
# define CPP_STREAMS__PRELUDE constexpr
# define CPP_STREAMS__SINK    inline
# define CPP_STREAMS__BODY(type)            \
  type (type const &)             = default;\
  type (type &&)                  = default;\
  type& operator= (type const &)  = default;\
  type& operator= (type &&)       = default
// ----------------------------------------------------------------------------
# include <algorithm>
# include <map>
# include <type_traits>
# include <set>
# include <vector>
// ----------------------------------------------------------------------------
// Three kind of objects
//  1. Sources
//    Sources pushes data through the pipeline
//    Sources support `operator >>` which combines
//      a. Source + Sink => Aggregated result
//      b. Source + Pipe => Source
//  2. Sinks
//    Sinks aggregates data pushed to it
//    Sinks support method `consume` when passed a source function
//      consumes it and returns the aggregated result
//  3. Pipes
//    Pipes transforms data pushed through it
//    Pipes support method `consume` when passed a source function
//      returns a new Source
// ----------------------------------------------------------------------------

namespace cpp_streams
{

  // --------------------------------------------------------------------------

  namespace detail
  {

    // ------------------------------------------------------------------------

    constexpr auto default_vector_reserve = 16U;

    // ------------------------------------------------------------------------

    template<typename TValueType>
    struct strip_type
    {
      using type = std::remove_cv_t<std::remove_reference_t<TValueType>>  ;
    };

    template<class TValueType>
    using strip_type_t = typename strip_type<TValueType>::type;

    // ------------------------------------------------------------------------

    template<typename TValueType, typename TSourceFunction>
    struct source
    {
      using value_type = TValueType;

      TSourceFunction source_function;

      CPP_STREAMS__BODY (source);

      explicit CPP_STREAMS__PRELUDE source (TSourceFunction const & source_function)
        : source_function (source_function)
      {
      }

      explicit CPP_STREAMS__PRELUDE source (TSourceFunction && source_function)
        : source_function (std::move (source_function))
      {
      }

      template<typename TSink>
      CPP_STREAMS__PRELUDE auto operator >> (TSink && sink) const
      {
        return sink (*this);
      }
    };

    // Adapts a source function into a Source
    template<typename TValueType, typename TSourceFunction>
    CPP_STREAMS__PRELUDE auto adapt_source_function (TSourceFunction && source_function)
    {
      return source<TValueType, TSourceFunction> (std::forward<TSourceFunction> (source_function));
    }

    template<typename T>
    struct is_source_impl
    {
      enum
      {
        value = false,
      };
    };

    template<typename TValueType, typename TSource>
    struct is_source_impl<source<TValueType, TSource>>
    {
      enum
      {
        value = true,
      };
    };

    template<typename T>
    struct is_source
    {
      enum
      {
        value = is_source_impl<strip_type_t<T>>::value,
      };
    };

    template<typename T>
    struct get_source_value_type_impl;

    template<typename TValueType, typename TSource>
    struct get_source_value_type_impl<source<TValueType, TSource>>
    {
      using type = TValueType;
    };

    template<typename T>
    struct get_source_value_type
    {
      using type = typename get_source_value_type_impl<strip_type_t<T>>::type;
    };

    template<typename T>
    using get_source_value_type_t = typename get_source_value_type<T>::type;

    // WORKAROUND: A using alias should be enough but causes problems in VS2015 RC
    template<typename T>
    struct get_stripped_source_value_type
    {
      using type = typename strip_type<typename get_source_value_type<T>::type>::type;
    };

    template<typename T>
    using get_stripped_source_value_type_t = typename get_stripped_source_value_type<T>::type;

    // ------------------------------------------------------------------------

  }

  // --------------------------------------------------------------------------
  // Sources
  // --------------------------------------------------------------------------

  auto from_range = [] (auto && begin, auto && end)
  {
    using begin_type  = decltype (begin)      ;
    using end_type    = decltype (end)        ;
    using value_type  = decltype (*(&begin))  ;

    return detail::adapt_source_function<value_type> (
      [begin = std::forward<begin_type> (begin), end = std::forward<end_type> (end)] (auto && sink)
      {
        for (auto iter = begin; iter < end && sink (iter); ++iter)
          ;
      });
  };

  // --------------------------------------------------------------------------

  auto from_iterators = [] (auto && begin, auto && end)
  {
    using begin_type  = decltype (begin);
    using end_type    = decltype (end);
    using value_type  = decltype (*begin);

    static_assert (std::is_same<begin_type, end_type>::value, "begin and end should be of same type");

    return detail::adapt_source_function<value_type> (
      [begin = std::forward<begin_type> (begin), end = std::forward<end_type> (end)] (auto && sink)
      {
        for (auto iter = begin; iter != end && sink (*iter); ++iter)
          ;
      });
  };

  // --------------------------------------------------------------------------

  auto from = [] (auto && container)
  {
    return from_iterators (container.begin (), container.end ());
  };

  // --------------------------------------------------------------------------

  auto from_array = [] (auto && arr)
  {
    using array_type = std::remove_reference_t<decltype (arr)>;

    static_assert (std::is_array<array_type>::value, "arr must be a C-Style array");

    // arr + 0 makes the expression a pointer
    return from_iterators (arr + 0, arr + std::extent<array_type, 0>::value);
  };

  // --------------------------------------------------------------------------

  template<typename TValue>
  CPP_STREAMS__PRELUDE auto from_empty ()
  {
    return detail::adapt_source_function<TValue> (
      [] (auto &&)
      {
      });
  }

  // --------------------------------------------------------------------------

  auto from_repeat = [] (auto && value, std::size_t count)
  {
    using value_type = decltype (value);

    return detail::adapt_source_function<value_type> (
      [count, value = std::forward<value_type> (value)] (auto && sink)
      {
        for (auto iter = 0U; iter < count && sink (value); ++iter)
          ;
      });
  };

  // --------------------------------------------------------------------------

  auto from_singleton = [] (auto && value)
  {
    using value_type = decltype (value);

    return from_repeat (std::forward<value_type> (value), 1);
  };

  // --------------------------------------------------------------------------
  // Pipes
  // --------------------------------------------------------------------------

  auto append = [] (auto && other_source)
  {
    CPP_STREAMS__CHECK_SOURCE (other_source);

#ifndef _MSC_VER
    // WORKAROUND: G++ gets confused with other_source_type declared inside lambda
    using other_source_type = decltype (other_source)                           ;
#endif

    return
      // WORKAROUND: perfect forwarding preferable
      [other_source] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type       = decltype (source)                                 ;
#ifdef _MSC_VER
        // WORKAROUND: VS2015 RC ICE:s if other_source_type is put in outer scope
        using other_source_type = decltype (other_source)                           ;
#endif
        using value_type        = detail::get_source_value_type_t<source_type>      ;
        using other_value_type  = detail::get_source_value_type_t<other_source_type>;

        static_assert (std::is_convertible<other_value_type, value_type>::value, "TOtherSource values must be convertible into a TSource value");

        return detail::adapt_source_function<value_type> (
          [other_source = other_source, source = std::forward<source_type> (source)] (auto && sink)
          {
            source.source_function ([&sink] (auto && v)
            {
              return sink (std::forward<decltype (v)> (v));
            });

            other_source.source_function ([&sink] (auto && v)
            {
              return sink (std::forward<decltype (v)> (v));
            });
          });
      };
  };

  // --------------------------------------------------------------------------

  auto collect = [] (auto && collector)
  {
#ifndef _MSC_VER
    // WORKAROUND: G++ gets confused with collector_type declared inside lambda
    using collector_type    = decltype (collector)                              ;
#endif
    return
      // WORKAROUND: perfect forwarding preferable
      [collector] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

#ifdef _MSC_VER
        // WORKAROUND: VS2015 RC ICE:s if collector_type is put in outer scope
        using collector_type    = decltype (collector)                              ;
#endif
        using source_type       = decltype (source)                                 ;
        using value_type        = detail::get_source_value_type_t<source_type>      ;
        using inner_source_type = std::result_of_t<collector_type (value_type)>     ;
        using inner_value_type  = detail::get_source_value_type_t<inner_source_type>;

        return detail::adapt_source_function<inner_value_type> (
          [collector, source = std::forward<source_type> (source)] (auto && sink)
          {
            source.source_function ([&collector, &sink] (auto && v)
            {
              auto result = true;

              // Don't use std::forward<decltype (v)> (v) as this might cause v to destroy
              //  If the inner_source is member field of v we do like v to live on
              auto inner_source = collector (v);

              CPP_STREAMS__CHECK_SOURCE (inner_source);

              inner_source.source_function ([&result, &sink] (auto && iv)
              {
                return result = sink (std::forward<decltype (iv)> (iv));
              });

              return result;
            });
          });

      };

  };

  // --------------------------------------------------------------------------

  auto filter = [] (auto && tester)
  {
    // using tester_type = decltype (tester);

    return
      // WORKAROUND: perfect forwarding preferable
      [tester] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type = decltype (source)                           ;
        using value_type  = detail::get_source_value_type_t<source_type>;

        return detail::adapt_source_function<value_type> (
          [tester, source = std::forward<source_type> (source)] (auto && sink)
          {
            source.source_function ([&tester, &sink] (auto && v)
            {
              if (tester (v))
              {
                return sink (std::forward<decltype (v)> (v));
              }
              else
              {
                return true;
              }
            });
          });
      };
  };

  // --------------------------------------------------------------------------

  auto map = [] (auto && mapper)
  {
#ifndef _MSC_VER
    // WORKAROUND: G++ gets confused with mapper_type declared inside lambda
    using mapper_type     = decltype (mapper);
#endif

    return
      // WORKAROUND: perfect forwarding preferable
      [mapper] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

#ifdef _MSC_VER
        // WORKAROUND: VS2015 RC ICE:s if mapper_type is put in outer scope
        using mapper_type     = decltype (mapper)                           ;
#endif
        using source_type     = decltype (source)                           ;
        using value_type      = detail::get_source_value_type_t<source_type>;
        using map_value_type  = std::result_of_t<mapper_type (value_type)>  ;

        return detail::adapt_source_function<map_value_type> (
          [mapper, source = std::forward<source_type> (source)] (auto && sink)
          {
            source.source_function ([&mapper, &sink] (auto && v)
            {
              return sink (mapper (std::forward<decltype (v)> (v)));
            });
          });
      };
  };

  // --------------------------------------------------------------------------

  auto mapi = [] (auto && mapper)
  {
#ifndef _MSC_VER
    // WORKAROUND: G++ gets confused with mapper_type declared inside lambda
    using mapper_type     = decltype (mapper);
#endif

    return
      // WORKAROUND: perfect forwarding preferable
      [mapper] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

#ifdef _MSC_VER
        // WORKAROUND: VS2015 RC ICE:s if mapper_type is put in outer scope
        using mapper_type     = decltype (mapper)                                       ;
#endif
        using source_type     = decltype (source)                                       ;
        using value_type      = detail::get_source_value_type_t<source_type>            ;
        using map_value_type  = std::result_of_t<mapper_type (std::size_t, value_type)> ;

        return detail::adapt_source_function<map_value_type> (
          [mapper, source = std::forward<source_type> (source)] (auto && sink)
          {
            std::size_t iter = 0U;

            source.source_function ([&iter, &mapper, &sink] (auto && v)
            {
              return sink (mapper (iter++, std::forward<decltype (v)> (v)));
            });
          });
      };
  };

  // --------------------------------------------------------------------------

#ifndef _MSC_VER
  auto reverse =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE (source);

      using source_type         = decltype (source)                                     ;
      using stripped_value_type = detail::get_stripped_source_value_type_t<source_type> ;
      // Added std::add_rvalue_reference_t to allow moving of vector copies
      using value_type          = std::add_rvalue_reference_t<stripped_value_type>      ;

      return detail::adapt_source_function<value_type> (
        [source = std::forward<source_type> (source)] (auto && sink)
        {
          std::vector<stripped_value_type> result;
          result.reserve (detail::default_vector_reserve);

          source.source_function ([&result] (auto && v)
          {
            result.push_back (std::forward<decltype (v)> (v));
            return true;
          });

          auto iter = result.size ();
          while (iter != 0 && sink (std::move (result[--iter])))
            ;
        });
    };
#endif

  // --------------------------------------------------------------------------

  auto skip = [] (std::size_t count)
  {
    return
      [count] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type = decltype (source)                           ;
        using value_type  = detail::get_source_value_type_t<source_type>;

        return detail::adapt_source_function<value_type> (
          [count, source = std::forward<source_type> (source)] (auto && sink)
          {
            auto remaining = count;

            source.source_function ([&remaining, &sink] (auto && v)
            {
              if (remaining == 0)
              {
                return sink (std::forward<decltype (v)> (v));
              }
              else
              {
                --remaining;
                return true;
              }
            });
          });
      };
  };

  // --------------------------------------------------------------------------

  auto skip_while = [] (auto && skipper)
  {
    // using skipper_type = decltype ()

    return
      // WORKAROUND: perfect forwarding preferable
      [skipper] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type = decltype (source)                           ;
        using value_type  = detail::get_source_value_type_t<source_type>;

        return detail::adapt_source_function<value_type> (
          [skipper, source = std::forward<source_type> (source)] (auto && sink)
          {
            auto do_skip = true;

            source.source_function ([&do_skip, &skipper, &sink] (auto && v)
            {
              if (!do_skip)
              {
                return sink (std::forward<decltype (v)> (v));
              }
              else if (skipper (v))
              {
                return true;
              }
              else
              {
                do_skip = false;
                return sink (std::forward<decltype (v)> (v));
              }
            });
          });
      };
  };

  // --------------------------------------------------------------------------

#ifndef _MSC_VER
  auto sort = [] (auto && sorter)
  {
    return
      // WORKAROUND: perfect forwarding preferable
      [sorter] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type         = decltype (source)                                     ;
        using stripped_value_type = detail::get_stripped_source_value_type_t<source_type> ;
        // Added std::add_rvalue_reference_t to allow moving of vector copies
        using value_type          = std::add_rvalue_reference_t<stripped_value_type>      ;

        return detail::adapt_source_function<value_type> (
          [sorter, source = std::forward<source_type> (source)] (auto && sink)
          {
            std::vector<stripped_value_type> result;
            result.reserve (detail::default_vector_reserve);

            source.source_function ([&result] (auto && v)
            {
              result.push_back (std::forward<decltype (v)> (v));
              return true;
            });

            std::sort (
                result.begin ()
              , result.end ()
              , sorter
              );

            auto sz = result.size ();
            for (auto iter = 0U; iter < sz && sink (std::move (result[iter])); ++iter)
              ;
          });
      };
  };

  // --------------------------------------------------------------------------

  auto sort_by = [] (auto && selector)
  {
    return
      sort ([selector] (auto && l, auto && r)
        {
          return selector (std::forward<decltype (l)> (l)) < selector (std::forward<decltype (r)> (r));
        });
  };
#endif

  // --------------------------------------------------------------------------

  auto take = [] (std::size_t count)
  {
    return
      [count] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type = decltype (source)                           ;
        using value_type  = detail::get_source_value_type_t<source_type>;

        return detail::adapt_source_function<value_type> (
          [count, source = std::forward<source_type> (source)] (auto && sink)
          {
            auto remaining = count;
            source.source_function ([&remaining, &sink] (auto && v)
            {
              if (remaining > 0)
              {
                --remaining;
                return sink (std::forward<decltype (v)> (v));
              }
              else
              {
                return false;
              }
            });
          });
      };
  };

  // --------------------------------------------------------------------------

  auto take_while = [] (auto && taker)
  {
    return
      // WORKAROUND: perfect forwarding preferable
      [taker] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type = decltype (source)                           ;
        using value_type  = detail::get_source_value_type_t<source_type>;

        return detail::adapt_source_function<value_type> (
          [taker, source = std::forward<source_type> (source)] (auto && sink)
          {
            source.source_function ([&taker, &sink] (auto && v)
            {
              if (taker (v))
              {
                return sink (std::forward<decltype (v)> (v));
              }
              else
              {
                return false;
              }
            });
          });
      };
  };

  // --------------------------------------------------------------------------
  // Sinks
  // --------------------------------------------------------------------------

  auto to_all = [] (auto && tester)
  {
    // using tester_type  = decltype (tester);

    return
      // WORKAROUND: perfect forwarding preferable
      [tester] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        auto result = false;

        source.source_function (
          [&result, &tester] (auto && v)
          {
            return result = tester (std::forward<decltype (v)> (v));
          });

        return result;
      };
  };

  // --------------------------------------------------------------------------

  auto to_any = [] (auto && tester)
  {
    // using tester_type  = decltype (tester);

    return
      // WORKAROUND: perfect forwarding preferable
      [tester] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        auto result = true;

        source.source_function (
          [&result, &tester] (auto && v)
          {
            return result = !tester (std::forward<decltype (v)> (v));
          });

        return !result;
      };
  };

  // --------------------------------------------------------------------------

  auto to_first_or_default =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE (source);

      using source_type = decltype (source);
      using value_type  = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: value_type result {} doesn't work in VS2015 RC
      auto result = value_type ();

      source.source_function (
        [&result] (auto && v)
        {
          result = std::forward<decltype (v)> (v);
          return false;
        });

      return result;
    };

  // --------------------------------------------------------------------------

  auto to_iter = [] (auto && iteration)
  {
    // using iteration_type  = decltype (iteration);

    return
      // WORKAROUND: perfect forwarding preferable
      [iteration] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        source.source_function (
          [&iteration] (auto && v)
          {
            return iteration (v);
          });
      };
  };

  // --------------------------------------------------------------------------

  auto to_fold = [] (auto && initial, auto && folder)
  {
    // using state_type  = decltype (initial);
    // using folder_type = decltype (folder);

    return
      // WORKAROUND: perfect forwarding preferable
      [initial, folder] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        auto state = initial;

        source.source_function (
          [&state, &folder] (auto && v)
          {
            state = folder (std::move (state), std::forward<decltype (v)> (v));
            return true;
          });

        return state;
      };
  };

  // --------------------------------------------------------------------------

  auto to_last_or_default =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE (source);

      using source_type= decltype (source);
      using value_type = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: value_type result {} doesn't work in VS2015 RC
      auto result = value_type ();

      source.source_function (
        [&result] (auto && v)
        {
          result = std::forward<decltype (v)> (v);
          return true;
        });

      return result;
    };

  // --------------------------------------------------------------------------

  auto to_length =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE (source);

      std::size_t result = 0;

      source.source_function (
        [&result] (auto &&)
        {
          ++result;
          return true;
        });

      return result;
    };

  // --------------------------------------------------------------------------

#ifndef _MSC_VER
  auto to_map = [] (auto && key_selector)
  {
    using key_selector_type  = decltype (key_selector)                              ;

    return
      // WORKAROUND: perfect forwarding preferable
      [key_selector] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);


        using source_type       = decltype (source)                                     ;
        using value_type        = detail::get_stripped_source_value_type_t<source_type> ;
        using selected_key_type = std::result_of_t<key_selector_type (value_type)>      ;
        using key_type          = detail::strip_type_t<selected_key_type>               ;
        using map_type          = std::map<key_type, value_type>                        ;
        using item_type         = typename map_type::value_type                         ;

        // WORKAROUND: std::set<value_type> result; doesn't work in VS2015 RC
        auto result = map_type ();

        source.source_function (
          [&key_selector, &result] (auto && v)
          {
            auto key = key_selector (v);
            result.insert (item_type (std::move (key), std::forward<decltype (v)> (v)));
            return true;
          });

        return result;
      };
  };
#endif

  // --------------------------------------------------------------------------

  auto to_max = [] (auto && initial)
  {
    return
      // WORKAROUND: perfect forwarding preferable
      [initial] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type= decltype (source);
        using value_type = detail::get_stripped_source_value_type_t<source_type>;

        // WORKAROUND: value_type result = initial doesn't work in VS2015 RC
        auto result = value_type (initial);

        source.source_function (
          [&result] (auto && v)
          {
            // WORKAROUND: std::max produced warnings in VS2015 RC
            if (result < v)
            {
              result = std::forward<decltype (v)> (v);
            }
            return true;
          });

        return result;
      };
  };

  // --------------------------------------------------------------------------

  auto to_min = [] (auto && initial)
  {
    return
      // WORKAROUND: perfect forwarding preferable
      [initial] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE (source);

        using source_type= decltype (source);
        using value_type = detail::get_stripped_source_value_type_t<source_type>;

        // WORKAROUND: value_type result = initial doesn't work in VS2015 RC
        auto result = value_type (initial);

        source.source_function (
          [&result] (auto && v)
          {
            // WORKAROUND: std::min produced warnings in VS2015 RC
            if (v < result)
            {
              result = std::forward<decltype (v)> (v);
            }
            return true;
          });

        return result;
      };
  };

  // --------------------------------------------------------------------------

  auto to_set =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE (source);

      using source_type= decltype (source);
      using value_type = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: std::set<value_type> result; doesn't work in VS2015 RC
      auto result = std::set<value_type> ();

      source.source_function (
        [&result] (auto && v)
        {
          result.insert (std::forward<decltype (v)> (v));
          return true;
        });

      return result;
    };

  // --------------------------------------------------------------------------

  auto to_sum =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE (source);

      using source_type= decltype (source);
      using value_type = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: value_type result {} doesn't work in VS2015 RC
      auto result = value_type ();

      source.source_function (
        [&result] (auto && v)
        {
          result += std::forward<decltype (v)> (v);
          return true;
        });

      return result;
    };

  // --------------------------------------------------------------------------

  auto to_vector =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE (source);

      using source_type= decltype (source);
      using value_type = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: std::vector<value_type> result {} doesn't work in VS2015 RC
      auto result = std::vector<value_type> ();
      result.reserve (detail::default_vector_reserve);

      source.source_function (
        [&result] (auto && v)
        {
          result.push_back (std::forward<decltype (v)> (v));
          return true;
        });

      return result;
    };

  // --------------------------------------------------------------------------

}

// ----------------------------------------------------------------------------
#endif // CPP_STREAMS__INCLUDE_GUARD
// ----------------------------------------------------------------------------
