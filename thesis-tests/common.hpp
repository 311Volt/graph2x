
#ifndef G2X_THESIS_TESTS_COMMON_HPP
#define G2X_THESIS_TESTS_COMMON_HPP

#include <ranges>
#include <string>
#include <iostream>
#include <chrono>

#include <reflect>
#include <fstream>

#include <graph2x.hpp>

namespace mgr {

	void save_test_results(std::ranges::range auto&& results, std::ostream& os = std::cout) {

		using row_type = std::ranges::range_value_t<decltype(results)>;

		reflect::for_each<row_type>([&](auto I) {
			os << reflect::member_name<I, row_type>();
			if(I != reflect::size<row_type>() - 1) {
				os << ",";
			}
		});
		os << "\n";

		for(auto&& row: results) {
			reflect::for_each([&](auto I) {
				os << reflect::get<I>(row);
				if(I != reflect::size<row_type>() - 1) {
					os << ",";
				}
			}, row);
			os << "\n";
		}

	}

	void save_test_results(std::ranges::range auto&& results, const std::string& filename) {
		std::ofstream os(filename);
		save_test_results(results, os);
	}


	class stopwatch {
	public:

		stopwatch() {
			start = std::chrono::high_resolution_clock::now();
		}

		[[nodiscard]] double peek() const {
			return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
	};

	template<typename T>
	class average {
	public:

		void add(const T& value) {
			T y = value - comp;
			T t = total + y;
			comp = (t - total) - y;
			total = t;
			// total += value;
			++samples;
		}

		[[nodiscard]] T get() const {
			if(samples == 0) {
				return {};
			}
			return total / samples;
		}
	private:
		T total = 0;
		T comp = 0;
		int samples = 0;
	};

}


#endif //G2X_THESIS_TESTS_COMMON_HPP
