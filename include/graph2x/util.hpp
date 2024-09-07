
#ifndef GRAPH2X_UTIL_HPP
#define GRAPH2X_UTIL_HPP

#include <format>
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

	template<typename T>
	class array_2d {
	public:
		using idx_t = std::ptrdiff_t;

		array_2d() = default;

		array_2d(idx_t width, idx_t height): width_(width), height_(height) {
			data_.resize(width * height);
		}

		array_2d(idx_t width, idx_t height, T value) : array_2d(width, height) {
			std::fill(data_.begin(), data_.end(), value);
		}

		[[nodiscard]] idx_t width() const {
			return width_;
		}

		[[nodiscard]] idx_t height() const {
			return height_;
		}

		auto&& operator[](this auto&& self, idx_t x, idx_t y) {
#ifdef GRAPH2X_DEBUG
			bounds_check(x, y);
#endif
			return self.access(x, y);
		}

		auto&& operator[](this auto&& self, const std::pair<idx_t, idx_t>& idx) {
#ifdef GRAPH2X_DEBUG
			bounds_check(x, y);
#endif
			return self.access(idx.first, idx.second);
		}

		auto&& at(this auto&& self, idx_t x, idx_t y) {
			bounds_check(x, y);
			return self.access(x, y);
		}

		auto&& begin(this auto&& self) {
			return self.data_.begin();
		}

		auto&& end(this auto&& self) {
			return self.data_.end();
		}

		auto&& data(this auto&& self) {
			return self.data_.data();
		}

		template<typename FnT>
		void for_each_indexed(FnT&& fn) const {
			for(idx_t y=0; y<height_; ++y) {
				for(idx_t x=0; x<width_; ++x) {
					fn(x, y, (*this)[x, y]);
				}
			}
		}

		[[nodiscard]] std::pair<idx_t, idx_t> offset_to_coord(idx_t offset) const {
			return {offset % width(), offset / width()};
		}

		[[nodiscard]] idx_t coord_to_offset(idx_t x, idx_t y) const {
			return y * width() + x;
		}

		using value_type = T;
	private:
		int width_ = 0, height_ = 0;
		std::vector<T> data_;

		auto&& access(this auto&& self, idx_t x, idx_t y) {
			return self.data_[y * self.width() + x];
		}

		void bounds_check(idx_t x, idx_t y) {
			if(x < 0 || x >= width() || y < 0 || y >= height()) {
				throw std::out_of_range(std::format(
					"element ({},{}) is out of bounds of this {}x{} 2d array",
					x, y, width(), height()
				));
			}
		}
	};

}


#endif //GRAPH2X_UTIL_HPP
