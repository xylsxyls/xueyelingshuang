#include "LockFreeQueue.h"

//#include "ConsoleTest.h"
//#include <stdint.h>
//#include <stdio.h>
//#ifdef __unix__
//#include <signal.h>
//#include <stdlib.h>
//#endif
//#include <iostream>
//#include <thread>
//#include <vector>
//#include <algorithm>
//#include <fstream>
//#include <atomic>
//#include <chrono>
//#include <random>
//#include <condition_variable>
//#include <mutex>
//#include "LockFreeQueue/LockFreeQueueAPI.h"
//
//#ifdef _MSC_VER
//BOOL CALLBACK ConsoleHandler(DWORD eve) { return FALSE; }
//int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);
//#elif __unix__
//void CtrlCMessage(int eve) { if (eve == 2) exit(0); }
//struct CtrlC { CtrlC() { struct sigaction sigIntHandler; sigIntHandler.sa_handler = CtrlCMessage; sigemptyset(&sigIntHandler.sa_mask); sigIntHandler.sa_flags = 0; sigaction(SIGINT, &sigIntHandler, nullptr); } };
//CtrlC g_ctrlc;
//#endif
//
////#define DEBUG_PUSH_POP
//
//std::atomic<int> global_counter(-1);
//
//void reset_counter() { global_counter.store(-1); }
//
//class Semaphore {
//public:
//	Semaphore(int count = 0) : m_count(count) {}
//
//	void signal() {
//		std::unique_lock<std::mutex> lock(m_mtx);
//		++m_count;
//		m_cv.notify_one();
//	}
//
//	void wait() {
//		std::unique_lock<std::mutex> lock(m_mtx);
//		while (m_count == 0) {
//			m_cv.wait(lock);
//		}
//		--m_count;
//	}
//
//private:
//	std::mutex m_mtx;
//	std::condition_variable m_cv;
//	int m_count;
//};
//
//
//// 辅助函数：输出当前监控计数
//void print_monitor_count(const std::string& tag) {
//#ifdef DEBUG_PUSH_POP
//	std::cout << "[Monitor][" << tag << "] push_success = " << LockFreeQueue<int>::g_push_success.load()
//		<< ", pop_success = " << LockFreeQueue<int>::g_pop_success.load() << std::endl;
//#endif
//}
//
//// 重置监控计数
//void reset_monitor_count() {
//#ifdef DEBUG_PUSH_POP
//	LockFreeQueue<int>::g_push_success = 0;
//	LockFreeQueue<int>::g_pop_success = 0;
//#endif
//}
//
//// ========== 带进度输出的 pop 函数 ==========
//void single_thread_pop(LockFreeQueue<int>& queue, int num_elements, std::vector<int>& output) {
//	output.reserve(num_elements);
//	std::cout << "[single_thread_pop] Starting to pop " << num_elements << " elements..." << std::endl;
//	auto start = std::chrono::steady_clock::now();
//	for (int i = 0; i < num_elements; ++i) {
//		int value;
//		while (!queue.pop(&value)) {
//			std::this_thread::yield();
//		}
//		output.push_back(value);
//		if ((i + 1) % 100000 == 0) {
//			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//			std::cout << "[single_thread_pop] Progress: " << (i + 1) << "/" << num_elements
//#ifdef DEBUG_PUSH_POP
//				<< " (pop_success=" << LockFreeQueue<int>::g_pop_success.load()
//#endif
//				<< ", elapsed " << elapsed << " s)" << std::endl;
//		}
//	}
//	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//	std::cout << "[single_thread_pop] Finished popping " << num_elements << " elements in " << elapsed << " s" << std::endl;
//}
//
//// ========== 丢弃 pop 函数 ==========
//void single_thread_discard_pop(LockFreeQueue<int>& queue, int num_elements) {
//	std::cout << "[single_thread_discard_pop] Starting to discard-pop "
//		<< num_elements << " elements..." << std::endl;
//
//	auto start = std::chrono::steady_clock::now();
//
//	for (int i = 0; i < num_elements; ++i) {
//		while (!queue.pop(nullptr)) {
//			std::this_thread::yield();
//		}
//
//		if ((i + 1) % 100000 == 0) {
//			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
//				std::chrono::steady_clock::now() - start).count();
//
//			std::cout << "[single_thread_discard_pop] Progress: "
//				<< (i + 1) << "/" << num_elements
//				<< ", elapsed " << elapsed << " s" << std::endl;
//		}
//	}
//
//	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
//		std::chrono::steady_clock::now() - start).count();
//
//	std::cout << "[single_thread_discard_pop] Finished discard-popping "
//		<< num_elements << " elements in " << elapsed << " s" << std::endl;
//}
//
//// ========== 带进度输出的 push 函数（多线程，不在这里打印，而是通过计数器） ==========
//void push_elements(LockFreeQueue<int>& queue, int num_elements) {
//	for (int i = 0; i < num_elements; ++i) {
//		queue.push(++global_counter);
//	}
//	// 不在这里打印，避免大量输出
//}
//
//// 批量 push（无额外输出）
//void perform_push_operations(LockFreeQueue<int>& queue, int num_threads, int num_elements_per_thread) {
//	std::vector<std::thread> threads;
//	for (int i = 0; i < num_threads; ++i) {
//		threads.emplace_back(push_elements, std::ref(queue), num_elements_per_thread);
//	}
//	for (auto& t : threads) t.join();
//}
//
//// ========== 带详细输出的 concurrent_push_pop ==========
//void concurrent_push_pop(LockFreeQueue<int>& queue, int num_push_threads, int num_elements_per_thread, std::vector<int>& popped_values) {
//	int total = num_push_threads * num_elements_per_thread;
//
//	std::cout << "[concurrent_push_pop] Starting " << num_push_threads << " push threads..." << std::endl;
//	auto start_time = std::chrono::steady_clock::now();
//	std::vector<std::thread> push_threads;
//	push_threads.reserve(num_push_threads);
//
//	for (int i = 0; i < num_push_threads; ++i) {
//		push_threads.emplace_back(push_elements, std::ref(queue), num_elements_per_thread);
//	}
//
//	std::cout << "[concurrent_push_pop] All push threads started, creating pop thread..." << std::endl;
//	std::thread pop_thread(single_thread_pop, std::ref(queue), total, std::ref(popped_values));
//
//	std::cout << "[concurrent_push_pop] Waiting for push threads to finish..." << std::endl;
//	for (auto& t : push_threads) {
//		t.join();
//	}
//	auto push_end = std::chrono::steady_clock::now();
//	auto push_elapsed = std::chrono::duration_cast<std::chrono::seconds>(push_end - start_time).count();
//	std::cout << "[concurrent_push_pop] All push threads finished in " << push_elapsed << " s. Queue size = " << queue.size() << std::endl;
//
//	std::cout << "[concurrent_push_pop] Waiting for pop thread to finish..." << std::endl;
//	pop_thread.join();
//	auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count();
//	std::cout << "[concurrent_push_pop] Pop thread finished. Total time = " << total_elapsed << " s. Popped size = " << popped_values.size() << std::endl;
//
//	print_monitor_count("concurrent_push_pop end");
//}
//
//// ========== 验证函数 ==========
//bool validate_queue_size(LockFreeQueue<int>& queue, const std::string& message, int expected_size) {
//	int actual_size = queue.size();
//	bool result = (actual_size == expected_size);
//	std::cout << (result ? "passed" : "failed") << ": " << message << ": actual size = " << actual_size << ", expected size = " << expected_size << std::endl;
//	return result;
//}
//
//bool validate_popped_values(const std::vector<int>& values, int expected_size) {
//	if (values.size() != static_cast<size_t>(expected_size)) {
//		std::cout << "failed: Popped values size mismatch: expected " << expected_size << ", got " << values.size() << std::endl;
//		return false;
//	}
//	std::vector<int> sorted = values;
//	std::sort(sorted.begin(), sorted.end());
//	for (int i = 0; i < expected_size; ++i) {
//		if (sorted[i] != i) {
//			std::cout << "failed: Value mismatch at index " << i << ": expected " << i << ", got " << sorted[i] << std::endl;
//			return false;
//		}
//	}
//	std::cout << "passed: All popped values are correct." << std::endl;
//	return true;
//}
//
//// ========== Test 1 ==========
//bool test1(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 1: Parallel push, then single-thread pop ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::cout << "[test1] Starting " << num_threads << " push threads..." << std::endl;
//	auto start = std::chrono::steady_clock::now();
//	perform_push_operations(queue, num_threads, num_per_thread);
//	auto push_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//	std::cout << "[test1] All push threads finished in " << push_elapsed << " s" << std::endl;
//
//	if (!validate_queue_size(queue, "Queue size after push", total)) return false;
//
//	std::vector<int> popped;
//	single_thread_pop(queue, total, popped);
//	if (!validate_queue_size(queue, "Queue size after pop", 0)) return false;
//	if (!validate_popped_values(popped, total)) return false;
//	print_monitor_count("test1 end");
//	return true;
//}
//
//// ========== Test 2 ==========
//bool test2(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 2: Concurrent push and pop (simultaneous) ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//	std::vector<int> popped;
//
//	concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//	if (!validate_queue_size(queue, "Queue size after concurrent", 0)) return false;
//	if (!validate_popped_values(popped, total)) return false;
//	print_monitor_count("test2 end");
//	return true;
//}
//
//// ========== Test 3A ==========
//bool test3A(int num_threads, int num_per_thread, int rounds) {
//	std::cout << "\n=== Test 3A: Multiple rounds (" << rounds << " rounds) ===" << std::endl;
//	for (int round = 0; round < rounds; ++round) {
//		std::cout << "--- Round " << round << " starting ---" << std::endl;
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		int total = num_threads * num_per_thread;
//		reset_counter();
//		std::vector<int> popped;
//
//		concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << "failed: Round " << round << " size mismatch (queue size=" << queue.size() << ", popped=" << popped.size() << ", total=" << total << ")" << std::endl;
//			return false;
//		}
//		if (!validate_popped_values(popped, total)) {
//			std::cout << "failed: Round " << round << " value mismatch" << std::endl;
//			return false;
//		}
//		if ((round + 1) % 5 == 0) {
//			std::cout << "  Completed round " << (round + 1) << " of " << rounds << std::endl;
//		}
//		print_monitor_count("test3A round " + std::to_string(round));
//	}
//	std::cout << "passed: All " << rounds << " rounds completed successfully" << std::endl;
//	return true;
//}
//
//// ========== Test 3B ==========
//bool test3B(int num_threads, int num_per_thread, int rounds) {
//	std::cout << "\n=== Test 3B: Multiple rounds with delay (" << rounds << " rounds) ===" << std::endl;
//	for (int round = 0; round < rounds; ++round) {
//		std::cout << "--- Round " << round << " starting ---" << std::endl;
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		int total = num_threads * num_per_thread;
//		reset_counter();
//		std::vector<int> popped;
//
//		if (round > 0) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		}
//
//		concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << "failed: Round " << round << " size mismatch (queue size=" << queue.size() << ", popped=" << popped.size() << ", total=" << total << ")" << std::endl;
//			return false;
//		}
//		if (!validate_popped_values(popped, total)) {
//			std::cout << "failed: Round " << round << " value mismatch" << std::endl;
//			return false;
//		}
//		if ((round + 1) % 5 == 0) {
//			std::cout << "  Completed round " << (round + 1) << " of " << rounds << std::endl;
//		}
//		print_monitor_count("test3B round " + std::to_string(round));
//	}
//	std::cout << "passed: All " << rounds << " rounds completed successfully" << std::endl;
//	return true;
//}
//
//// ========== Test 4 ==========
//bool test4() {
//	std::cout << "\n=== Test 4: Different push thread counts ===" << std::endl;
//
//	const int target_total = 20000000;
//	std::vector<int> thread_counts = { 50, 100, 200, 400 };
//
//	for (int num_threads : thread_counts) {
//		int per_thread = target_total / num_threads;
//		int total = num_threads * per_thread;
//
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		reset_counter();
//
//		std::cout << "  Testing with " << num_threads
//			<< " threads, " << per_thread
//			<< " each (total " << total << ")..." << std::endl;
//
//		std::vector<int> popped;
//
//		auto start = std::chrono::steady_clock::now();
//		concurrent_push_pop(queue, num_threads, per_thread, popped);
//		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
//			std::chrono::steady_clock::now() - start).count();
//
//		std::cout << "  Test completed in " << elapsed << " s" << std::endl;
//
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << " FAILED: queue.size=" << queue.size()
//				<< ", popped=" << popped.size()
//				<< ", expected=" << total << std::endl;
//			return false;
//		}
//
//		if (!validate_popped_values(popped, total)) {
//			return false;
//		}
//
//		std::cout << " PASSED" << std::endl;
//		print_monitor_count("test4 threads=" + std::to_string(num_threads));
//	}
//
//	return true;
//}
//
//// ========== Test 5 ==========
//bool test5(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 5: High pressure test (recreate queue each time) ===" << std::endl;
//	for (int run = 0; run < 3; ++run) {
//		std::cout << "  Run " << run + 1 << " of 3" << std::endl;
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		int total = num_threads * num_per_thread;
//		reset_counter();
//		std::vector<int> popped;
//
//		concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << "failed: Run " << run << " size mismatch" << std::endl;
//			return false;
//		}
//		std::vector<int> sorted = popped;
//		std::sort(sorted.begin(), sorted.end());
//		bool valid = true;
//		for (int i = 0; i < total; ++i) {
//			if (sorted[i] != i) { valid = false; break; }
//		}
//		if (!valid) {
//			std::cout << "failed: Run " << run << " value mismatch" << std::endl;
//			return false;
//		}
//		std::cout << "  Run " << (run + 1) << " passed" << std::endl;
//		print_monitor_count("test5 run " + std::to_string(run));
//	}
//	std::cout << "passed: All pressure test runs passed" << std::endl;
//	return true;
//}
//
//// ========== Test 6 ==========
//bool test6(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 6: Push all, then single clear ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::cout << "[test6] Pushing " << total << " elements..." << std::endl;
//	auto start = std::chrono::steady_clock::now();
//	perform_push_operations(queue, num_threads, num_per_thread);
//	auto push_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//	std::cout << "[test6] Push completed in " << push_elapsed << " s" << std::endl;
//
//	if (!validate_queue_size(queue, "Queue size before clear", total)) return false;
//
//	std::cout << "[test6] Clearing queue..." << std::endl;
//	queue.clear();
//	if (!validate_queue_size(queue, "Queue size after clear", 0)) return false;
//
//	std::cout << "[test6] Repushing and popping to verify..." << std::endl;
//	reset_counter();
//	std::vector<int> popped;
//	concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//	if (!validate_queue_size(queue, "Queue size after clear and repush", 0)) return false;
//	if (!validate_popped_values(popped, total)) return false;
//
//	print_monitor_count("test6 end");
//	std::cout << "passed: Clear test passed" << std::endl;
//	return true;
//}
//
//// ========== Test 7 ==========
//bool test7(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 7: Frequent clear during push ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::atomic<bool> push_done{ false };
//	std::atomic<int> clear_count{ 0 };
//	std::atomic<int> actual_pushed{ 0 };
//
//	// 启动 push 线程
//	std::vector<std::thread> push_threads;
//	for (int i = 0; i < num_threads; ++i) {
//		push_threads.emplace_back([&]() {
//			for (int j = 0; j < num_per_thread; ++j) {
//				queue.push(++global_counter);
//				actual_pushed++;
//			}
//		});
//	}
//
//	// 单线程频繁 clear
//	std::thread clear_thread([&]() {
//		while (!push_done.load()) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(1));
//			queue.clear();
//			clear_count++;
//			if (clear_count.load() % 100 == 0) {
//				std::cout << "  Clear count: " << clear_count.load()
//					<< ", queue size: " << queue.size() << std::endl;
//				print_monitor_count("test7 clear");
//			}
//		}
//		queue.clear();
//		std::cout << "Final clear performed, total clears: " << clear_count.load() << std::endl;
//	});
//
//	// 等待所有 push 完成
//	for (auto& t : push_threads) t.join();
//	push_done = true;
//	clear_thread.join();
//
//	if (!validate_queue_size(queue, "Queue size after final clear", 0)) return false;
//
//	std::cout << "Total pushes attempted: " << actual_pushed.load() << std::endl;
//	std::cout << "Total clears performed: " << clear_count.load() << std::endl;
//	print_monitor_count("test7 end");
//	std::cout << "passed: Frequent clear test passed (no crash)" << std::endl;
//	return true;
//}
//
//// ========== Test 8 ==========
//bool test8(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 8: Clear, then repush multiple times ===" << std::endl;
//	const int cycles = 5;
//	int total = num_threads * num_per_thread;
//
//	for (int cycle = 0; cycle < cycles; ++cycle) {
//		std::cout << "  Cycle " << (cycle + 1) << "/" << cycles << ": ";
//		reset_counter();
//		LockFreeQueue<int> queue;
//
//		perform_push_operations(queue, num_threads, num_per_thread);
//		int size_before_clear = queue.size();
//		if (size_before_clear != total) {
//			std::cout << "FAILED (size before clear: " << size_before_clear << ")" << std::endl;
//			return false;
//		}
//
//		queue.clear();
//		int size_after_clear = queue.size();
//		if (size_after_clear != 0) {
//			std::cout << "FAILED (size after clear: " << size_after_clear << ")" << std::endl;
//			return false;
//		}
//		std::cout << "OK" << std::endl;
//		print_monitor_count("test8 cycle " + std::to_string(cycle));
//	}
//	std::cout << "passed: Multiple clear-repush cycles passed" << std::endl;
//	return true;
//}
//
//// ========== Test 9 ==========
//bool test9(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 9: Multi-thread clear requests, pop thread executes clear ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::atomic<bool> push_finished{ false };
//	std::atomic<int> clear_request_count{ 0 };
//	std::atomic<int> clear_executed_count{ 0 };
//	std::atomic<int> push_count{ 0 };
//	std::atomic<int> pop_count{ 0 };
//
//	// 多线程 push
//	std::vector<std::thread> push_threads;
//	for (int i = 0; i < num_threads; ++i) {
//		push_threads.emplace_back([&]() {
//			for (int j = 0; j < num_per_thread; ++j) {
//				queue.push(++global_counter);
//				push_count++;
//			}
//		});
//	}
//
//	// 多线程 clear 请求：这里只发请求，不直接 clear
//	const int clear_thread_count = 10;
//	std::vector<std::thread> clear_request_threads;
//	for (int i = 0; i < clear_thread_count; ++i) {
//		clear_request_threads.emplace_back([&, i]() {
//			while (!push_finished.load()) {
//				std::this_thread::sleep_for(std::chrono::milliseconds(5 + (i % 10)));
//				clear_request_count++;
//
//				int req = clear_request_count.load();
//				if (req % 100 == 0) {
//					std::cout << "  Clear requests: " << req
//						<< ", executed: " << clear_executed_count.load() << std::endl;
//				}
//			}
//		});
//	}
//
//	// 单线程 pop + 执行 clear
//	std::thread pop_thread([&]() {
//		int value;
//
//		while (true) {
//			if (queue.pop(&value)) {
//				pop_count++;
//			}
//
//			if (clear_request_count.load() > clear_executed_count.load()) {
//				queue.clear();
//				clear_executed_count++;
//
//				if (clear_executed_count.load() % 50 == 0) {
//					std::cout << "  Clear executed #" << clear_executed_count.load()
//						<< ", queue size: " << queue.size()
//						<< ", requests: " << clear_request_count.load()
//						<< ", popped: " << pop_count.load() << std::endl;
//				}
//			}
//
//			if (push_finished.load() && push_count.load() >= total) {
//				while (clear_request_count.load() > clear_executed_count.load()) {
//					queue.clear();
//					clear_executed_count++;
//				}
//				break;
//			}
//
//			std::this_thread::yield();
//		}
//
//		while (queue.pop(&value)) {
//			pop_count++;
//		}
//
//		queue.clear();
//		clear_executed_count++;
//
//		std::cout << "  Pop thread finished." << std::endl;
//		std::cout << "  Total clears executed: " << clear_executed_count.load() << std::endl;
//		std::cout << "  Total pops: " << pop_count.load() << std::endl;
//	});
//
//	for (auto& t : push_threads) {
//		t.join();
//	}
//
//	push_finished = true;
//	std::cout << "All push threads finished. Total pushes: "
//		<< push_count.load() << std::endl;
//
//	for (auto& t : clear_request_threads) {
//		t.join();
//	}
//
//	std::cout << "All clear request threads finished. Total requests: "
//		<< clear_request_count.load() << std::endl;
//
//	pop_thread.join();
//
//	queue.clear();
//	if (!validate_queue_size(queue, "Queue size after final clear", 0)) {
//		return false;
//	}
//
//	reset_counter();
//	std::vector<int> popped;
//	concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//	if (!validate_queue_size(queue, "Queue size after clear and repush", 0)) {
//		return false;
//	}
//
//	if (!validate_popped_values(popped, total)) {
//		return false;
//	}
//
//	print_monitor_count("test9 end");
//	std::cout << "passed: Multi-thread clear requests test passed" << std::endl;
//	return true;
//}
//
//bool test10(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 10: Concurrent push and direct pop(nullptr) discard ===" << std::endl;
//	reset_monitor_count();
//
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::cout << "[test10] Starting " << num_threads
//		<< " push threads and one discard-pop thread..." << std::endl;
//
//	auto start_time = std::chrono::steady_clock::now();
//
//	std::vector<std::thread> push_threads;
//	push_threads.reserve(num_threads);
//
//	for (int i = 0; i < num_threads; ++i) {
//		push_threads.emplace_back(push_elements, std::ref(queue), num_per_thread);
//	}
//
//	std::thread discard_thread(single_thread_discard_pop, std::ref(queue), total);
//
//	for (auto& t : push_threads) {
//		t.join();
//	}
//
//	std::cout << "[test10] All push threads finished. Queue size = "
//		<< queue.size() << std::endl;
//
//	discard_thread.join();
//
//	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
//		std::chrono::steady_clock::now() - start_time).count();
//
//	std::cout << "[test10] Discard-pop finished. Total time = "
//		<< elapsed << " s" << std::endl;
//
//	if (!validate_queue_size(queue, "Queue size after direct pop(nullptr)", 0)) {
//		return false;
//	}
//
//	print_monitor_count("test10 end");
//	std::cout << "passed: Direct pop(nullptr) discard test passed" << std::endl;
//	return true;
//}
//
//bool test11(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 11: Explicit destroy/init reuse ===" << std::endl;
//	reset_monitor_count();
//
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::cout << "[test11] First push " << total << " elements..." << std::endl;
//	perform_push_operations(queue, num_threads, num_per_thread);
//
//	if (!validate_queue_size(queue, "Queue size before explicit destroy", total)) {
//		return false;
//	}
//
//	std::cout << "[test11] Explicit destroy..." << std::endl;
//	queue.destroy();
//
//	if (!validate_queue_size(queue, "Queue size after explicit destroy", 0)) {
//		return false;
//	}
//
//	std::cout << "[test11] Explicit init after destroy..." << std::endl;
//	queue.init();
//
//	if (!validate_queue_size(queue, "Queue size after re-init", 0)) {
//		return false;
//	}
//
//	std::cout << "[test11] Repush and pop after destroy/init..." << std::endl;
//	reset_counter();
//
//	std::vector<int> popped;
//	concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//	if (!validate_queue_size(queue, "Queue size after destroy/init repush", 0)) {
//		return false;
//	}
//
//	if (!validate_popped_values(popped, total)) {
//		return false;
//	}
//
//	print_monitor_count("test11 end");
//	std::cout << "passed: Explicit destroy/init reuse test passed" << std::endl;
//	return true;
//}
//
//bool test12() {
//	std::cout << "\n=== Test 12: Empty queue boundary operations ===" << std::endl;
//	reset_monitor_count();
//
//	LockFreeQueue<int> queue;
//
//	int value = -1;
//
//	if (queue.pop(&value)) {
//		std::cout << "failed: pop(&value) on empty queue returned true" << std::endl;
//		return false;
//	}
//
//	if (queue.pop(nullptr)) {
//		std::cout << "failed: pop(nullptr) on empty queue returned true" << std::endl;
//		return false;
//	}
//
//	queue.clear();
//
//	if (!validate_queue_size(queue, "Queue size after empty clear", 0)) {
//		return false;
//	}
//
//	queue.destroy();
//
//	if (!validate_queue_size(queue, "Queue size after empty destroy", 0)) {
//		return false;
//	}
//
//	queue.destroy();
//
//	if (!validate_queue_size(queue, "Queue size after repeated destroy", 0)) {
//		return false;
//	}
//
//	queue.init();
//
//	if (!validate_queue_size(queue, "Queue size after init from destroyed state", 0)) {
//		return false;
//	}
//
//	queue.push(123);
//
//	if (!queue.pop(&value)) {
//		std::cout << "failed: pop after init/push returned false" << std::endl;
//		return false;
//	}
//
//	if (value != 123) {
//		std::cout << "failed: value mismatch after init/push/pop, got "
//			<< value << ", expected 123" << std::endl;
//		return false;
//	}
//
//	if (!validate_queue_size(queue, "Queue size after final pop", 0)) {
//		return false;
//	}
//
//	print_monitor_count("test12 end");
//	std::cout << "passed: Empty queue boundary operations passed" << std::endl;
//	return true;
//}
//
//// ========== main ==========
//#include "CDump/CDumpAPI.h"
//
//int main() {
//	CDump::declareDumpFile();
//
//#ifdef DEBUG_PUSH_POP
//	// 监控线程
//	std::atomic<bool> monitoring{ true };
//	auto monitor = std::thread([&]() {
//		while (monitoring) {
//			std::this_thread::sleep_for(std::chrono::seconds(10)); // 每10秒输出一次
//			std::cout << "[Background Monitor] push_success = "
//				<< LockFreeQueue<int>::g_push_success.load()
//				<< ", pop_success = "
//				<< LockFreeQueue<int>::g_pop_success.load()
//				<< std::endl;
//		}
//	});
//#endif
//
//	const int num_threads = 200;
//	const int num_per_thread = 100000;
//	const int repeat = 800;
//	bool all_passed = true;
//
//	for (int run = 1; run <= repeat; ++run) {
//		std::cout << "\n========== Run " << run << " of " << repeat << " ==========" << std::endl;
//		if (!test1(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test2(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test3A(num_threads, num_per_thread, 10)) { all_passed = false; break; }
//		if (!test3B(num_threads, num_per_thread, 10)) { all_passed = false; break; }
//		if (!test4()) { all_passed = false; break; }
//		if (!test5(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test6(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test7(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test8(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test9(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test10(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test11(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test12()) { all_passed = false; break; }
//	}
//
//#ifdef DEBUG_PUSH_POP
//	monitoring = false;
//	if (monitor.joinable()) monitor.join();
//#endif
//
//	if (all_passed) {
//		std::cout << "\n================== ALL TESTS PASSED ==================\n";
//	}
//	else {
//		std::cout << "\n================== SOME TESTS FAILED ==================\n";
//	}
//	return all_passed ? 0 : 1;
//}

