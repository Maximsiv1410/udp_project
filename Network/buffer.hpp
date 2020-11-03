#pragma once
#include "net_essential.hpp"

namespace net {


#define MAX_SIZE 65535

	class buffer {
		std::array<char, MAX_SIZE> buffer_;
		std::size_t size_ = 0;

	public:
		buffer() = default;

		char * data() {
			return buffer_.data();
		}

		std::size_t  max_size() {
			return buffer_.size();
		}

		void set_size(std::size_t newsize) {
			size_ = newsize;
		}

		std::size_t size() {
			return size_;
		}

		void assign(const char * source, std::size_t size) {
			if (size_ != 0) {
				std::memset(buffer_.data(), 0, size_);
			}

			std::memcpy(buffer_.data(), source, size);
			size_ = size;
		}

		char & operator[](std::size_t index) {
			return buffer_[index];
		}

		void zero() {
			std::memset(buffer_.data(), 0, size_);
			size_ = 0;
		}
	};
}

