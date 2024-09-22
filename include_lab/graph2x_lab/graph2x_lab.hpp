
#ifndef GRAPH2X_LAB_HPP
#define GRAPH2X_LAB_HPP

#include <string>
#include <functional>
#include <generator>
#include <math.h>

#include <reflect>
#include <BS_thread_pool.hpp>
#include <iostream>
#include <fstream>
#include <random>

#define G2X_ATTR(MemPtr, Value) \
	template<auto MP> \
		requires MP == MemPtr \
	static constexpr auto g2x__get_attr() { \
		return Value; \
	}


namespace g2x::lab {


	struct label {
		std::string text;
	};

	struct title {
		std::string text;
	};

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

	template<typename TestT>
	struct test_run {

		using x_axis_type = typename TestT::x_axis;
		using y_axis_type = typename TestT::y_axis;

		std::string short_title = "test";
		std::string title;
		int samples_per_point = 5;
		TestT test_instance;
		std::mt19937_64 rng = {};
		std::generator<double> x_axis;
		bool save_to_csv = false;
		bool save_to_pgfplots = false;
	};

	inline double linspace_at(double start, double stop, int num, int i) {
		return start + (i * (stop-start)) / (num-1);
	}

	inline double logspace_at(double start, double stop, int num, int i) {
		double exp_start = std::log(start);
		double exp_stop = std::log(stop);
		return std::exp(linspace_at(exp_start, exp_stop, num, i));
	}

	inline std::generator<double> linspace(double start, double stop, int num=50) {
		if(num < 2) {
			throw std::invalid_argument("0- and 1-point ranges not supported");
		}
		for(int i=0; i<num; i++) {
			co_yield linspace_at(start, stop, num, i);
		}
	}

	inline std::generator<double> logspace(double start, double stop, int num=50) {
		if(num < 2) {
			throw std::invalid_argument("0- and 1-point ranges not supported");
		}
		for(int i=0; i<num; i++) {
			co_yield logspace_at(start, stop, num, i);
		}
	}

	template<typename TestT>
	using test_run_result_t = std::vector<std::pair<typename TestT::x_axis, typename TestT::y_axis>>;

	inline std::string latex_escape_string(const std::string_view str) {
		static std::unordered_map<char, std::string> chr_map {
			{'#', "\\#"},
			{'$', "\\$"},
			{'%', "\\%"},
			{'&', "\\&"},
			{'~', "\\~{}"},
			{'_', "\\_"},
			{'^', "\\^{}"},
			{'\\', "\\textbackslash{}"},
			{'{', "\\{"},
			{'}', "\\}"},
		};
		std::string result;
		for(char c: str) {
			if(chr_map.contains(c)) {
				result += chr_map.at(c);
			} else {
				result += c;
			}
		}
		return result;
	}

	template<typename TestT>
	auto render_to_csv(const std::ostream& os_c, const test_run<TestT>& run, const test_run_result_t<TestT>& results) {

		auto& os = const_cast<std::ostream&>(os_c);

		using x_axis_t = typename TestT::x_axis;
		using y_axis_t = typename TestT::y_axis;

		reflect::for_each<x_axis_t>([&](auto I) {
			os << reflect::member_name<I, x_axis_t>() << ",";
		});

		reflect::for_each<y_axis_t>([&](auto I) {
			os << reflect::member_name<I, y_axis_t>();
			if(I < reflect::size<y_axis_t>() - 1) {
				os << ",";
			}
		});
		os << "\n";

		for(const auto& [x, y]: results) {
			reflect::for_each([&](auto I) {
				os << reflect::get<I>(x) << ",";
			}, x);

			reflect::for_each([&](auto I) {
				os << reflect::get<I>(y);
				if(I < reflect::size<y_axis_t>() - 1) {
					os << ",";
				}
			}, y);
			os << "\n";
		}
	}

