#pragma once

#include "net_essential.hpp"

namespace net {

	template <typename T>
	class tsqueue {
		std::queue<T> coll;
		std::mutex guard;
	public:

		template<typename D, typename = typename std::enable_if<std::is_same<typename std::decay<D>::type, T >::value >::type >
		void push(D && elem) {
			std::unique_lock<std::mutex> ulock(guard);
			coll.push(std::forward<D>(elem));
		}

		void pop(T & out) {
			std::unique_lock<std::mutex> ulock(guard);
			out = std::move(coll.front());
			coll.pop();
		}

		T pop() {
			std::unique_lock<std::mutex> ulock(guard);
			auto ret = std::move(coll.front());
			coll.pop();
			return ret;
		}
		

		std::size_t size() {
			std::unique_lock<std::mutex> ulock(guard);
			return coll.size();
		}

		bool empty() {
			std::unique_lock<std::mutex> ulock(guard);
			return coll.empty();
		}

		void clear() {
			std::unique_lock<std::mutex> ulock(guard);
			coll.clear();
		}

		std::shared_ptr<T> try_pop() {
			std::unique_lock<std::mutex> ulock(guard);
			if (!coll.empty()) {
				T item = std::move(coll.front());
				auto ptr = std::make_shared<T>(std::move(item));
				coll.pop();
				return ptr;
			}
			else {
				return std::shared_ptr<T>(nullptr);
			}
		}

	};

}