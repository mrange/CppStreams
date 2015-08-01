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
#ifndef CPP_STREAMS__FUNCTIONAL_TESTS__INCLUDE_GUARD
# define CPP_STREAMS__FUNCTIONAL_TESTS__INCLUDE_GUARD
// ----------------------------------------------------------------------------
# include "../cpp_streams/cpp_streams.hpp"

# include <chrono>
# include <cstdint>
# include <algorithm>
# include <iostream>
# include <iterator>
# include <sstream>
# include <string>
# include <tuple>
// ----------------------------------------------------------------------------
# define CPP_STREAMS__TEST()                  test_prelude (__FILE__, __LINE__, __FUNCTION__)
# define CPP_STREAMS__ERROR(msg)              test_error (__FILE__, __LINE__, __FUNCTION__, msg)
# define CPP_STREAMS__EQUAL(expected, actual) test_equal (__FILE__, __LINE__, __FUNCTION__, #expected, expected, #actual, actual)
// ----------------------------------------------------------------------------
// Functional test strategy:
//  Test functionalitty with
//    1. Empty list
//    2. Non-empty list
//    3. Simple and complex value types
//    4. Avoid auto for 'actual' to catch type transformation errors
//    6. Avoid auto && in lambda expressions passed to 'CppStreams' to catch type transformation errors
// ----------------------------------------------------------------------------
namespace functional_tests
{
  struct user
  {
    CPP_STREAMS__BODY (user);

    user () = default;

    bool operator == (user const & o) const
    {
      return
            true
        &&  id                == o.id
        &&  first_name        == o.first_name
        &&  last_name         == o.last_name
        &&  lottery_numbers   == o.lottery_numbers
        ;
    }

    std::uint64_t     id              ;
    std::string       first_name      ;
    std::string       last_name       ;
    std::vector<int>  lottery_numbers ;

    bool operator < (user const & o) const
    {
      return id < o.id;
    }
  };

  template<typename TOne, typename TTwo>
  std::ostream & operator << (std::ostream & s, std::tuple<TOne, TTwo> const & v)
  {
    s
      << "{"
      << std::get<0> (v)
      << ", "
      << std::get<1> (v)
      << "}"
      ;

    return s;
  }

  template<typename TValueType>
  std::ostream & operator << (std::ostream & s, std::vector<TValueType> const & vs)
  {
    s
      << "[("
      << vs.size ()
      << ")"
      ;

      for (auto && v : vs )
      {
        s
          << ", "
          << v
          ;
      }

    s
      << "]"
      ;

    return s;
  }

  template<typename TValueType>
  std::ostream & operator << (std::ostream & s, std::set<TValueType> const & vs)
  {
    s
      << "{("
      << vs.size ()
      << ")"
      ;

      for (auto && v : vs )
      {
        s
          << ", "
          << v
          ;
      }

    s
      << "}"
      ;

    return s;
  }

  template<typename TKeyType, typename TValueType>
  std::ostream & operator << (std::ostream & s, std::map<TKeyType, TValueType> const & vs)
  {
    s
      << "{("
      << vs.size ()
      << ")"
      ;

      for (auto && v : vs )
      {
        s
          << ", {"
          << v.first
          << ", "
          << v.second
          << "}"
          ;
      }

    s
      << "}"
      ;

    return s;
  }

  std::ostream & operator << (std::ostream & s, user const & v)
  {
    s
      << "{user"
      << ", id:"
      << v.id
      << ", first_name:'"
      << v.first_name
      << "', last_name:'"
      << v.last_name
      << "', lottery_numbers:'"
      << v.lottery_numbers
      << "'}"
      ;

    return s;
  }

  constexpr std::size_t operator "" _sz (unsigned long long n)
  {
    return static_cast<std::size_t> (n);
  }

  std::size_t             errors_detected = 0;

  std::vector<int>  const empty_ints  {};
  std::vector<int>  const some_ints   {3,1,4,1,5,9,2,6,5,3,5,8,9,7,9,};

  user                    empty_user;

  std::vector<user> const empty_users {};
  std::vector<user> const some_users
  {
    {1001, "Bill"   , "Gates" , {1,2,3,4,5,6      }},
    {1002, "Melinda", "Gates" , {1,4,9,16,25,36   }},
    {1003, "Steve"  , "Jobs"  , {36,35,34,33,32,31}},
  };

