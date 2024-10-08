
#ifndef GRAPH2X_UTIL_HPP
#define GRAPH2X_UTIL_HPP

#include <format>
#include <utility>

namespace g2x {


	namespace detail {


		template<typename TNextFn>
		auto rust_like_range(TNextFn&& next_fn) {
			return
				std::views::iota(isize(0))
				| std::views::transform(
					[
						next = std::forward<TNextFn>(next_fn),
						val = decltype(next_fn()){},
						last_i = isize(-1)
					]
					([[maybe_unused]] isize i) mutable {
						if(i != last_i) {
							val = next();
							last_i = i;
						}
						return val;
					})
				| std::views::take_while([]<typename T>(const std::optional<T>& opt) {
					return opt.has_value();
				})
				| std::views::transform([]<typename T>(const std::optional<T>& opt) -> T {
					return *opt;
				});
		}

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
			data_ = decltype(data_)(width * height, value);
		}

		[[nodiscard]] idx_t width() const {
			return width_;
		}

		[[nodiscard]] idx_t height() const {
			return height_;
		}

		decltype(auto) operator[](this auto&& self, idx_t x, idx_t y) {
#ifdef GRAPH2X_DEBUG
			bounds_check(x, y);
#endif
			return self.access(x, y);
		}

		decltype(auto) operator[](this auto&& self, const std::pair<idx_t, idx_t>& idx) {
#ifdef GRAPH2X_DEBUG
			bounds_check(x, y);
#endif
			return self.access(idx.first, idx.second);
		}

		decltype(auto) at(this auto&& self, idx_t x, idx_t y) {
			self.bounds_check(x, y);
			return self.access(x, y);
		}

		auto begin(this auto&& self) {
			return self.data_.begin();
		}

		auto end(this auto&& self) {
			return self.data_.end();
		}

		auto data(this auto&& self) {
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

		decltype(auto) access(this auto&& self, idx_t x, idx_t y) {
			auto offset = y * self.width() + x;
			return self.data_[offset];
		}

		void bounds_check(idx_t x, idx_t y) const {
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
