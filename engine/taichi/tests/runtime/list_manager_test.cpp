// Standalone test of the actual runtime implementation, without JIT or Python.
// clang++-15 -std=c++17 -DARCH_x64 -pthread -I . \
//   tests/runtime/list_manager_test.cpp -o /tmp/list_manager_test
#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#include "taichi/runtime/llvm/runtime_module/runtime.cpp"

#define CHECK(condition)                                              \
  do {                                                                \
    if (!(condition)) {                                               \
      std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, \
                   #condition);                                       \
      std::abort();                                                   \
    }                                                                 \
  } while (false)

struct Allocations {
  std::mutex mutex;
  std::vector<void *> pointers;

  static void *allocate(void *opaque, std::size_t size, std::size_t alignment) {
    auto *self = static_cast<Allocations *>(opaque);
    auto padded = (size + alignment - 1) / alignment * alignment;
    void *ptr = std::aligned_alloc(alignment, padded);
    CHECK(ptr);
    // Match the runtime pool's zero-initialized allocation contract.
    std::memset(ptr, 0, padded);
    std::lock_guard<std::mutex> guard(self->mutex);
    self->pointers.push_back(ptr);
    return ptr;
  }

  ~Allocations() {
    for (auto ptr : pointers)
      std::free(ptr);
  }
};

int main() {
  Allocations allocations;
  auto runtime = new LLVMRuntime{};
  runtime->host_allocator = Allocations::allocate;
  runtime->memory_pool = reinterpret_cast<Ptr>(&allocations);
  ListManager list(runtime, sizeof(int), 16);
  static_assert(sizeof(ListManager) < 4096,
                "An empty list directory should fit within one runtime page");
  CHECK(list.get_num_active_chunks() == 0);
  CHECK(list.get_chunk(512) == nullptr);
  CHECK(allocations.pointers.empty());

  // Sparse out-of-order chunks cross pages and reach the final directory slot.
  const int chunks[] = {512, 511, 4096, int(ListManager::max_num_chunks - 1)};
  constexpr int workers = 8;
  std::atomic<int> ready{0};
  std::atomic<bool> start{false};
  std::vector<std::thread> threads;
  for (int worker = 0; worker < workers; worker++) {
    threads.emplace_back([&, worker] {
      ready.fetch_add(1);
      while (!start.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      for (int repeat = 0; repeat < 1000; repeat++) {
        for (int chunk : chunks) {
          // All threads contend for the same new page and same new chunk.
          auto ptr = list.touch_and_get(chunk * 16 + worker);
          *reinterpret_cast<int *>(ptr) = worker + repeat;
        }
      }
    });
  }
  while (ready.load() != workers)
    std::this_thread::yield();
  start.store(true, std::memory_order_release);
  for (auto &thread : threads)
    thread.join();

  CHECK(list.get_num_active_chunks() == 4);
  CHECK(allocations.pointers.size() == 8);  // Four pages, four data chunks.
  CHECK(runtime->total_requested_memory ==
        4 * (512 * sizeof(Ptr) + 16 * sizeof(int)));
  for (int chunk : chunks) {
    for (int worker = 0; worker < workers; worker++) {
      auto index = chunk * 16 + worker;
      auto ptr = list.get_element_ptr(index);
      CHECK(*reinterpret_cast<int *>(ptr) == worker + 999);
      CHECK(list.ptr2index(ptr) == index);
    }
  }

  // Clearing reuses existing storage; lookups do not materialize empty pages.
  auto stable = list.get_element_ptr(512 * 16);
  list.resize(123);
  list.clear();
  CHECK(list.size() == 0);
  CHECK(list.touch_and_get(512 * 16) == stable);
  CHECK(list.get_chunk(1024) == nullptr);
  CHECK(allocations.pointers.size() == 8);
  CHECK(list.get_num_active_chunks() == 4);
  CHECK(runtime->error_code == 0);
  delete runtime;
  std::puts("ListManager sparse directory and concurrent allocation passed");
}
