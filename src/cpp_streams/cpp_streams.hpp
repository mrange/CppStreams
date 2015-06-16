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

    template<typename TOtherSource>
    struct append_pipe
    {
      TOtherSource  other_source  ;

      CPP_STREAMS__BODY (append_pipe);

      static_assert (is_source_adapter<TOtherSource>::value, "TOtherSource must be a proper cpp_streams source");

      explicit CPP_STREAMS__PRELUDE append_pipe (TOtherSource other_source)
        : other_source (std::move (other_source))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        using value_type        = get_source_value_type_t<decltype (source)>;
        using other_value_type  = get_source_value_type_t<decltype (source)>;
        static_assert (std::is_convertible<other_value_type, value_type>::value, "TOtherSource values must be convertible into a TSource value");

        return adapt_source<value_type> (
          [this, other_source = other_source, source = std::forward<TSource> (source)] (auto && sink)
          {
            source ([&sink] (auto && v)
            {
              return sink (std::forward<decltype (v)> (v));
            });

            other_source.source_function ([&sink] (auto && v)
            {
              return sink (std::forward<decltype (v)> (v));
            });
          });
      }
    };

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
    struct map_pipe
    {
      TPredicate  predicate ;

      template<typename TValue>
      static TValue get_value ();

      CPP_STREAMS__BODY (map_pipe);

      explicit CPP_STREAMS__PRELUDE map_pipe (TPredicate predicate)
        : predicate (std::move (predicate))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        using value_type = decltype (predicate (get_value<TValueType> ()));

        return adapt_source<value_type> (
          [this, predicate = predicate, source = std::forward<TSource> (source)] (auto && sink)
          {
            source ([&predicate, &sink] (auto && v)
            {
              return sink (predicate (std::forward<decltype (v)> (v)));
            });
          });
      }
    };

    // ------------------------------------------------------------------------

    template<typename TPredicate>
    struct mapi_pipe
    {
      TPredicate  predicate ;

      template<typename TValue>
      static TValue get_value ();

      CPP_STREAMS__BODY (mapi_pipe);

      explicit CPP_STREAMS__PRELUDE mapi_pipe (TPredicate predicate)
        : predicate (std::move (predicate))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        using value_type = decltype (predicate (0U, get_value<TValueType> ()));

        return adapt_source<value_type> (
          [this, predicate = predicate, source = std::forward<TSource> (source)] (auto && sink)
          {
            std::size_t iter = 0U;
            source ([&iter, &predicate, &sink] (auto && v)
            {
              return sink (predicate (iter++, std::forward<decltype (v)> (v)));
            });
          });
      }
    };

    // ------------------------------------------------------------------------

    struct reverse_pipe
    {
      std::size_t reserve;

      CPP_STREAMS__BODY (reverse_pipe);

      explicit CPP_STREAMS__PRELUDE reverse_pipe (std::size_t reserve)
        : reserve (reserve)
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        using value_type = strip_type_t<TValueType>;

        return adapt_source<value_type> (
          [this, reserve = reserve, source = std::forward<TSource> (source)] (auto && sink)
          {
            std::vector<value_type> result;
            result.reserve (reserve);

            source ([&result] (auto && v)
            {
              result.push_back (std::forward<decltype (v)> (v));
              return true;
            });

            auto iter = result.size ();
            while (iter != 0 && sink (std::move (result[--iter])))
              ;
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

  template<typename TIterator>
  CPP_STREAMS__PRELUDE auto from_iterators (TIterator begin, TIterator end)
  {
    return detail::adapt_source<decltype (*begin)> (
      [begin = std::move (begin), end = std::move (end)] (auto && sink)
      {
        for (auto iter = begin; iter != end && sink (*iter); ++iter)
            ;
      });
  }

  // --------------------------------------------------------------------------

  template<typename TContainer>
  CPP_STREAMS__PRELUDE auto from (TContainer & container)
  {
    return from_iterators (container.begin (), container.end ());
  }

  // --------------------------------------------------------------------------

  template<typename TArray>
  CPP_STREAMS__PRELUDE auto from_array (TArray & arr)
  {
    return from_iterators (arr, arr + std::extent<TArray, 0>::value);
  }

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

  template<typename TValue>
  CPP_STREAMS__PRELUDE auto from_repeat (TValue value, std::size_t count)
  {
    return detail::adapt_source<TValue> (
      [count, value = std::move (value)] (auto && sink)
      {
        for (auto iter = 0U; iter < count && sink (value); ++iter)
            ;
      });
  }

  // --------------------------------------------------------------------------

  template<typename TValue>
  CPP_STREAMS__PRELUDE auto from_singleton (TValue value)
  {
    return from_repeat (std::move (value), 1);
  }

  // --------------------------------------------------------------------------
  // Pipes
  // --------------------------------------------------------------------------

  template<typename TOtherSource>
  CPP_STREAMS__PRELUDE auto append (TOtherSource other_source)
  {
    return detail::append_pipe<TOtherSource> (std::move (other_source));
  }

  // --------------------------------------------------------------------------

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto collect (TPredicate predicate)
  {
    return detail::collect_pipe<TPredicate> (std::move (predicate));
  }

  // --------------------------------------------------------------------------

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto filter (TPredicate && predicate)
  {
    return
      [predicate = std::forward<TPredicate> (predicate)] (auto && source)
      {
        using value_type = detail::get_source_value_type_t<decltype (source)>;

        return detail::adapt_source<value_type> (
          [predicate, source = std::forward<decltype (source)> (source)] (auto && sink)
          {
            source.source_function ([&predicate, &sink] (auto && v)
            {
              if (predicate (v))
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
  }

  // --------------------------------------------------------------------------

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto map (TPredicate predicate)
  {
    return detail::map_pipe<TPredicate> (std::move (predicate));
  }

  // --------------------------------------------------------------------------

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto mapi (TPredicate predicate)
  {
    return detail::mapi_pipe<TPredicate> (std::move (predicate));
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto reverse (std::size_t reserve = 16)
  {
    return detail::reverse_pipe (reserve);
  }

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

  CPP_STREAMS__SINK auto to_first_or_default ()
  {
    return
      [] (auto && source)
      {
        // WORKAROUND: For some reason 'using' doesn't work here in VS2015 RC
        //  Interestingly enough it's not needed in to_last_or_default
        //  Seems order dependent
        typedef detail::get_stripped_source_value_type_t<decltype (source)> value_type;

        value_type result {};

        source.source_function (
          [&result] (auto && v)
          {
            result = (std::forward<decltype (v)> (v));
            return false;
          });

        return result;
      };
  }

  // --------------------------------------------------------------------------

  template<typename TIteration>
  CPP_STREAMS__SINK auto to_iter (TIteration && iteration)
  {
    return
      [iteration = std::forward<TIteration> (iteration)] (auto && source)
      {
        source.source_function (
          [&iteration] (auto && v)
          {
            return iteration (v);
          });
      };
  }

  // --------------------------------------------------------------------------

  template<typename TState, typename TFolder>
  CPP_STREAMS__PRELUDE auto to_fold (TState && initial, TFolder && folder)
  {
    return
      [initial = std::forward<TState> (initial), folder = std::forward<TFolder> (folder)] (auto && source)
      {
        auto state = initial;

        source.source_function (
          [&state, &folder] (auto && v)
          {
            state = folder (std::move (state), std::forward<decltype (v)> (v));
            return true;
          });

        return state;
      };
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__SINK auto to_last_or_default ()
  {
    return
      [] (auto && source)
      {
        using value_type = detail::get_stripped_source_value_type_t<decltype (source)>;

        value_type result {};

        source.source_function (
          [&result] (auto && v)
          {
            result = (std::forward<decltype (v)> (v));
            return true;
          });

        return result;
      };
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__SINK auto to_sum ()
  {
    return
      [] (auto && source)
      {
        using value_type = detail::get_stripped_source_value_type_t<decltype (source)>;

        value_type result {};

        source.source_function (
          [&result] (auto && v)
          {
            result += std::forward<decltype (v)> (v);
            return true;
          });

        return result;
      };
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__SINK auto to_vector ()
  {
    return
      [] (auto && source)
      {
        using value_type = detail::get_stripped_source_value_type_t<decltype (source)>;

        std::vector<value_type> result;

        source.source_function (
          [&result] (auto && v)
          {
            result.push_back (std::forward<decltype (v)> (v));
            return true;
          });

        return result;
      };
  }

  // --------------------------------------------------------------------------

}

// ----------------------------------------------------------------------------
#endif // CPP_STREAMS__INCLUDE_GUARD
// ----------------------------------------------------------------------------