  void test_error (char const * /*file_name*/, int line_no, char const * /*function_name*/, char const * message)
  {
    ++errors_detected;
    std::cout
      << "ERROR - Line "
      << line_no
      << " : "
      << message
      << std::endl
      ;
  }

  template<typename TExpected, typename TActual>
  bool test_equal (
      char const *  file_name
    , int           line_no
    , char const *  function_name
    , char const *  expected_name
    , TExpected &&  expected
    , char const *  actual_name
    , TActual &&    actual
    )
  {
    if (expected == actual)
    {
      return true;
    }
    else
    {
      std::ostringstream stream;

      stream
        << expected_name
        << " ("
        << expected
        << ") == "
        << actual_name
        << " ("
        << actual
        << ")"
        ;

      auto message = stream.str ();

      test_error (file_name, line_no, function_name, message.c_str ());

      return false;
    }
  }

  auto identity = [] (auto && v)
  {
    return std::forward<decltype (v)> (v);
  };

  auto map_tostring = [] (auto && v)
  {
    return std::to_string (std::forward<decltype(v)> (v));
  };

  auto map_id = [] (user const & v)
  {
    return v.id;
  };

  auto map_true = [] (auto &&)
  {
    return true;
  };

  auto map_false = [] (auto &&)
  {
    return false;
  };

  template<typename TContainer, typename TPredicate>
  auto compute_sum (TContainer && container, TPredicate && predicate)
  {
    using value_type = decltype (predicate (container.front ()));

    value_type sum {};

    for (auto && v : container)
    {
      sum += predicate (std::forward<decltype (v)> (v));
    }

    return sum;
  }

  void test_prelude (char const * /*file_name*/, int /*line_no*/, char const * function_name)
  {
    std::cout
      << "Running: "
      << function_name
      << std::endl
      ;
  }

  void test__from ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<int> expected  {};
      std::vector<int> actual    =
            from (empty_ints)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int> expected  = some_ints;
      std::vector<int> actual    =
            from (some_ints)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__from_range ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      int begin     = 10;
      int expected  = 0;
      int actual    =
            from_range (begin, 0)
        >>  to_sum
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int end       = 10;
      int expected  = 0;
      int actual    =
            from_range (end, end)
        >>  to_sum
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = 10*(10 - 1) / 2;
      int actual    =
            from_range (0, 10)
        >>  to_sum
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int end       = 10;
      int expected  = 9;
      int actual    =
            from_range (8, end)
        >>  to_last_or_default
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__from_array ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    int ints [] {3,1,4};

