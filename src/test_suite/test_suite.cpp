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

#include "stdafx.h"

#define TEST
#ifdef TEST

#include "../cpp_streams/cpp_streams.hpp"

int main()
{
  using namespace cpp_streams;

  int ints[] = {3,1,4,1,5};

  auto r =
        from_array (ints)
    >>  filter ([] (auto && v) { return v % 2 != 0; })
//    >>  to_fold (1, [] (auto && s, auto && v) { return s * v; })
//    >>  to_vector ()
    >>  to_sum ()
    ;

  std::cout << r << std::endl;

  return 0;
}
#else

#include "functional_tests.hpp"

int main()
{
  functional_tests::run_functional_tests ();
  functional_tests::run_performance_tests ();

  return 0;
}

#endif
