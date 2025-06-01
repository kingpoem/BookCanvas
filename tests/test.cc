// #include <cmath>
// #include <generator>
// #include <iostream>
// #include <numeric>
// #include <ranges>
// #include <vector>
//
// std::generator<int> myfunc(auto&& v) {
//     for (auto&& vi : v | std::views::filter([](auto&& x) { return x >= 2; })
//                          | std::views::transform([](auto&& x) { return sqrtf(x); })) {
//         co_yield vi;
//     }
// }
//
// int main() {
//     std::vector<int> v(10);
//     std::iota(v.begin(), v.end(), 1);
//     for (auto&& vi : myfunc(v)) {
//         std::cout << vi << std::endl;
//     }
//
//     return 0;
// }
//
//
// #include <tuple>
//
// int main() {
//     auto tup = std::tuple(1,1.0f, 'h');
//     auto first = std::get<0>(tup);
// }
