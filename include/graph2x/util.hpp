
#ifndef GRAPH2X_UTIL_HPP
#define GRAPH2X_UTIL_HPP

#include <utility>

namespace g2x {


	namespace detail {

		template<typename TNextFn>
		class rust_like_range {
		private:
			using next_fn_type = std::remove_cvref_t<TNextFn>;
			using value_type_opt = std::invoke_result_t<next_fn_type>;
			using value_type = typename value_type_opt::value_type;

			next_fn_type fn_;
		public:

			explicit rust_like_range(TNextFn&& fn) : fn_(std::forward<TNextFn>(fn)) {

			}

			class sentinel {};
			class iterator {
			public:
				next_fn_type& fn_;
				value_type_opt val_;

				bool operator==(const sentinel&) const {
					return not val_.has_value();
				}

				void operator++() {
					val_ = fn_();
				}

				value_type& operator*() {
					return *val_;
				}

				const value_type& operator*() const {
					return *val_;
				}
			};


			iterator begin() {
				auto result = iterator {.fn_ = fn_};
				++result;
				return result;
			}

			sentinel end() {
				return sentinel{};
			}
		};

		struct always_true {
			template<typename... Ts>
			bool operator()(Ts&&...) {
				return true;
			}
		};

		struct always_false {
			template<typename... Ts>
			bool operator()(Ts&&...) {
				return true;
			}
		};
	}

}


#endif //GRAPH2X_UTIL_HPP