    {
      int expected  = 8;
      int actual    = from_array (ints) >> to_sum;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__from_repeat ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<user> expected  = empty_users;
      std::vector<user> actual    =
            from_repeat (empty_user, 0U)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int> expected  {3,3,3};
      std::vector<int> actual    =
            from_repeat (3, 3U)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__from_singleton ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<user> expected  { some_users[0] };
      std::vector<user> actual    =
            from_singleton (some_users[0])
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__from_empty ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<user> expected  {};
      std::vector<user> actual    =
            from_empty<user> ()
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__to_all ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      bool expected = false;
      bool actual   = from (empty_users) >> to_all (map_true);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      bool expected = false;
      bool actual   = from (some_users) >> to_all (map_false);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      bool expected = true;
      bool actual   = from (some_users) >> to_all (map_true);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      bool expected = true;
      bool actual   = from (some_ints) >> to_all ([] (auto && v) {return v > 0;});
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__to_any ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      bool expected = false;
      bool actual   = from (empty_users) >> to_any (map_true);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      bool expected = false;
      bool actual   = from (some_users) >> to_any (map_false);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      bool expected = true;
      bool actual   = from (some_users) >> to_any (map_true);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      bool expected = true;
      bool actual   = from (some_ints) >> to_any ([] (auto && v) {return v > 8;});
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__to_first_or_default ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      user expected = empty_user;
      user actual   = from (empty_users) >> to_first_or_default;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = some_ints.front ();
      int actual    = from (some_ints) >> to_first_or_default;
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__to_last_or_default ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      user expected   = empty_user;
      user actual     = from (empty_users) >> to_last_or_default;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected    = some_ints.back ();
      int actual      = from (some_ints) >> to_last_or_default;
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__to_length ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::size_t expected  = 0;
      std::size_t actual    = from (empty_ints) >> to_length;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::size_t expected  = some_users.size ();
      std::size_t actual    = from (some_users) >> to_length;
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

// TODO: Fix to_map for VS2015 RC
  void test__to_map ()
  {
#ifndef _MSC_VER
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto apply_map = [] (auto && key_selector, auto && vs)
    {
      using value_type        = detail::strip_type_t<decltype (vs.front ())>      ;
      using key_selector_type = decltype (key_selector)                           ;
      using selected_key_type = std::result_of_t<key_selector_type (value_type)>  ;
      using key_type          = detail::strip_type_t<selected_key_type>           ;
      using map_type          = std::map<key_type, value_type>                    ;
      using item_type         = typename map_type::value_type                     ;
      map_type result;

      for (auto && v : vs)
      {
        auto key = key_selector (v);
        result.insert (item_type (std::move (key), std::forward<decltype (v)> (v)));
      }

      return result;
    };


    {
      std::map<int, int> expected           = apply_map (identity, empty_ints);
      std::map<int, int> actual             = from (empty_ints) >> to_map (identity);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::map<int, int> expected           = apply_map (identity, some_ints);
      std::map<int, int> actual             = from (some_ints) >> to_map (identity);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::map<std::uint64_t, user> expected= apply_map (map_id, some_users);
      std::map<std::uint64_t, user> actual  = from (some_users) >> to_map (map_id);
      CPP_STREAMS__EQUAL (expected, actual);
    }
#endif
  }

  void test__to_max ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      int expected  = -1;
      int actual    = from (empty_ints) >> to_max (-1);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::uint64_t expected  = 1003;
      std::uint64_t actual    = from (some_users) >> map (map_id) >> to_max (0U);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = 9;
      int actual    = from (some_ints) >> to_max (0);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = 10;
      int actual    = from (some_ints) >> to_max (10);
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__to_min ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      int expected  = 100;
      int actual    = from (empty_ints) >> to_min (100);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::uint64_t expected  = 1001;
      std::uint64_t actual    = from (some_users) >> map (map_id) >> to_min (10000U);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = 1;
      int actual    = from (some_ints) >> to_min (100);
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = 0;
      int actual    = from (some_ints) >> to_min (0);
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__to_set ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto apply_set = [] (auto && vs)
    {
      using value_type = detail::strip_type_t<decltype (vs.front ())>;
      std::set<value_type> result;

      for (auto && v : vs)
      {
        result.insert (std::forward<decltype (v)> (v));
      }

      return result;
    };


    {
      std::set<int> expected  = apply_set (empty_ints);
      std::set<int> actual    = from (empty_ints) >> to_set;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::set<int> expected  = apply_set (some_ints);
      std::set<int> actual    = from (some_ints) >> to_set;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::set<user> expected = apply_set (some_users);
      std::set<user> actual   = from (some_users) >> to_set;
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__to_sum ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      int expected  = 0;
      int actual    = from (empty_ints) >> to_sum;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = compute_sum (some_ints, identity);
      int actual    = from (some_ints) >> to_sum;
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__to_vector ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<int> expected = empty_ints;
      std::vector<int> actual   = from (empty_ints) >> to_vector;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected  = some_users;
      std::vector<user> actual    = from (some_users) >> to_vector;
      CPP_STREAMS__EQUAL (expected, actual);
    }
  }

  void test__to_iter ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      int expected  = 0;
      int actual    = 0;
          from (empty_ints)
      >>  to_iter ([&actual] (int v) { actual += v; return true; })
      ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::uint64_t expected  = 0;
      std::uint64_t actual    = 0;
          from (empty_users)
      >>  to_iter ([&actual] (user const & v) { actual += v.id; return false; })
      ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = compute_sum (some_ints, identity);
      int actual    = 0;
          from (some_ints)
      >>  to_iter ([&actual] (int v) { actual += v; return true; })
      ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::uint64_t expected  = some_users.front ().id;
      std::uint64_t actual    = 0;
          from (some_users)
      >>  to_iter ([&actual] (user const & v) { actual += v.id; return false; })
      ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__to_fold ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto fold_int   = [] (int s, int v) { return s + v; };
    auto fold_user  = [] (std::uint64_t s, user const & v) { return s + v.id; };

