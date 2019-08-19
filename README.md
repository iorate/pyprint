# pyprint
Python-like `print` for C++17

## Example

```cpp
#include <list>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <tuple>

#include "./pyprint.hpp"

int main() {
  using namespace std;
  using namespace pyprint;

  // Fundamental Types
  print(42, 3.5, "Hello, world!");
  // 42 3.5 Hello, world!

  // Ranges
  print(vector<vector<string>>{{"foo"s, "bar"s,}, {}, {"baz"s, "qux"s}});
  // [[foo, bar], [], [baz, qux]]

  // Sets and Maps
  print(set{42, 23, 50}, unordered_map<string, int>{{"hoge"s, 15}, {"fuga"s, 21}});
  // {23, 42, 50} {fuga: 21, hoge: 15}

  // Tuples
  print(pair{'A', "bcd"sv}, tuple{list{1, 2}, set{'A', 'B'}});
  // (A, bcd) ([1, 2], {A, B})

  // Keyword Arguments (sep, end, file, flush)
  print(42, L"Hello, error!", _file = wcerr, _sep = L'\n', _end = L"\n-- END --\n");
  // 42
  // Hello, error!
  // -- END --
}
```

## Author
[iorate](https://github.com/iorate) ([Twitter](https://twitter.com/iorate))

## License
pyprint is licensed under [Boost Software License 1.0](LICENSE_1_0.txt).
