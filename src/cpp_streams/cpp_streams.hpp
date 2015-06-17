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
  static_assert(                                      \
      detail::is_source_adapter<decltype(src)>::value \
    , #src " must be a a proper cpp_streams source"   \
    )
# define CPP_STREAMS__PRELUDE constexpr
# define CPP_STREAMS__SINK    inline
# define CPP_STREAMS__BODY(type)            \
  type (type const &)             = default;\
  type (type &&)                  = default;\
  type& operator= (type const &)  = default;\
  type& operator= (type &&)       = default;
// ----------------------------------------------------------------------------
# include <type_traits>
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
    struct source_adapter
    {
      using value_type = TValueType;

      TSourceFunction source_function;

      CPP_STREAMS__BODY (source_adapter);

      explicit CPP_STREAMS__PRELUDE source_adapter (TSourceFunction const & source_function)
        : source_function (source_function)
      {
      }

      explicit CPP_STREAMS__PRELUDE source_adapter (TSourceFunction && source_function)
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
    template<typename TValueType, typename TSource>
    CPP_STREAMS__PRELUDE auto adapt_source (TSource && source)
    {
      return source_adapter<TValueType, TSource> (std::forward<TSource> (source));
    }

    template<typename T>
    struct is_source_adapter_impl
    {
      enum
      {
        value = false,
      };
    };

    template<typename TValueType, typename TSource>
    struct is_source_adapter_impl<source_adapter<TValueType, TSource>>
    {
      enum
      {
        value = true,
      };
    };

    template<typename T>
    struct is_source_adapter
    {
      enum
      {
        value = is_source_adapter_impl<strip_type_t<T>>::value,
      };
    };

    template<typename T>
    struct get_source_value_type_impl;

    template<typename TValueType, typename TSource>
    struct get_source_value_type_impl<source_adapter<TValueType, TSource>>
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

    template<typename TPredicate>
    struct collect_pipe
    {
      TPredicate  predicate ;

      template<typename TValue>
      static TValue get_value ();

      CPP_STREAMS__BODY (collect_pipe);

      explicit CPP_STREAMS__PRELUDE collect_pipe (TPredicate predicate)
        : predicate (std::move (predicate))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        using inner_source_type = decltype (predicate (get_value<TValueType> ()));
        using inner_value_type  = get_source_value_type_t<inner_source_type>;

        static_assert (is_source_adapter<inner_source_type>::value, "consume predicate must return a proper cpp_streams source");

        return adapt_source<inner_value_type> (
          [this, predicate = predicate, source = std::forward<TSource> (source)] (auto && sink)
          {
            source ([&predicate, &sink] (auto && v)
            {
              auto result = true;

              // Don't use std::forward<decltype (v)> (v) as this might cause v to destroy
              //  If the inner_source is member field of v we do like v to live on
              auto inner_source = predicate (v);

              inner_source.source_function ([&result, &sink] (auto && iv)
              {
                return result = sink (std::forward<decltype (iv)> (iv));
              });

              return result;
            });
          });
      }
    };

    // ------------------------------------------------------------------------

    template<typename TPredicate>
    struct skip_while_pipe
    {
      TPredicate predicate;

      CPP_STREAMS__BODY (skip_while_pipe);

      explicit CPP_STREAMS__PRELUDE skip_while_pipe (TPredicate predicate)
        : predicate (std::move (predicate))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        using value_type = strip_type_t<TValueType>;

        return adapt_source<value_type> (
          [this, predicate = predicate, source = std::forward<TSource> (source)] (auto && sink)
          {
            auto do_skip = true;

            source ([&do_skip, &predicate, &sink] (auto && v)
            {
              if (!do_skip)
              {
                return sink (std::forward<decltype (v)> (v));
              }
              if (predicate (v))
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
      }
    };

    // ------------------------------------------------------------------------

    template<typename TPredicate>
    struct take_while_pipe
    {
      TPredicate predicate;

      CPP_STREAMS__BODY (take_while_pipe);

      explicit CPP_STREAMS__PRELUDE take_while_pipe (TPredicate predicate)
        : predicate (std::move (predicate))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        using value_type = strip_type_t<TValueType>;

        return adapt_source<value_type> (
          [this, predicate = predicate, source = std::forward<TSource> (source)] (auto && sink)
          {
            source ([&predicate, &sink] (auto && v)
            {
              if (predicate (v))
              {
                return sink (std::forward<decltype (v)> (v));
              }
              else
              {
                return false;
              }
            });
          });
      }
    };

    // ------------------------------------------------------------------------

  }

  // --------------------------------------------------------------------------
  // Sources
  // --------------------------------------------------------------------------

  auto from_iterators = [] (auto && begin, auto && end)
  {
    using begin_type  = decltype (begin);
    using end_type    = decltype (end);
    using value_type  = decltype (*begin);

    return detail::adapt_source<value_type> (
      [begin = std::forward<begin_type> (begin), end = std::forward<end_type> (end)] (auto && sink)
      {
        for (auto iter = begin; iter != end && sink (*iter); ++iter)
            ;
      });
  };

  // --------------------------------------------------------------------------

  auto from = [] (auto && container)
  {
    using container_type = decltype (container);

    return from_iterators (container.begin (), container.end ());
  };

  // --------------------------------------------------------------------------

  auto from_array = [] (auto && arr)
  {
    using array_type = std::remove_reference_t<decltype (arr)>;

    static_assert (std::is_array<array_type>::value, "arr must be a C-Style array");

    return from_iterators (arr, arr + std::extent<array_type, 0>::value);
  };

  // --------------------------------------------------------------------------

  template<typename TValue>
  CPP_STREAMS__PRELUDE auto from_empty ()
  {
    return detail::adapt_source<TValue> (
      [] (auto &&)
      {
      });
  }

  // --------------------------------------------------------------------------

  auto from_repeat = [] (auto && value, std::size_t count)
  {
    using value_type = decltype (value);

    return detail::adapt_source<value_type> (
      [count, value = std::forward<value_type> (value)] (auto && sink)
      {
        for (auto iter = 0U; iter < count && sink (value); ++iter)
            ;
      });
  };

  // --------------------------------------------------------------------------

  template<typename TValue>
  CPP_STREAMS__PRELUDE auto from_singleton (TValue value)
  {
    return from_repeat (std::move (value), 1);
  }

  // --------------------------------------------------------------------------
  // Pipes
  // --------------------------------------------------------------------------

  auto append = [] (auto && other_source)
  {
    CPP_STREAMS__CHECK_SOURCE(other_source);

    return
      // WORKAROUND: perfect forwarding preferable
      [other_source] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE(source);

        using source_type       = decltype (source);
        using other_source_type = decltype (other_source);
        using value_type        = detail::get_source_value_type_t<source_type>;
        using other_value_type  = detail::get_source_value_type_t<other_source_type>;

        static_assert (std::is_convertible<other_value_type, value_type>::value, "TOtherSource values must be convertible into a TSource value");

        return detail::adapt_source<value_type> (
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

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto collect (TPredicate predicate)
  {
    return detail::collect_pipe<TPredicate> (std::move (predicate));
  }

  // --------------------------------------------------------------------------

  auto filter = [] (auto && tester)
  {
    using tester_type = decltype (tester);

    return
      // WORKAROUND: perfect forwarding preferable
      [tester] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE(source);

        using source_type = decltype (source);
        using value_type  = detail::get_source_value_type_t<source_type>;

        return detail::adapt_source<value_type> (
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
        CPP_STREAMS__CHECK_SOURCE(source);

#ifdef _MSC_VER
        // WORKAROUND: VS2015 RC ICE:s if mapper_type is put in outer scope
        using mapper_type     = decltype (mapper);
#endif
        using source_type     = decltype (source);
        using value_type      = detail::get_source_value_type_t<source_type>   ;
        using map_value_type  = std::result_of_t<mapper_type (value_type)> ;

        return detail::adapt_source<map_value_type> (
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
        CPP_STREAMS__CHECK_SOURCE(source);

#ifdef _MSC_VER
        // WORKAROUND: VS2015 RC ICE:s if mapper_type is put in outer scope
        using mapper_type     = decltype (mapper);
#endif
        using source_type     = decltype (source);
        using value_type      = detail::get_source_value_type_t<source_type>   ;
        using map_value_type  = std::result_of_t<mapper_type (std::size_t, value_type)> ;

        return detail::adapt_source<map_value_type> (
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
  // WORKAROUND: Find VS2015 RC workaround
  auto reverse =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE(source);

      using source_type = decltype (source);
      using value_type  = detail::get_source_value_type_t<source_type>;

      return detail::adapt_source<value_type> (
        [source = std::forward<source_type> (source)] (auto && sink)
        {
          std::vector<detail::strip_type_t<value_type>> result;
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

  // --------------------------------------------------------------------------

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto skip_while (TPredicate predicate)
  {
    return detail::skip_while_pipe<TPredicate> (std::move (predicate));
  }

  // --------------------------------------------------------------------------

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto take_while (TPredicate predicate)
  {
    return detail::take_while_pipe<TPredicate> (std::move (predicate));
  }

  // --------------------------------------------------------------------------
  // Sinks
  // --------------------------------------------------------------------------

  auto to_first_or_default =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE(source);

      using source_type = decltype (source);
      using value_type  = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: value_type result {} doesn't work in VS2015 RC
      auto result = value_type ();

      source.source_function (
        [&] (auto && v)
        {
          result = (std::forward<decltype (v)> (v));
          return false;
        });

      return result;
    };

  // --------------------------------------------------------------------------

  auto to_iter = [] (auto && iteration)
  {
    using iteration_type  = decltype (iteration);

    return
      // WORKAROUND: perfect forwarding preferable
      [iteration] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE(source);

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
    using state_type  = decltype (initial);
    using folder_type = decltype (folder);

    return
      [initial, folder] (auto && source)
      {
        CPP_STREAMS__CHECK_SOURCE(source);

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
      CPP_STREAMS__CHECK_SOURCE(source);

      using source_type= decltype (source);
      using value_type = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: value_type result {} doesn't work in VS2015 RC
      auto result = value_type ();

      source.source_function (
        [&result] (auto && v)
        {
          result = (std::forward<decltype (v)> (v));
          return true;
        });

      return result;
    };

  // --------------------------------------------------------------------------

  auto to_sum =
    [] (auto && source)
    {
      CPP_STREAMS__CHECK_SOURCE(source);

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
      CPP_STREAMS__CHECK_SOURCE(source);

      using source_type= decltype (source);
      using value_type = detail::get_stripped_source_value_type_t<source_type>;

      // WORKAROUND: std::vector<value_type> result {} doesn't work in VS2015 RC
      auto result = std::vector<value_type> ();

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