    {
      int expected  = 0;
      int actual    =
            from (empty_ints)
        >>  to_fold (0, fold_int)
      ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      int expected  = compute_sum (some_ints, identity);
      int actual    =
            from (some_ints)
        >>  to_fold (0, fold_int)
      ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::uint64_t expected  = compute_sum (some_users, map_id);
      std::uint64_t actual    =
            from (some_users)
        >>  to_fold (0ULL, fold_user)
      ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__append ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<int> expected {};
      std::vector<int> actual   =
            from (empty_ints)
        >>  append (from (empty_ints))
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = some_users;
      std::vector<user> actual   =
            from (some_users)
        >>  append (from (empty_users))
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int> expected = some_ints;
      std::vector<int> actual   =
            from (empty_ints)
        >>  append (from (some_ints))
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      auto double_users = some_users;
      double_users.reserve (double_users.size () * 2);
      std::copy (
          some_users.begin ()
        , some_users.end ()
        , std::back_inserter (double_users)
        );
      std::vector<user> expected = double_users;
      std::vector<user> actual   =
            from (some_users)
        >>  append (from (some_users))
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__collect ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto collect_simple   = [] (user const & u) { return from (u.lottery_numbers); };
    auto collect_advanced = [] (user const & u) { return from (u.lottery_numbers) >> map (map_tostring); };

    {
      std::vector<int>  expected  {};
      std::vector<int>  actual    =
            from (empty_users)
        >>  collect (collect_simple)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected  = some_users[0].lottery_numbers;
      std::vector<int>  actual    =
            from_singleton (some_users[0])
        >>  collect (collect_simple)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<std::string>  expected  =
            from (some_users[0].lottery_numbers)
        >>  map (map_tostring)
        >> to_vector
        ;
      std::vector<std::string>  actual    =
            from_singleton (some_users[0])
        >>  collect (collect_advanced)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    // These tests fails in VS2015 RC
#ifndef _MSC_VER
    auto apply_collect = [] (auto && collect, auto && vs)
    {
      using source_type = detail::strip_type_t<decltype (collect (vs.front ()))>;
      using value_type  = detail::strip_type_t<typename source_type::value_type>;
      std::vector<value_type> result;
      for (auto && v : vs)
      {
            collect (std::forward<decltype(v)> (v))
        >>  to_iter ([&] (auto && iv)
            {
              result.push_back (std::forward<decltype(iv)> (iv));
              return true;
            })
        ;
      }
      return result;
    };

    {
      std::vector<int>  expected  = apply_collect (collect_simple, some_users);
      std::vector<int>  actual    =
            from (some_users)
        >>  collect (collect_simple)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<std::string>  expected  = apply_collect (collect_advanced, some_users);
      std::vector<std::string>  actual    =
            from (some_users)
        >>  collect (collect_advanced)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }
#endif

  }

  void test__filter ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto filter_int   = [] (int v) { return v % 2 == 0; };
    auto filter_user  = [] (user const & v) { return v.last_name == "Gates"; };

    auto apply_filter = [] (auto && filter, auto && vs)
    {
      using value_type = detail::strip_type_t<decltype (vs.front ())>;
      std::vector<value_type> result;
      result.reserve (vs.size ());
      std::copy_if (
          vs.begin ()
        , vs.end ()
        , std::back_inserter (result)
        , filter
        );
      return result;
    };

    {
      std::vector<int> expected {};
      std::vector<int> actual   =
            from (empty_ints)
        >>  filter (filter_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int> expected = apply_filter (filter_int, some_ints);
      std::vector<int> actual   =
            from (some_ints)
        >>  filter (filter_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected  = apply_filter (filter_user, some_users);
      std::vector<user> actual    =
            from (some_users)
        >>  filter (filter_user)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__map ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto map_int   = map_tostring ;
    auto map_user  = map_id       ;

    auto apply_map = [] (auto && map, auto && vs)
    {
      using value_type = detail::strip_type_t<decltype (map (vs.front ()))>;
      std::vector<value_type> result;
      result.reserve (vs.size ());
      std::transform (
          vs.begin ()
        , vs.end ()
        , std::back_inserter (result)
        , map
        );
      return result;
    };

    {
      std::vector<std::string> expected {};
      std::vector<std::string> actual   =
            from (empty_ints)
        >>  map (map_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<std::string> expected = apply_map (map_int, some_ints);
      std::vector<std::string> actual   =
            from (some_ints)
        >>  map (map_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<std::uint64_t> expected = apply_map (map_user, some_users);
      std::vector<std::uint64_t> actual   =
            from (some_users)
        >>  map (map_user)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__mapi ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto mapi_int   = [] (std::size_t idx, int v)
    {
      return std::make_tuple (idx, std::to_string (v));
    };
    auto mapi_user  = [] (std::size_t idx, user const & v)
    {
      return std::make_tuple (idx, v.id);
    };

    auto apply_mapi = [] (auto && mapi, auto && vs)
    {
      using value_type = detail::strip_type_t<decltype (mapi (0U, vs.front ()))>;
      std::vector<value_type> result;
      result.reserve (vs.size ());
      std::size_t idx = 0;
      for (auto && v : vs)
      {
        result.push_back (mapi (idx++, std::forward<decltype(v)> (v)));
      }
      return result;
    };

    {
      std::vector<std::tuple<std::size_t, std::string>> expected {};
      std::vector<std::tuple<std::size_t, std::string>> actual   =
            from (empty_ints)
        >>  mapi (mapi_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<std::tuple<std::size_t, std::string>> expected = apply_mapi (mapi_int, some_ints);
      std::vector<std::tuple<std::size_t, std::string>> actual   =
            from (some_ints)
        >>  mapi (mapi_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<std::tuple<std::size_t, std::uint64_t>> expected = apply_mapi (mapi_user, some_users);
      std::vector<std::tuple<std::size_t, std::uint64_t>> actual   =
            from (some_users)
        >>  mapi (mapi_user)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

// TODO: Fix reverse for VS2015 RC
  void test__reverse ()
  {
#ifndef _MSC_VER
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto apply_reverse = [] (auto && vs)
    {
      using value_type = detail::strip_type_t<decltype (vs.front ())>;
      std::vector<value_type> result;
      result.reserve (vs.size ());
      std::reverse_copy (
          vs.begin ()
        , vs.end ()
        , std::back_inserter (result)
        );
      return result;
    };

    {
      std::vector<int> expected {};
      std::vector<int> actual   =
            from (empty_ints)
        >>  reverse
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int> expected = apply_reverse (some_ints);
      std::vector<int> actual   =
            from (some_ints)
        >>  reverse
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = apply_reverse (some_users);
      std::vector<user> actual   =
            from (some_users)
        >>  reverse
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

#endif
  }

  void test__skip ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<int>  expected = {};
      std::vector<int>  actual   =
            from (some_ints)
        >>  skip (10000)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected {};
      std::vector<int>  actual   =
            from (empty_ints)
        >>  skip (0)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = some_users;
      std::vector<user> actual   =
            from (some_users)
        >>  skip (0)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected = {9, 2, 6, 5, 3, 5, 8, 9, 7, 9};
      std::vector<int>  actual   =
            from (some_ints)
        >>  skip (5)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__skip_while ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<int>  expected = {};
      std::vector<int>  actual   =
            from (some_ints)
        >>  skip_while (map_true)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected {};
      std::vector<int>  actual   =
            from (empty_ints)
        >>  skip_while ([] (auto &&) { return false; })
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = some_users;
      std::vector<user> actual   =
            from (some_users)
        >>  skip_while ([] (auto &&) { return false; })
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected = {9, 2, 6, 5, 3, 5, 8, 9, 7, 9};
      std::vector<int>  actual   =
            from (some_ints)
        >>  skip_while ([] (auto && v) { return v < 9; })
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

// TODO: Fix sort for VS2015 RC
  void test__sort ()
  {
#ifndef _MSC_VER
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto apply_sort = [] (auto && sorter, auto && vs)
    {
      using value_type = detail::strip_type_t<decltype (vs.front ())>;
      std::vector<value_type> result = std::forward<decltype (vs)> (vs);
      std::sort (
          result.begin ()
        , result.end ()
        , sorter
        );
      return result;
    };

    auto sorter_int   = [] (auto && l, auto && r) { return l < r; };
    auto sorter_user  = [] (user const & l, user const & r) { return l.id < r.id; };

    {
      std::vector<int> expected {};
      std::vector<int> actual   =
            from (empty_ints)
        >>  sort (sorter_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int> expected = apply_sort (sorter_int, some_ints);
      std::vector<int> actual   =
            from (some_ints)
        >>  sort (sorter_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = apply_sort (sorter_user, some_users);
      std::vector<user> actual   =
            from (some_users)
        >>  sort (sorter_user)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

#endif
  }

// TODO: Fix sort_by for VS2015 RC
  void test__sort_by ()
  {
#ifndef _MSC_VER
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto apply_sort_by = [] (auto && selector, auto && vs)
    {
      using value_type = detail::strip_type_t<decltype (vs.front ())>;
      std::vector<value_type> result = std::forward<decltype (vs)> (vs);
      auto sorter = [selector] (auto && l, auto && r)
        {
          return selector (std::forward<decltype (l)> (l)) < selector (std::forward<decltype (r)> (r));
        };
      std::sort (
          result.begin ()
        , result.end ()
        , sorter
        );
      return result;
    };

    auto selector_int   = identity;
    auto selector_user  = map_id;

    {
      std::vector<int> expected {};
      std::vector<int> actual   =
            from (empty_ints)
        >>  sort_by (selector_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int> expected = apply_sort_by (selector_int, some_ints);
      std::vector<int> actual   =
            from (some_ints)
        >>  sort_by (selector_int)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = apply_sort_by (selector_user, some_users);
      std::vector<user> actual   =
            from (some_users)
        >>  sort_by (selector_user)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

#endif
  }

  void test__take ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<int>  expected = {};
      std::vector<int>  actual   =
            from (some_ints)
        >>  take (0)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected {};
      std::vector<int>  actual   =
            from (empty_ints)
        >>  take (0)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = some_users;
      std::vector<user> actual   =
            from (some_users)
        >>  take (10000)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected = {3,1,4,1};
      std::vector<int>  actual   =
            from (some_ints)
        >>  take (4)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__take_while ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    {
      std::vector<int>  expected = {};
      std::vector<int>  actual   =
            from (some_ints)
        >>  take_while ([] (auto &&) { return false; })
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected {};
      std::vector<int>  actual   =
            from (empty_ints)
        >>  take_while (map_false)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<user> expected = some_users;
      std::vector<user> actual   =
            from (some_users)
        >>  take_while (map_true)
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

    {
      std::vector<int>  expected = {3,1,4,1};
      std::vector<int>  actual   =
            from (some_ints)
        >>  take_while ([] (auto && v) { return v < 5; })
        >>  to_vector
        ;
      CPP_STREAMS__EQUAL (expected, actual);
    }

  }

  void test__example ()
  {
    CPP_STREAMS__TEST ();

    // Smoke tests the example on the homepage

    using namespace cpp_streams;

    std::vector<int> ints {3,1,4,1,5,9,2,6,5,4};

    // Produce a sum of even ints (+1)
    auto sum =
          from (ints)
      >>  filter ([] (auto && v) {return v % 2 == 0;})  // Keep only even numbers
      >>  map ([] (auto && v) {return v + 1;})          // +1
      >>  to_sum                                        // Compute sum
      ;

    std::cout << "SUM: " << sum << std::endl;
  }

  void test__mutating_source ()
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto expected =
          from (some_ints)
      >>  map ([] (auto && v) {return v % 2 == 0 ? v + 1 : v;})
      >>  to_vector
      ;
    auto actual = some_ints;
        from (actual)
    >>  filter ([] (auto && v) {return v % 2 == 0;})
// TODO: Investigate how to test negative tests case like below
//    >>  map ([] (auto && v) {return v + 1;})
    >>  to_iter ([] (auto && v) { v += 1; return true; });
    CPP_STREAMS__EQUAL (expected, actual);
  }

  void test__basic ()
  {
    CPP_STREAMS__TEST ();

    // Smoke tests a simple query

    using namespace cpp_streams;

    auto query =
          from (some_ints)
      >>  filter ([] (auto && v) {return v % 2 == 0;})
      >>  map ([] (auto && v) {return v + 1;})
//      >>  reverse ()
//      >>  map ([] (auto && v) {return std::to_string (v);})
      ;

    std::cout << "SUM: " << (query >> to_sum) << std::endl;
    std::cout << "FIRST: " << (query >> to_first_or_default) << std::endl;
    std::cout << "LAST: " << (query >> to_last_or_default) << std::endl;
  }

  void test__complex ()
  {
    CPP_STREAMS__TEST ();

    // Smoke tests a complex query

    using namespace cpp_streams;

    auto query =
          from (some_users)
      >>  filter ([] (auto && v) {return v.id != 0;})
      >>  collect ([] (auto && v) {return from (v.lottery_numbers);})
      >>  take_while ([] (auto && v) {return v < 10;})
      >>  map ([] (auto && v) {return std::to_string (v);})
      >>  to_vector
      ;

  }

  void run_functional_tests ()
  {
    std::cout
      << "Running functional tests..."
      << std::endl
      ;

    test__from                ();
    test__from_range          ();
    test__from_array          ();
    test__from_repeat         ();
    test__from_singleton      ();
    test__from_empty          ();

    test__append              ();
    test__collect             ();
    test__filter              ();
    test__map                 ();
    test__mapi                ();
    test__reverse             ();
    test__skip                ();
    test__skip_while          ();
    test__sort                ();
    test__sort_by             ();
    test__take                ();
    test__take_while          ();

    test__to_all              ();
    test__to_any              ();
    test__to_first_or_default ();
    test__to_last_or_default  ();
    test__to_length           ();
    test__to_map              ();
    test__to_max              ();
    test__to_min              ();
    test__to_set              ();
    test__to_sum              ();
    test__to_vector           ();
    test__to_iter             ();
    test__to_fold             ();

    test__mutating_source     ();

    // test__example             ();
    // test__basic               ();
    // test__complex             ();

    if (errors_detected > 0)
    {
      std::cout
        << "Detected "
        << errors_detected
        << " functional errors"
        << std::endl
        ;
    }
    else
    {
      std::cout
        << "No functional errors detected"
        << std::endl
        ;
    }
  }

  template<typename TPredicate>
  auto time_it (std::size_t count, TPredicate && predicate)
  {
    auto then = std::chrono::high_resolution_clock::now ();

    for (auto iter = 0U; iter < count; ++iter)
    {
      predicate ();
    }

    auto now = std::chrono::high_resolution_clock::now ();

    auto diff = now - then;

    return std::chrono::duration_cast<std::chrono::milliseconds> (diff);
  }

  auto create_vector (int inner)
  {
    std::vector<int> ints;
    ints.reserve (inner);

    for (auto iter = 0; iter < inner; ++iter)
    {
      ints.push_back (iter);
    }

    return ints;
  }

  void performance__simple_pipe_line (int outer, int inner)
  {
    CPP_STREAMS__TEST ();

    using namespace cpp_streams;

    auto ints = create_vector (inner);

    {
      auto cs_func = [] (auto && vs)
      {
        return
              from (vs)
          >>  filter ([] (auto && v) {return v % 2 == 0;})
          >>  map ([] (auto && v) {return v + 1;})
          >>  to_sum
          ;
      };

      std::cout << "cs_sum: " << cs_func (ints) << std::endl;

      auto cs_time = time_it (outer, [&] () { cs_func (ints); });

      std::cout << "cs_time: " << cs_time.count () << " ms" << std::endl;
    }

    {
      auto classic_func = [] (auto && vs)
      {
        auto sum = 0;

        for (auto && v : vs)
        {
          if (v % 2 == 0)
          {
            sum += (v + 1);
          }
        }

        return sum;
      };

      std::cout << "classic_sum: " << classic_func (ints) << std::endl;

      auto classic_time = time_it (outer, [&] () { classic_func (ints); });

      std::cout << "classic_time: " << classic_time.count () << " ms" << std::endl;
    }
  }

  void run_performance_tests ()
  {
    std::cout
      << "Running performance tests..."
      << std::endl
      ;

    performance__simple_pipe_line     (100000, 10000);
  }

}
// ----------------------------------------------------------------------------
#endif // CPP_STREAMS__FUNCTIONAL_TESTS__INCLUDE_GUARD
// ----------------------------------------------------------------------------
