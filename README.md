Creating a separated values parser is a straightforward task, but one that meets these demanding criteria:

* Fast performance
* Dynamic column specification at runtime
* Users to preview column names prior to parsing
* Supports position-independent columns
* Manages line breaks within cells
* Allows user-defined data types

is considerably more difficult. This implementation successfully satisfies all these requirements.

Compiling the example:

```
c++ main.cpp rsv.cpp -std=c++20 -g
```

## Basic usage

```C++
auto sequence  = std::vector<std::string>();
auto charge    = std::vector<int>();
auto ascesions = std::vector<std::string>();
auto score     = std::vector<float>();

auto [file, econd] = rsv::open("tst.tsv");

if(econd)
{
    std::cerr << econd.message() << '\n';
    std::exit(1);
}

auto schema = rsv::schema({
    rsv::f("charge"   , charge),
    rsv::f("ascesions", ascesions),
    rsv::f("sequence" , sequence),
    rsv::f("score"    , score)}
);

rsv::read(file, schema, '\t');
```

## How to read based on column positions instead of column names?
You build the schema with the positions instead:
```C++
auto schema = rsv::schema({
    rsv::f(1, charge),
    rsv::f(2, ascesions),
    rsv::f(0, sequence),
    rsv::f(3, score)}
);
```
Just like with column name-based schema, the order in which the fields are presented in the schema does not matter.

## How to inspect column names before calling `rsv::read` ?

You can call `rsv::columns` when, for instance, you want to inspect the column names before building the schema:

```C++
auto cols = rsv::columns(file, '\t');
```
Since `rsv::read` expects all names in the schema to be present in the file, you are expected to call this function to perform the necessary checks.

## How to read values to a custom type?

This can be achieved by providing a custom field handler. A field handler is a callback function that processes the cell string and stores the result. 

In this example, we convert a string representing a point in time to `std::time_point`:

```C++

...
auto delegate_time = [](const std::string_view& src, void* dst)
{
    auto* casted = static_cast<std::vector<time_point<system_clock>>*>(dst);
    auto [success, value] = str_to_time(src);
    casted->push_back(value);
    return success;
};

auto time = std::vector<time_point<system_clock>();

auto schema = rsv::schema({
    rsv::f("time", time, delegate_time),
    ...
);
```

Where `str_to_time` converts a `std::string` to a `std::time_point`.


## What happens when values are missing?

When using the default handler the missing values are set to: an empty string if type is `std::string`; `std::numeric_limits<T>::quiet_NaN()` if `T` is a floating value type; `std::numeric_limits<T>::max()` if `T` is integer (`bool` not included).
