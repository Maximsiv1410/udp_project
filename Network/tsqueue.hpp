#pragma once

#include "net_essential.hpp"

namespace net {

	template <typename T>
	class tsqueue {
		std::queue<T> coll;
		mutable std::mutex guard;

		std::condition_variable cvar;
	public:

		template<typename D, typename = typename std::enable_if<std::is_same<typename std::decay<D>::type, T >::value >::type >
		void push(D && elem) {
			{
				std::unique_lock<std::mutex> ulock(guard);
				coll.push(std::forward<D>(elem));		
			}
			cvar.notify_one();
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
		
		bool try_pop(T& item) {
			std::lock_guard<std::mutex> ulock(guard);
			if (coll.empty()) 
				return false;
			item = std::move(coll.front());
			coll.pop();
			return true;
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

		void wait_and_pop(T& item) {
			std::unique_lock<std::mutex> waiter(guard);
			cvar.wait(waiter, [this]() { return !coll.empty(); });

			item = std::move(coll.front());
			coll.pop();
		}

		std::shared_ptr<T> wait_and_pop() {
			std::unique_lock<std::mutex> ulock(guard);
			cvar.wait(ulock, [this]{ return !coll.empty(); });

			auto result = std::make_shared<T>(std::move(coll.front()));
			coll.pop();
			return result;
		}


		std::size_t size() {
			std::unique_lock<std::mutex> ulock(guard);
			return coll.size();
		}

		bool empty() const {
			std::unique_lock<std::mutex> ulock(guard);
			return coll.empty();
		}

		void clear() {
			std::unique_lock<std::mutex> ulock(guard);
			coll.clear();
		}

		void wait() {
			std::unique_lock<std::mutex> waiter(guard);
			cvar.wait(waiter, [this]() { return !coll.empty(); });
		}	

		

	};

}