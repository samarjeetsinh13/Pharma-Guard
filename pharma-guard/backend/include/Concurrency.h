// Concurrency.h - threads, a Runnable-style abstraction, and mutex synchronization.
//
// JAVA -> C++ MAPPING (C++ has no "extends Thread" / "implements Runnable"):
//
//   Java                              | This project (C++17)
//   ----------------------------------+------------------------------------------------
//   interface Runnable { run(); }     | abstract class Runnable { virtual void run() = 0; }
//   class A implements Runnable       | class A : public Runnable        (PairScanTask)
//   new Thread(runnable)              | ThreadRunner t("name", &runnable);
//   class B extends Thread            | class B : public ThreadRunner, overriding run()
//   thread.start()                    | ThreadRunner::start()   -> std::thread is created
//   thread.join()                     | ThreadRunner::join()    -> std::thread::join()
//   Thread.sleep(ms)                  | ThreadRunner::sleepMs(ms) -> std::this_thread::sleep_for
//   Thread.yield()                    | ThreadRunner::yieldNow()  -> std::this_thread::yield
//   synchronized (lock) { ... }       | std::lock_guard<std::mutex> guard(mutex_);
//   Thread.getState()                 | ThreadRunner::state()  (our own enum, see below)
//
// THREAD LIFECYCLE (names follow Java's Thread.State)
//   NEW           the ThreadRunner object exists, start() not called yet
//   RUNNABLE      start() was called; the OS may schedule the thread
//   RUNNING       the thread is executing run()
//   TIMED_WAITING the thread is inside sleepMs()
//   TERMINATED    run() finished (or threw); the thread is done
#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// The Runnable-style abstraction: "something that can be run on a thread".
class Runnable {
public:
    virtual ~Runnable() = default;
    virtual void run() = 0;
};

enum class ThreadState { New, Runnable, Running, TimedWaiting, Terminated };
const char* toString(ThreadState state);

// A thread-safe list of text messages. Several threads can add() at the same time
// because every access is protected by a mutex.
class TraceLog {
public:
    TraceLog();
    void add(const std::string& message);
    std::vector<std::string> snapshot() const;

private:
    mutable std::mutex mutex_;  // "mutable" so const functions (snapshot) can still lock it
    std::vector<std::string> lines_;
    std::chrono::steady_clock::time_point start_;
};

// ThreadRunner wraps std::thread and tracks the lifecycle.
// It is itself a Runnable, so both Java styles work:
//   (1) give it a Runnable target  ->  ThreadRunner t("w1", &task);
//   (2) inherit from it and override run()  ->  class MyThread : public ThreadRunner
class ThreadRunner : public Runnable {
public:
    ThreadRunner(const std::string& name, Runnable* target = nullptr, TraceLog* trace = nullptr);
    ~ThreadRunner() override;  // joins the thread if the caller forgot to

    ThreadRunner(const ThreadRunner&) = delete;
    ThreadRunner& operator=(const ThreadRunner&) = delete;

    void start();  // NEW -> RUNNABLE, then the new thread moves itself to RUNNING
    void join();   // waits until the thread is TERMINATED

    // Default run(): call the target. Subclasses override this ("extends Thread" style).
    void run() override;

    ThreadState state() const { return state_.load(); }
    const std::string& name() const { return name_; }
    void traceSleeps(bool on) { traceSleeps_ = on; }

    static void sleepMs(int milliseconds);  // pause the calling thread
    static void yieldNow();                 // politely let other threads run

private:
    void body();                            // the function the new std::thread executes
    void setState(ThreadState next, bool log);

    std::string name_;
    Runnable* target_;
    TraceLog* trace_;
    bool traceSleeps_ = false;
    std::atomic<ThreadState> state_;
    std::thread thread_;

    static thread_local ThreadRunner* current_;  // which ThreadRunner owns the calling thread
};