//-------------------------old--------------------------------

//#include "ConsoleTest.h"
//#include <stdint.h>
//#include <stdio.h>
//#ifdef __unix__
//#include <signal.h>
//#include <stdlib.h>
//#endif
//#include <iostream>
//#include <thread>
//#include <vector>
//#include <algorithm>
//#include <fstream>
//#include <atomic>
//#include <chrono>
//#include <random>
//#include <condition_variable>
//#include <mutex>
//#include "LockFreeQueue/LockFreeQueueAPI.h"
//
//#ifdef _MSC_VER
//BOOL CALLBACK ConsoleHandler(DWORD eve) { return FALSE; }
//int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);
//#elif __unix__
//void CtrlCMessage(int eve) { if (eve == 2) exit(0); }
//struct CtrlC { CtrlC() { struct sigaction sigIntHandler; sigIntHandler.sa_handler = CtrlCMessage; sigemptyset(&sigIntHandler.sa_mask); sigIntHandler.sa_flags = 0; sigaction(SIGINT, &sigIntHandler, nullptr); } };
//CtrlC g_ctrlc;
//#endif
//
////#define DEBUG_PUSH_POP
//
//std::atomic<int> global_counter(-1);
//
//void reset_counter() { global_counter.store(-1); }
//
//class Semaphore {
//public:
//	Semaphore(int count = 0) : m_count(count) {}
//
//	void signal() {
//		std::unique_lock<std::mutex> lock(m_mtx);
//		++m_count;
//		m_cv.notify_one();
//	}
//
//	void wait() {
//		std::unique_lock<std::mutex> lock(m_mtx);
//		while (m_count == 0) {
//			m_cv.wait(lock);
//		}
//		--m_count;
//	}
//
//private:
//	std::mutex m_mtx;
//	std::condition_variable m_cv;
//	int m_count;
//};
//
//
//// 辅助函数：输出当前监控计数
//void print_monitor_count(const std::string& tag) {
//#ifdef DEBUG_PUSH_POP
//	std::cout << "[Monitor][" << tag << "] push_success = " << LockFreeQueue<int>::g_push_success.load()
//		<< ", pop_success = " << LockFreeQueue<int>::g_pop_success.load() << std::endl;
//#endif
//}
//
//// 重置监控计数
//void reset_monitor_count() {
//#ifdef DEBUG_PUSH_POP
//	LockFreeQueue<int>::g_push_success = 0;
//	LockFreeQueue<int>::g_pop_success = 0;
//#endif
//}
//
//// ========== 带进度输出的 pop 函数 ==========
////void single_thread_pop(LockFreeQueue<int>& queue, int num_elements, std::vector<int>& output) {
////	output.reserve(num_elements);
////	std::cout << "[single_thread_pop] Starting to pop " << num_elements << " elements..." << std::endl;
////	auto start = std::chrono::steady_clock::now();
////	for (int i = 0; i < num_elements; ++i) {
////		int value;
////		while (!queue.pop(&value)) {
////			std::this_thread::yield();
////		}
////		output.push_back(value);
////		if ((i + 1) % 100000 == 0) {
////			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
////			std::cout << "[single_thread_pop] Progress: " << (i + 1) << "/" << num_elements
////#ifdef DEBUG_PUSH_POP
////				<< " (pop_success=" << LockFreeQueue<int>::g_pop_success.load()
////#endif
////				<< ", elapsed " << elapsed << " s)" << std::endl;
////		}
////	}
////	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
////	std::cout << "[single_thread_pop] Finished popping " << num_elements << " elements in " << elapsed << " s" << std::endl;
////}
//
//void single_thread_pop(LockFreeQueue<int>& queue, int num_elements, std::vector<int>& output) {
//    output.reserve(num_elements);
//    std::cout << "[single_thread_pop] Starting to pop " << num_elements << " elements..." << std::endl;
//    auto start = std::chrono::steady_clock::now();
//    for (int i = 0; i < num_elements; ++i) {
//        int value;
//        while (!queue.pop(&value)) {
//            std::this_thread::yield();
//        }
//        output.push_back(value);
//        int remaining = num_elements - (i + 1);
//        if ((i + 1) % 100000 == 0) {
//            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//            std::cout << "[single_thread_pop] Progress: " << (i + 1) << "/" << num_elements
//                      << ", elapsed " << elapsed << " s, queue.size() = " << queue.size() << std::endl;
//        }
//        if (remaining <= 100) {
//            std::cout << "[single_thread_pop] Near end: popped " << (i + 1) << "/" << num_elements
//                      << " (value=" << value << "), remaining=" << remaining
//                      << ", queue.size()=" << queue.size() << std::endl;
//        }
//    }
//    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//    std::cout << "[single_thread_pop] Finished popping " << num_elements << " elements in " << elapsed << " s"
//              << ", final queue.size() = " << queue.size() << std::endl;
//}
//
//// ========== 带进度输出的 push 函数（多线程，不在这里打印，而是通过计数器） ==========
//void push_elements(LockFreeQueue<int>& queue, int num_elements) {
//	for (int i = 0; i < num_elements; ++i) {
//		queue.push(++global_counter);
//	}
//	// 不在这里打印，避免大量输出
//}
//
//// 批量 push（无额外输出）
//void perform_push_operations(LockFreeQueue<int>& queue, int num_threads, int num_elements_per_thread) {
//	std::vector<std::thread> threads;
//	for (int i = 0; i < num_threads; ++i) {
//		threads.emplace_back(push_elements, std::ref(queue), num_elements_per_thread);
//	}
//	for (auto& t : threads) t.join();
//}
//
//// ========== 带详细输出的 concurrent_push_pop ==========
//void concurrent_push_pop(LockFreeQueue<int>& queue, int num_push_threads, int num_elements_per_thread, std::vector<int>& popped_values) {
//	int total = num_push_threads * num_elements_per_thread;
//
//	std::cout << "[concurrent_push_pop] Starting " << num_push_threads << " push threads..." << std::endl;
//	auto start_time = std::chrono::steady_clock::now();
//	std::vector<std::thread> push_threads;
//	push_threads.reserve(num_push_threads);
//
//	for (int i = 0; i < num_push_threads; ++i) {
//		push_threads.emplace_back(push_elements, std::ref(queue), num_elements_per_thread);
//	}
//
//	std::cout << "[concurrent_push_pop] All push threads started, creating pop thread..." << std::endl;
//	std::thread pop_thread(single_thread_pop, std::ref(queue), total, std::ref(popped_values));
//
//	std::cout << "[concurrent_push_pop] Waiting for push threads to finish..." << std::endl;
//	for (auto& t : push_threads) {
//		t.join();
//	}
//	auto push_end = std::chrono::steady_clock::now();
//	auto push_elapsed = std::chrono::duration_cast<std::chrono::seconds>(push_end - start_time).count();
//	std::cout << "[concurrent_push_pop] All push threads finished in " << push_elapsed << " s. Queue size = " << queue.size() << std::endl;
//
//	std::cout << "[concurrent_push_pop] Waiting for pop thread to finish..." << std::endl;
//	pop_thread.join();
//	auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count();
//	std::cout << "[concurrent_push_pop] Pop thread finished. Total time = " << total_elapsed << " s. Popped size = " << popped_values.size() << std::endl;
//
//	print_monitor_count("concurrent_push_pop end");
//}
//
//// ========== 验证函数 ==========
//bool validate_queue_size(LockFreeQueue<int>& queue, const std::string& message, int expected_size) {
//	int actual_size = queue.size();
//	bool result = (actual_size == expected_size);
//	std::cout << (result ? "passed" : "failed") << ": " << message << ": actual size = " << actual_size << ", expected size = " << expected_size << std::endl;
//	return result;
//}
//
//bool validate_popped_values(const std::vector<int>& values, int expected_size) {
//	if (values.size() != static_cast<size_t>(expected_size)) {
//		std::cout << "failed: Popped values size mismatch: expected " << expected_size << ", got " << values.size() << std::endl;
//		return false;
//	}
//	std::vector<int> sorted = values;
//	std::sort(sorted.begin(), sorted.end());
//	for (int i = 0; i < expected_size; ++i) {
//		if (sorted[i] != i) {
//			std::cout << "failed: Value mismatch at index " << i << ": expected " << i << ", got " << sorted[i] << std::endl;
//			return false;
//		}
//	}
//	std::cout << "passed: All popped values are correct." << std::endl;
//	return true;
//}
//
//// ========== Test 1 ==========
//bool test1(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 1: Parallel push, then single-thread pop ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::cout << "[test1] Starting " << num_threads << " push threads..." << std::endl;
//	auto start = std::chrono::steady_clock::now();
//	perform_push_operations(queue, num_threads, num_per_thread);
//	auto push_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//	std::cout << "[test1] All push threads finished in " << push_elapsed << " s" << std::endl;
//
//	if (!validate_queue_size(queue, "Queue size after push", total)) return false;
//
//	std::vector<int> popped;
//	single_thread_pop(queue, total, popped);
//	if (!validate_queue_size(queue, "Queue size after pop", 0)) return false;
//	if (!validate_popped_values(popped, total)) return false;
//	print_monitor_count("test1 end");
//	return true;
//}
//
//// ========== Test 2 ==========
//bool test2(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 2: Concurrent push and pop (simultaneous) ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//	std::vector<int> popped;
//
//	concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//	if (!validate_queue_size(queue, "Queue size after concurrent", 0)) return false;
//	if (!validate_popped_values(popped, total)) return false;
//	print_monitor_count("test2 end");
//	return true;
//}
//
//// ========== Test 3A ==========
//bool test3A(int num_threads, int num_per_thread, int rounds) {
//	std::cout << "\n=== Test 3A: Multiple rounds (" << rounds << " rounds) ===" << std::endl;
//	for (int round = 0; round < rounds; ++round) {
//		std::cout << "--- Round " << round << " starting ---" << std::endl;
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		int total = num_threads * num_per_thread;
//		reset_counter();
//		std::vector<int> popped;
//
//		concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << "failed: Round " << round << " size mismatch (queue size=" << queue.size() << ", popped=" << popped.size() << ", total=" << total << ")" << std::endl;
//			return false;
//		}
//		if (!validate_popped_values(popped, total)) {
//			std::cout << "failed: Round " << round << " value mismatch" << std::endl;
//			return false;
//		}
//		if ((round + 1) % 5 == 0) {
//			std::cout << "  Completed round " << (round + 1) << " of " << rounds << std::endl;
//		}
//		print_monitor_count("test3A round " + std::to_string(round));
//	}
//	std::cout << "passed: All " << rounds << " rounds completed successfully" << std::endl;
//	return true;
//}
//
//// ========== Test 3B ==========
//bool test3B(int num_threads, int num_per_thread, int rounds) {
//	std::cout << "\n=== Test 3B: Multiple rounds with delay (" << rounds << " rounds) ===" << std::endl;
//	for (int round = 0; round < rounds; ++round) {
//		std::cout << "--- Round " << round << " starting ---" << std::endl;
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		int total = num_threads * num_per_thread;
//		reset_counter();
//		std::vector<int> popped;
//
//		if (round > 0) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		}
//
//		concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << "failed: Round " << round << " size mismatch (queue size=" << queue.size() << ", popped=" << popped.size() << ", total=" << total << ")" << std::endl;
//			return false;
//		}
//		if (!validate_popped_values(popped, total)) {
//			std::cout << "failed: Round " << round << " value mismatch" << std::endl;
//			return false;
//		}
//		if ((round + 1) % 5 == 0) {
//			std::cout << "  Completed round " << (round + 1) << " of " << rounds << std::endl;
//		}
//		print_monitor_count("test3B round " + std::to_string(round));
//	}
//	std::cout << "passed: All " << rounds << " rounds completed successfully" << std::endl;
//	return true;
//}
//
//// ========== Test 4 ==========
//bool test4() {
//	std::cout << "\n=== Test 4: Different push thread counts ===" << std::endl;
//	const int per_thread = 20000;
//	std::vector<int> thread_counts = { 50, 100, 200, 400 };
//	for (int num_threads : thread_counts) {
//		int total = num_threads * per_thread;
//		if (total > 2500000) continue;
//
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		reset_counter();
//		std::cout << "  Testing with " << num_threads << " threads, " << per_thread << " each (total " << total << ")..." << std::endl;
//		std::vector<int> popped;
//
//		auto start = std::chrono::steady_clock::now();
//		concurrent_push_pop(queue, num_threads, per_thread, popped);
//		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//
//		std::cout << "  Test completed in " << elapsed << " s" << std::endl;
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << " FAILED: queue.size=" << queue.size() << ", popped=" << popped.size() << ", expected=" << total << std::endl;
//			return false;
//		}
//		std::vector<int> sorted = popped;
//		std::sort(sorted.begin(), sorted.end());
//		bool valid = true;
//		for (int i = 0; i < total; ++i) {
//			if (sorted[i] != i) { valid = false; break; }
//		}
//		std::cout << (valid ? " PASSED" : " FAILED") << std::endl;
//		if (!valid) return false;
//		print_monitor_count("test4 threads=" + std::to_string(num_threads));
//	}
//	return true;
//}
//
//// ========== Test 5 ==========
//bool test5(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 5: High pressure test (recreate queue each time) ===" << std::endl;
//	for (int run = 0; run < 3; ++run) {
//		std::cout << "  Run " << run + 1 << " of 3" << std::endl;
//		reset_monitor_count();
//		LockFreeQueue<int> queue;
//		int total = num_threads * num_per_thread;
//		reset_counter();
//		std::vector<int> popped;
//
//		concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//
//		if (queue.size() != 0 || popped.size() != static_cast<size_t>(total)) {
//			std::cout << "failed: Run " << run << " size mismatch" << std::endl;
//			return false;
//		}
//		std::vector<int> sorted = popped;
//		std::sort(sorted.begin(), sorted.end());
//		bool valid = true;
//		for (int i = 0; i < total; ++i) {
//			if (sorted[i] != i) { valid = false; break; }
//		}
//		if (!valid) {
//			std::cout << "failed: Run " << run << " value mismatch" << std::endl;
//			return false;
//		}
//		std::cout << "  Run " << (run + 1) << " passed" << std::endl;
//		print_monitor_count("test5 run " + std::to_string(run));
//	}
//	std::cout << "passed: All pressure test runs passed" << std::endl;
//	return true;
//}
//
//// ========== Test 6 ==========
//bool test6(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 6: Push all, then single clear ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::cout << "[test6] Pushing " << total << " elements..." << std::endl;
//	auto start = std::chrono::steady_clock::now();
//	perform_push_operations(queue, num_threads, num_per_thread);
//	auto push_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
//	std::cout << "[test6] Push completed in " << push_elapsed << " s" << std::endl;
//
//	if (!validate_queue_size(queue, "Queue size before clear", total)) return false;
//
//	std::cout << "[test6] Clearing queue..." << std::endl;
//	queue.clear();
//	if (!validate_queue_size(queue, "Queue size after clear", 0)) return false;
//
//	std::cout << "[test6] Repushing and popping to verify..." << std::endl;
//	reset_counter();
//	std::vector<int> popped;
//	concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//	if (!validate_queue_size(queue, "Queue size after clear and repush", 0)) return false;
//	if (!validate_popped_values(popped, total)) return false;
//
//	print_monitor_count("test6 end");
//	std::cout << "passed: Clear test passed" << std::endl;
//	return true;
//}
//
//// ========== Test 7 ==========
//bool test7(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 7: Frequent clear during push ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	std::atomic<bool> push_done{ false };
//	std::atomic<int> clear_count{ 0 };
//	std::atomic<int> actual_pushed{ 0 };
//
//	// 启动 push 线程
//	std::vector<std::thread> push_threads;
//	for (int i = 0; i < num_threads; ++i) {
//		push_threads.emplace_back([&]() {
//			for (int j = 0; j < num_per_thread; ++j) {
//				queue.push(++global_counter);
//				actual_pushed++;
//			}
//		});
//	}
//
//	// 单线程频繁 clear
//	std::thread clear_thread([&]() {
//		while (!push_done.load()) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(1));
//			queue.clear();
//			clear_count++;
//			if (clear_count.load() % 100 == 0) {
//				std::cout << "  Clear count: " << clear_count.load()
//					<< ", queue size: " << queue.size() << std::endl;
//				print_monitor_count("test7 clear");
//			}
//		}
//		queue.clear();
//		std::cout << "Final clear performed, total clears: " << clear_count.load() << std::endl;
//	});
//
//	// 等待所有 push 完成
//	for (auto& t : push_threads) t.join();
//	push_done = true;
//	clear_thread.join();
//
//	if (!validate_queue_size(queue, "Queue size after final clear", 0)) return false;
//
//	std::cout << "Total pushes attempted: " << actual_pushed.load() << std::endl;
//	std::cout << "Total clears performed: " << clear_count.load() << std::endl;
//	print_monitor_count("test7 end");
//	std::cout << "passed: Frequent clear test passed (no crash)" << std::endl;
//	return true;
//}
//
//// ========== Test 8 ==========
//bool test8(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 8: Clear, then repush multiple times ===" << std::endl;
//	const int cycles = 5;
//	int total = num_threads * num_per_thread;
//
//	for (int cycle = 0; cycle < cycles; ++cycle) {
//		std::cout << "  Cycle " << (cycle + 1) << "/" << cycles << ": ";
//		reset_counter();
//		LockFreeQueue<int> queue;
//
//		perform_push_operations(queue, num_threads, num_per_thread);
//		int size_before_clear = queue.size();
//		if (size_before_clear != total) {
//			std::cout << "FAILED (size before clear: " << size_before_clear << ")" << std::endl;
//			return false;
//		}
//
//		queue.clear();
//		int size_after_clear = queue.size();
//		if (size_after_clear != 0) {
//			std::cout << "FAILED (size after clear: " << size_after_clear << ")" << std::endl;
//			return false;
//		}
//		std::cout << "OK" << std::endl;
//		print_monitor_count("test8 cycle " + std::to_string(cycle));
//	}
//	std::cout << "passed: Multiple clear-repush cycles passed" << std::endl;
//	return true;
//}
//
//// ========== Test 9 ==========
//bool test9(int num_threads, int num_per_thread) {
//	std::cout << "\n=== Test 9: Multi-thread clear requests, pop thread executes clear ===" << std::endl;
//	reset_monitor_count();
//	LockFreeQueue<int> queue;
//	int total = num_threads * num_per_thread;
//	reset_counter();
//
//	Semaphore clear_sem(0);
//	std::atomic<bool> test_running{ true };
//	std::atomic<bool> push_finished{ false };
//	std::atomic<int> clear_request_count{ 0 };
//	std::atomic<int> clear_executed_count{ 0 };
//	std::atomic<int> push_count{ 0 };
//	std::atomic<int> pop_count{ 0 };
//
//	// 多线程 push
//	std::vector<std::thread> push_threads;
//	for (int i = 0; i < num_threads; ++i) {
//		push_threads.emplace_back([&]() {
//			for (int j = 0; j < num_per_thread; ++j) {
//				queue.push(++global_counter);
//				push_count++;
//			}
//		});
//	}
//
//	// 多线程 clear 请求
//	const int clear_thread_count = 10;
//	std::vector<std::thread> clear_request_threads;
//	for (int i = 0; i < clear_thread_count; ++i) {
//		clear_request_threads.emplace_back([&]() {
//			while (!push_finished.load()) {
//				std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10 + 5));
//				clear_request_count++;
//				clear_sem.signal();
//				int req = clear_request_count.load();
//				if (req % 100 == 0) {
//					std::cout << "  Clear requests: " << req
//						<< ", executed: " << clear_executed_count.load() << std::endl;
//				}
//			}
//		});
//	}
//
//	// 单线程 pop + 执行 clear
//	std::thread pop_thread([&]() {
//		int value;
//		while (true) {
//			if (queue.pop(&value)) {
//				pop_count++;
//			}
//			if (clear_request_count.load() > clear_executed_count.load()) {
//				queue.clear();
//				clear_executed_count++;
//				if (clear_executed_count.load() % 50 == 0) {
//					std::cout << "  Clear executed #" << clear_executed_count.load()
//						<< ", queue size: " << queue.size()
//						<< ", requests: " << clear_request_count.load()
//						<< ", popped: " << pop_count.load() << std::endl;
//				}
//			}
//			if (push_finished.load() && push_count.load() >= total) {
//				while (clear_request_count.load() > clear_executed_count.load()) {
//					queue.clear();
//					clear_executed_count++;
//				}
//				break;
//			}
//			std::this_thread::yield();
//		}
//		while (queue.pop(&value)) pop_count++;
//		queue.clear();
//		clear_executed_count++;
//		std::cout << "  Pop thread finished." << std::endl;
//		std::cout << "  Total clears executed: " << clear_executed_count.load() << std::endl;
//		std::cout << "  Total pops: " << pop_count.load() << std::endl;
//	});
//
//	// 等待所有 push 完成
//	for (auto& t : push_threads) t.join();
//	push_finished = true;
//	std::cout << "All push threads finished. Total pushes: " << push_count.load() << std::endl;
//
//	for (auto& t : clear_request_threads) t.join();
//	std::cout << "All clear request threads finished. Total requests: " << clear_request_count.load() << std::endl;
//
//	pop_thread.join();
//
//	queue.clear();
//	if (!validate_queue_size(queue, "Queue size after final clear", 0)) return false;
//
//	reset_counter();
//	std::vector<int> popped;
//	concurrent_push_pop(queue, num_threads, num_per_thread, popped);
//	if (!validate_queue_size(queue, "Queue size after clear and repush", 0)) return false;
//	if (!validate_popped_values(popped, total)) return false;
//
//	print_monitor_count("test9 end");
//	std::cout << "passed: Multi-thread clear requests test passed" << std::endl;
//	return true;
//}
//
//// ========== main ==========
//#include "CDump/CDumpAPI.h"
//
//int main() {
//	CDump::declareDumpFile();
//
//#ifdef DEBUG_PUSH_POP
//	// 监控线程
//	std::atomic<bool> monitoring{ true };
//	auto monitor = std::thread([&]() {
//		while (monitoring) {
//			std::this_thread::sleep_for(std::chrono::seconds(10)); // 每10秒输出一次
//			std::cout << "[Background Monitor] push_success = "
//				<< LockFreeQueue<int>::g_push_success.load()
//				<< ", pop_success = "
//				<< LockFreeQueue<int>::g_pop_success.load()
//				<< std::endl;
//		}
//	});
//#endif
//
//	const int num_threads = 200;
//	const int num_per_thread = 10000;
//	const int repeat = 800;
//	bool all_passed = true;
//
//	for (int run = 1; run <= repeat; ++run) {
//		std::cout << "\n========== Run " << run << " of " << repeat << " ==========" << std::endl;
//		if (!test1(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test2(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test3A(num_threads, num_per_thread, 10)) { all_passed = false; break; }
//		if (!test3B(num_threads, num_per_thread, 10)) { all_passed = false; break; }
//		if (!test4()) { all_passed = false; break; }
//		if (!test5(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test6(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test7(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test8(num_threads, num_per_thread)) { all_passed = false; break; }
//		if (!test9(num_threads, num_per_thread)) { all_passed = false; break; }
//	}
//
//#ifdef DEBUG_PUSH_POP
//	monitoring = false;
//	if (monitor.joinable()) monitor.join();
//#endif
//
//	if (all_passed) {
//		std::cout << "\n================== ALL TESTS PASSED ==================\n";
//	}
//	else {
//		std::cout << "\n================== SOME TESTS FAILED ==================\n";
//	}
//	return all_passed ? 0 : 1;
//}