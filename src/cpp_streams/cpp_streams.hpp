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

    template<typename TValueType, typename TSource>
    struct source_adapter
    {
      using value_type = TValueType;

      TSource source;

      CPP_STREAMS__BODY (source_adapter);

      explicit CPP_STREAMS__PRELUDE source_adapter (TSource const & source)
        : source (source)
      {
      }

      explicit CPP_STREAMS__PRELUDE source_adapter (TSource && source)
        : source (std::move (source))
      {
      }

      template<typename TSink>
      CPP_STREAMS__PRELUDE auto operator >> (TSink && sink) const
      {
        return sink.template consume<TValueType> (source);
      }
    };

    // Adapts a source function into a Source
    template<typename TValueType, typename TSource>
    CPP_STREAMS__PRELUDE auto adapt_source (TSource && source)
    {
      return source_adapter<TValueType, TSource> (std::forward<TSource> (source));
    }

    template<typename T>
    struct is_source_adapter
    {
      enum
      {
        value = false,
      };
    };

    template<typename TValueType, typename TSource>
    struct is_source_adapter<source_adapter<TValueType, TSource>>
    {
      enum
      {
        value = true,
      };
    };

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
        static_assert (std::is_same<TValueType, TOtherSource::value_type>::value, "TSource and TOtherSource must be a cpp_streams source of same value_type");

        return adapt_source<TValueType> (
          [this, other_source = other_source, source = std::forward<TSource> (source)] (auto && sink)
          {
            source ([&sink] (auto && v)
            {
              return sink (std::forward<decltype(v)> (v));
            });

            other_source.source ([&sink] (auto && v)
            {
              return sink (std::forward<decltype(v)> (v));
            });
          });
      }
    };

    // ------------------------------------------------------------------------

    template<typename TPredicate>
    struct filter_pipe
    {
      TPredicate  predicate ;

      CPP_STREAMS__BODY (filter_pipe);

      explicit CPP_STREAMS__PRELUDE filter_pipe (TPredicate predicate)
        : predicate (std::move (predicate))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__PRELUDE auto consume (TSource && source) const
      {
        return adapt_source<TValueType> (
          [this, predicate = predicate, source = std::forward<TSource> (source)] (auto && sink)
          {
            source ([&predicate, &sink] (auto && v)
            {
              if (predicate (v))
              {
                return sink (std::forward<decltype(v)> (v));
              }
              else
              {
                return true;
              }
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
            source ([&predicate, &sink (sink)] (auto && v)
            {
              return sink (predicate (std::forward<decltype(v)> (v)));
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
              result.push_back (std::forward<decltype(v)> (v));
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
                return sink (std::forward<decltype(v)> (v));
              }
              if (predicate (v))
              {
                return true;
              }
              else
              {
                do_skip = false;
                return sink (std::forward<decltype(v)> (v));
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
                return sink (std::forward<decltype(v)> (v));
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

    struct first_or_default_sink
    {
      CPP_STREAMS__BODY (first_or_default_sink);

      first_or_default_sink () = default;

      template<typename TValueType, typename TSource>
      CPP_STREAMS__SINK auto consume (TSource && source) const
      {
        using value_type = strip_type_t<TValueType>;

        value_type result {};

        source (
          [&result] (auto && v)
          {
            result = std::forward<decltype (v)> (v);
            return false;
          });

        return result;
      }
    };

    // ------------------------------------------------------------------------

    template<typename TFolder, typename TState>
    struct fold_sink
    {
      TFolder folder  ;
      TState  initial ;

      CPP_STREAMS__BODY (fold_sink);

      fold_sink (TFolder folder, TState initial)
        : folder  (std::move (folder))
        , initial (std::move (initial))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__SINK auto consume (TSource && source) const
      {
        auto state = initial;

        source (
          [this, &state] (auto && v)
          {
            state = folder (std::move (state), std::forward<decltype (v)> (v));
            return true;
          });

        return state;
      }
    };

    // ------------------------------------------------------------------------

    template<typename TIteration>
    struct iteration_sink
    {
      TIteration iteration;

      CPP_STREAMS__BODY (iteration_sink);

      explicit CPP_STREAMS__PRELUDE iteration_sink (TIteration iteration)
        : iteration (std::move (iteration))
      {
      }

      template<typename TValueType, typename TSource>
      CPP_STREAMS__SINK void consume (TSource && source) const
      {
        source (
          [iteration = iteration] (auto && v)
          {
            return iteration (v);
          });
      }
    };

    // ------------------------------------------------------------------------

    struct last_or_default_sink
    {
      CPP_STREAMS__BODY (last_or_default_sink);

      last_or_default_sink () = default;

      template<typename TValueType, typename TSource>
      CPP_STREAMS__SINK auto consume (TSource && source) const
      {
        using value_type = strip_type_t<TValueType>;

        value_type result {};

        source (
          [&result] (auto && v)
          {
            result = std::forward<decltype (v)> (v);
            return true;
          });

        return result;
      }
    };

    // ------------------------------------------------------------------------

    struct sum_sink
    {
      CPP_STREAMS__BODY (sum_sink);

      sum_sink () = default;

      template<typename TValueType, typename TSource>
      CPP_STREAMS__SINK auto consume (TSource && source) const
      {
        using value_type = strip_type_t<TValueType>;

        value_type result {};

        source (
          [&result] (auto && v)
          {
            result += std::forward<decltype (v)> (v);
            return true;
          });

        return result;
      }
    };

    // ------------------------------------------------------------------------

    struct vector_sink
    {
      CPP_STREAMS__BODY (vector_sink);

      vector_sink () = default;

      template<typename TValueType, typename TSource>
      CPP_STREAMS__SINK auto consume (TSource && source) const
      {
        using value_type = strip_type_t<TValueType>;

        std::vector<value_type> result;

        source (
          [&result] (auto && v)
          {
            result.push_back (std::forward<decltype (v)> (v));
            return true;
          });

        return result;
      }
    };

    // ------------------------------------------------------------------------

  }

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
  // Sources
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
      [] (auto && sink)
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
  CPP_STREAMS__PRELUDE auto filter (TPredicate predicate)
  {
    return detail::filter_pipe<TPredicate> (std::move (predicate));
  }

  // --------------------------------------------------------------------------

  template<typename TPredicate>
  CPP_STREAMS__PRELUDE auto map (TPredicate predicate)
  {
    return detail::map_pipe<TPredicate> (std::move (predicate));
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

  CPP_STREAMS__PRELUDE auto to_first_or_default ()
  {
    return detail::first_or_default_sink ();
  }

  // --------------------------------------------------------------------------

  template<typename TIteration>
  CPP_STREAMS__PRELUDE auto to_iter (TIteration iteration)
  {
    return detail::iteration_sink<TIteration> (std::move (iteration));
  }

  // --------------------------------------------------------------------------

  template<typename TState, typename TFolder>
  CPP_STREAMS__PRELUDE auto to_fold (TState initial, TFolder folder)
  {
    return detail::fold_sink<TFolder, TState> (std::move (folder), std::move (initial));
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto to_last_or_default ()
  {
    return detail::last_or_default_sink ();
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto to_sum ()
  {
    return detail::sum_sink ();
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto to_vector ()
  {
    return detail::vector_sink ();
  }

  // --------------------------------------------------------------------------

}

// ----------------------------------------------------------------------------
#endif // CPP_STREAMS__INCLUDE_GUARD
// ----------------------------------------------------------------------------
