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
  >>  to_sum                                        // Compute sum
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

* - Currently doesn't work on VS2015 RC

| Prio | Status  | Source operator         | Comment                                            |
|-----:| --------|-------------------------|----------------------------------------------------|
|      | Done    | from                    | Creates a source from a STL container              |
|      | Done    | from_iterators          | Creates a source from a pair of iterators          |
|      | Done    | from_array              | Creates a source from a C-style array              |
|      | Done    | from_repeat             | Creates an source from a value and repeat count    |
|      | Done    | from_singleton          | Creates an source from a value                     |
|      | Done    | from_empty              | Creates an empty source                            |
|      | Done    | from_range*             | Creates a source from a range                      |
|    2 | Planned | from_unfold             | Creates an source from an unfold function          |
|    2 | Planned | from_generator          | Creates an source from a generator function        |

### Pipe operators

| Prio | Status  | Pipe operator           | Comment                                            |
|-----:| --------|-------------------------|----------------------------------------------------|
|      | Done    | filter                  | Filter elements in pipeline using filter function  |
|      | Done    | map                     | Maps elements in pipeline using map function       |
|      | Done    | reverse*                | Reverses elements in pipeline                      |
|      | Done    | append                  | Appends two pipelines                              |
|      | Done    | skip_while              | Skips while func is true for elements in pipeline  |
|      | Done    | take_while              | Takes while func is true for elements in pipeline  |
|      | Done    | mapi                    | Maps elements in pipeline using mapi function      |
|      | Done    | collect                 | Collects sub elements in pipeline                  |
|      | Done    | skip                    | Skips n elements in pipeline                       |
|      | Done    | take                    | Takes n elements in pipeline                       |
|      | Done    | sort*                   | Orders elements in pipeline using order function   |
|      | Done    | sort_by*                | Orders elements in pipeline using order function   |
|    1 | Planned | order_by                | Orders elements in pipeline using order function   |
|    1 | Planned | then_by                 | Orders elements in pipeline using order function   |
|    2 | Planned | choose                  | Chooses elements in pipeline                       |
|    2 | Planned | concat                  | Concats a pipeline of pipelines                    |
|    2 | Planned | distinct_by             | Unique elements in pipeline using select function  |
|    2 | Planned | partition               | Partitions elements in pipeline in two heaps       |
|    2 | Planned | reduce                  | Reduces elements in pipeline using reduce function |
|    2 | Planned | union_with              | Union of two pipelines                             |
|    2 | Planned | intersect_with          | Intersection of two pipelines                      |
|    2 | Planned | expect_with             | Disjunction of two pipelines                       |
|    3 | Planned | windowed                | Splits elements in pipeline in chunks              |
|    3 | Planned | compare_with            | Compares two pipelines                             |
|    3 | Planned | pairwise                | Makes pair of elements in pipeline                 |
|    3 | Planned | permute                 | Permutes elements in pipeline using permutes func  |
|    3 | Planned | join_with               | Joins two pipelines                                |

### Sink operators

| Prio | Status  | Sink operator           | Comment                                            |
|-----:| --------|-------------------------|----------------------------------------------------|
|      | Done    | to_first_or_default     | Returns the first element of pipeline or default   |
|      | Done    | to_last_or_default      | Returns the last element of pipeline or default    |
|      | Done    | to_sum                  | Returns sum of elements in pipeline                |
|      | Done    | to_vector               | Returns vector of elements in pipeline             |
|      | Done    | to_iter                 | Applies iteration function to elements in pipeline |
|      | Done    | to_fold                 | Applies fold function to elements in pipeline      |
|      | Done    | to_any                  | True if pipeline has any element matching predicate|
|      | Done    | to_all                  | True if all pipeline element matches predicate     |
|      | Done    | to_length               | Returns length of elements in pipeline             |
|      | Done    | to_set                  | Returns set of elements in pipeline                |
|    2 | Planned | to_map                  | Returns map of elements in pipeline                |
|    2 | Planned | to_max                  | Returns max of elements in pipeline                |
|    2 | Planned | to_min                  | Returns min of elements in pipeline                |
|    2 | Planned | to_average              | Returns average of elements in pipeline            |
|    3 | Planned | to_first                | Returns the first element of pipeline or empty     |
|    3 | Planned | to_split_at             | Splits a pipeline at index n                       |
|    3 | Planend | to_last                 | Returns the last element of pipeline or empty      |
|    3 | Planned | to_lookup               | Returns lookup of elements in pipeline             |
|    3 | Planned | to_scan                 | Applies scan function to elements in pipeline      |
|    3 | Planned | to_split_into           | Splits a pipeline into at most n chunks            |

# TODO

NEXT: 011

1. 010 - VS2015: Find work-around for busted pipes
1. 006 - performance: Add performance tests
1. 007 - coverage: Add code coverage tests
1. 001 - general: Capture by RValue reference as implied here: [capture-by-universal-reference](http://stackoverflow.com/questions/21238463/capture-by-universal-reference)
1. 002 - iteration_sink: Check return type, if void return false
1. 008 - general: Use static_assert to check argument types
1. 009 - general: Use std::forward in API functions
1. 003 - from: Should capture the container by value if RValue reference
1. 004 - test: Figure out how to test negative type test cases (these will trigger compilation errors).
1. 005 - general: Not happy with the requirement to capture this in lambdas, find alternative

## Done

1. 000 - Complete status of operators