	template<typename TestT>
	auto render_to_pgfplots(const std::ostream& os_c, const test_run<TestT>& run, const test_run_result_t<TestT>& results) {

		auto& os = const_cast<std::ostream&>(os_c);

		using x_axis_t = typename TestT::x_axis;
		using y_axis_t = typename TestT::y_axis;

		std::vector<std::string> legend_positions = {"north east", "north west"};
		std::vector<std::string> plot_colors = {"blue", "red", "green"};
		std::vector<std::string> axis_y_lines = {"left", "right"};

		os << "\\begin{tikzpicture}";

		reflect::for_each<y_axis_t>([&](auto I) {

			os << std::format(R"||||(
	\begin{{axis}}[
		axis y line*={},
		title={{ {} }},
		xlabel={{ {} }},
		ylabel={{ YLABEL PLACEHOLDER }},
		legend pos={},
		ymajorgrids=true,
		grid style=dashed,
	]
	\addplot[
		color={},
		% mark=square,
	]
	coordinates {{
)||||"
			, axis_y_lines.at(I), run.title,
			  latex_escape_string(reflect::member_name<0, x_axis_t>()),
			  legend_positions.at(I), plot_colors.at(I));
			os << "\t\t\t";
			for(const auto& [x, y]: results) {
				os << std::format("({}, {})", reflect::get<0>(x), reflect::get<I>(y));
			}
			os << std::format(R"||||(
	}};
	\legend{{ {} }}

	\end{{axis}}

)||||"
			, latex_escape_string(reflect::member_name<I, y_axis_t>()));


		});


		os << "\\end{tikzpicture}";
	}


	template<typename TestT>
	test_run_result_t<TestT> execute_test(test_run<TestT> run, bool verbose = true) {

		using x_axis_t = typename TestT::x_axis;
		using y_axis_t = typename TestT::y_axis;

		std::vector<std::pair<x_axis_t, y_axis_t>> results;
		std::mutex results_mtx;

		BS::thread_pool pool;

		int i = 0;
		std::atomic_int tasks_finished = 0;
		for(const auto& x_gen: run.x_axis) {
			x_axis_t x(x_gen);
			std::ignore = pool.submit_task([i, x, &tasks_finished, &run, &results, &results_mtx] {
				{
					std::scoped_lock lk {results_mtx};
					if(results.size() <= i) {
						results.resize(i + 1);
					}
				}

				std::array<average<double>, reflect::size<y_axis_t>()> averages;
				for(int smp=0; smp<run.samples_per_point; ++smp) {
					auto y = run.test_instance.eval(x, run.rng);
					reflect::for_each([&](auto I) {
						averages[I].add(reflect::get<I>(y));
					}, y);
				}
				std::array<double, reflect::size<y_axis_t>()> final_values;
				for(int k=0; k<final_values.size(); k++) {
					final_values[k] = averages[k].get();
				}
				auto y_final = std::make_from_tuple<y_axis_t>(final_values);
				results[i] = {x, y_final};
				++tasks_finished;
			});
			i++;
		}

		if(verbose) {
			size_t bksp = 0;
			while(pool.get_tasks_total()) {
				std::cout << std::string(bksp, '\b');
				std::string tasks_finished_msg = std::format("running test \"{}\" ({} / {})", run.short_title, int(tasks_finished), i);
				std::cout << tasks_finished_msg;
				std::cout.flush();
				bksp = tasks_finished_msg.size();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		std::cout << "\n";

		pool.wait();

		if(run.save_to_csv) {
			std::string filename = run.short_title + ".csv";
			render_to_csv(std::ofstream(filename), run, results);
			if(verbose) {
				std::println("saving to {}", filename);
			}
		}
		if(run.save_to_pgfplots) {
			std::string filename = run.short_title + ".tex";
			render_to_pgfplots(std::ofstream(filename), run, results);
			if(verbose) {
				std::println("saving to {}", filename);
			}
		}

		return results;
	}


}


#endif //GRAPH2X_LAB_HPP
