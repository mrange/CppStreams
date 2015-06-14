# CppStreams

CppStreams is a data pipeline library for C++14.

Example:

```c++
using namespace cpp_streams;

std::vector<int> ints {3,1,4,1,5,9,2,6,5,4};

// Produce a sum of even ints (+1)
auto sum =
      from (ints)
  >>  filter ([] (auto && v) {return v % 2 == 0;})  // Keep only even numbers
  >>  map ([] (auto && v) {return v + 1;})          // +1
  >>  to_sum ()                                     // Compute sum
  ;

std::cout << "SUM: " << sum << std::endl;
```

## Verified compilers
1. Visual Studio 2015
2. G++ 4.9.2
3. Clang++ 3.6.0

## Motivation

In functional programming languages, such as Haskell & SML, programmers have been working with
data pipelines for 30+ years.

"Recently" this has become available to mainstream programmers in
languages such as C# (2008, LINQ) and Java (2014, Streams) as these languages has begun adopting
more functional constructs.

With C++11 C++ got lambda expressions which is crucial  in order
to implement data pipelines. Several attempts have been made to implement data pipelines among
others [cpplinq](https://cpplinq.codeplex.com/) that I begun working on during summer 2012.

The motivation for CppStreams are:

1. Use push instead of pull as this seems to lead to more efficient pipelines.
2. Support of update of source container (cpplinq can't do this)
3. Use C++14 in order to make the library code more succinct and easier to extend.
4. Use functional naming conventions over C# LINQ naming conventions.
5. Having a bit of fun

## Inspiration

1. C# LINQ got me interested in data pipelines
2. [F#](https://github.com/Microsoft/visualfsharp) made me
   to rediscover FP
3. [nessos/Streams](https://github.com/nessos/Streams) is a data pipeline for F#/C# that relies on push
4. Even though I authored [cpplinq](https://cpplinq.codeplex.com/) it's still an inspiration for
   CppStreams in terms of goals and learning on what can be improved

## Status

### Source operators

| Status  | Source operator         | Comment                                            |
| --------|-------------------------|----------------------------------------------------|
| Done    | from                    | Creates a source from a STL container              |
| Done    | from_iterators          | Creates a source from a pair of iterators          |
| Planned | from_array              | Creates a source from a C-style array              |

### Pipe operators

| Status  | Pipe operator           | Comment                                            |
| --------|-------------------------|----------------------------------------------------|
| Done    | filter                  | Filter elements in pipeline using filter function  |
| Done    | map                     | Maps elements in pipeline using map function       |
| Done    | reverse                 | Reverses elements in pipeline                      |
| Planned | order_by                | Orders elements in pipeline using order function   |
| Planned | then_by                 | Orders elements in pipeline using order function   |
| Planned | union_with              | Union of two pipelines                             |
| Planned | intersect_with          | Intersection of two pipelines                      |
| Planned | expect_with             | Disjunction of two pipelines                       |
| Planned | join_with               | Joins two pipelines                                |

### Sink operators

| Status  | Sink operator           | Comment                                            |
| --------|-------------------------|----------------------------------------------------|
| Done    | to_first_or_default     | Returns the first element of pipeline or default   |
| Done    | to_last_or_default      | Returns the last element of pipeline or default    |
| Done    | to_sum                  | Returns sum of elements in pipeline                |
| Done    | to_vector               | Returns vector of elements in pipeline             |
| Done    | to_iter                 | Applies iteration function to elements in pipeline |
| Done    | to_fold                 | Applies fold function to elements in pipeline      |
| Planned | to_length               | Returns length of elements in pipeline             |
| Planned | to_map                  | Returns map of elements in pipeline                |
| Planned | to_set                  | Returns set of elements in pipeline                |
| Planned | to_lookup               | Returns lookup of elements in pipeline             |

# TODO

NEXT: 008

1. 000 - Complete status of operators
2. 001 - general: Capture by RValue reference as implied here: [capture-by-universal-reference](http://stackoverflow.com/questions/21238463/capture-by-universal-reference)
3. 002 - iteration_sink: Check return type, if void return false
4. 003 - from: Should capture the container by value if RValue reference
5. 004 - test: Figure out how to test negative type test cases (these will trigger compilation errors).
6. 005 - general: Not happy with the requirement to capture this in lambdas, find alternative
7. 006 - performance: Add performance tests
8. 007 - coverage: Add code coverage tests

