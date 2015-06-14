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

      explicit CPP_STREAMS__PRELUDE source_adapter (TSource && source) noexcept
        : source (std::forward<TSource> (source))
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
            source ([predicate, sink = std::forward<decltype(sink)> (sink)] (auto && v)
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
            source ([predicate, sink = std::forward<decltype(sink)> (sink)] (auto && v)
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
          [folder = folder, &state] (auto && v)
          {
            state = folder (std::move (state), std::forward<decltype (v)> (v));
            return true;
          });

        return state;
      }
    };

  }

  // --------------------------------------------------------------------------

  template<typename TIterator>
  CPP_STREAMS__PRELUDE auto from_iterators (TIterator begin, TIterator end)
  {
    return detail::adapt_source<decltype (*begin)> (
      [begin,end] (auto && sink)
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

  CPP_STREAMS__PRELUDE auto reverse (std::size_t reserve = 16) noexcept
  {
    return detail::reverse_pipe (reserve);
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto to_first_or_default () noexcept
  {
    return detail::first_or_default_sink ();
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto to_last_or_default () noexcept
  {
    return detail::last_or_default_sink ();
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto to_sum () noexcept
  {
    return detail::sum_sink ();
  }

  // --------------------------------------------------------------------------

  CPP_STREAMS__PRELUDE auto to_vector () noexcept
  {
    return detail::vector_sink ();
  }

  // --------------------------------------------------------------------------

  template<typename TIteration>
  CPP_STREAMS__PRELUDE auto to_iter (TIteration iteration) noexcept
  {
    return detail::iteration_sink<TIteration> (std::move (iteration));
  }

  // --------------------------------------------------------------------------

  template<typename TState, typename TFolder>
  CPP_STREAMS__PRELUDE auto to_fold (TState initial, TFolder folder) noexcept
  {
    return detail::fold_sink<TFolder, TState> (std::move (folder), std::move (initial));
  }

  // --------------------------------------------------------------------------

}

// ----------------------------------------------------------------------------
#endif // CPP_STREAMS__INCLUDE_GUARD
// ----------------------------------------------------------------------------
