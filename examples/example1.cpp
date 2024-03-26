#include <iostream>
#include <optional>
#include <vector>
#include <graph2x.hpp>

int main()
{
	auto counter = [v = 0]() mutable -> std::optional<int> {
		++v;
		if(v > 15) {
			return std::nullopt;
		}
		return v;
	};

	for(const auto& i: g2x::detail::rust_like_range(counter)) {
		std::cout << i << '\t';
	}

	std::cout << "test\n";
}