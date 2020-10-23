#include <engine/async.hpp>
#include <engine/deadline.hpp>
#include <engine/semaphore.hpp>
#include <engine/sleep.hpp>
#include <engine/task/task_context.hpp>

#include <utest/utest.hpp>

TEST(Semaphore, Ctr) { engine::Semaphore s{100}; }

TEST(Semaphore, OnePass) {
  RunInCoro([] {
    engine::Semaphore s{1};
    auto task = engine::impl::Async(
        [&s]() { std::shared_lock<engine::Semaphore> guard{s}; });

    task.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Semaphore, PassAcrossCoroutines) {
  RunInCoro([] {
    engine::Semaphore s{1};
    std::shared_lock<engine::Semaphore> guard{s};

    auto task = engine::impl::Async([guard = std::move(guard)]() {});
    task.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Semaphore, PassAcrossCoroutinesLocal) {
  RunInCoro([] {
    engine::Semaphore s{1};
    std::shared_lock<engine::Semaphore> guard{s};
    auto task2 = engine::impl::Async([guard = std::move(guard)]() mutable {
      std::shared_lock<engine::Semaphore> local_guard = std::move(guard);
    });
    task2.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task2.IsFinished());
  });
}

TEST(Semaphore, TwoPass) {
  RunInCoro([] {
    engine::Semaphore s{2};
    std::shared_lock<engine::Semaphore> guard1{s};
    auto task = engine::impl::Async(
        [&s]() { std::shared_lock<engine::Semaphore> guard2{s}; });

    task.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Semaphore, LockAndCancel) {
  RunInCoro([] {
    engine::Semaphore s{1};
    std::shared_lock<engine::Semaphore> guard{s};
    auto task = engine::impl::Async(
        [&s]() { std::shared_lock<engine::Semaphore> guard{s}; });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());
    guard.unlock();
  });
}

TEST(Semaphore, Lock2AndCancel) {
  RunInCoro([] {
    engine::Semaphore s{2};
    std::shared_lock<engine::Semaphore> guard{s};
    std::shared_lock<engine::Semaphore> guard1{s};
    auto task = engine::impl::Async(
        [&s]() { std::shared_lock<engine::Semaphore> guard{s}; });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());
    guard1.unlock();
  });
}

TEST(Semaphore, LocksUnlocks) {
  RunInCoro([] {
    engine::Semaphore s{1};
    auto multilocker = [&s]() {
      for (unsigned i = 0; i < 100; i++) {
        std::shared_lock<engine::Semaphore> guard{s};
        engine::Yield();
      }
    };

    auto task = engine::impl::Async(multilocker);
    multilocker();

    task.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Semaphore, LocksUnlocksMT) {
  RunInCoro(
      []() {
        engine::Semaphore s{1};
        auto multilocker = [&s]() {
          for (unsigned i = 0; i < 100; i++) {
            std::shared_lock<engine::Semaphore> guard{s};
            engine::Yield();
          }
        };

        auto task = engine::impl::Async(multilocker);
        multilocker();

        task.WaitFor(kMaxTestWaitTime);
        EXPECT_TRUE(task.IsFinished());
      },
      2);
}

TEST(Semaphore, LocksUnlocksMtTourture) {
  RunInCoro(
      []() {
        engine::Semaphore s{2};
        auto multilocker = [&s]() {
          for (unsigned i = 0; i < 100; i++) {
            std::shared_lock<engine::Semaphore> guard{s};
            engine::Yield();
          }
        };

        constexpr std::size_t kTasksCount = 8;
        engine::TaskWithResult<void> tasks[kTasksCount] = {
            engine::impl::Async(multilocker), engine::impl::Async(multilocker),
            engine::impl::Async(multilocker), engine::impl::Async(multilocker),
            engine::impl::Async(multilocker), engine::impl::Async(multilocker),
            engine::impl::Async(multilocker), engine::impl::Async(multilocker)};

        const auto deadline = engine::Deadline::FromDuration(kMaxTestWaitTime);
        for (auto& t : tasks) {
          t.WaitUntil(deadline);
          EXPECT_TRUE(t.IsFinished());
        }
      },
      4);
}

TEST(Semaphore, TryLock) {
  RunInCoro([] {
    engine::Semaphore sem(2);

    std::shared_lock<engine::Semaphore> lock(sem);
    EXPECT_TRUE(engine::impl::Async([&sem] {
                  return !!std::shared_lock<engine::Semaphore>(
                      sem, std::try_to_lock);
                })
                    .Get());
    EXPECT_TRUE(engine::impl::Async([&sem] {
                  return !!std::shared_lock<engine::Semaphore>(
                      sem, std::chrono::milliseconds(10));
                })
                    .Get());
    EXPECT_TRUE(engine::impl::Async([&sem] {
                  return !!std::shared_lock<engine::Semaphore>(
                      sem, std::chrono::system_clock::now());
                })
                    .Get());

    auto long_holder = engine::impl::Async([&sem] {
      std::shared_lock<engine::Semaphore> lock(sem);
      engine::InterruptibleSleepUntil(engine::Deadline{});
    });
    engine::Yield();

    EXPECT_FALSE(engine::impl::Async([&sem] {
                   return !!std::shared_lock<engine::Semaphore>(
                       sem, std::try_to_lock);
                 })
                     .Get());

    EXPECT_FALSE(engine::impl::Async([&sem] {
                   return !!std::shared_lock<engine::Semaphore>(
                       sem, std::chrono::milliseconds(10));
                 })
                     .Get());
    EXPECT_FALSE(engine::impl::Async([&sem] {
                   return !!std::shared_lock<engine::Semaphore>(
                       sem, std::chrono::system_clock::now());
                 })
                     .Get());

    auto long_waiter = engine::impl::Async([&sem] {
      return !!std::shared_lock<engine::Semaphore>(sem, kMaxTestWaitTime);
    });
    engine::Yield();
    EXPECT_FALSE(long_waiter.IsFinished());
    long_holder.RequestCancel();
    EXPECT_TRUE(long_waiter.Get());
  });
}
