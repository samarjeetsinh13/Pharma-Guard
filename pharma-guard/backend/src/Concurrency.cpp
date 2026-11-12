#include "Concurrency.h"

#include <cstdio>
#include <stdexcept>

const char* toString(ThreadState state) {
    switch (state) {
        case ThreadState::New: return "NEW";
        case ThreadState::Runnable: return "RUNNABLE";
        case ThreadState::Running: return "RUNNING";
        case ThreadState::TimedWaiting: return "TIMED_WAITING";
        case ThreadState::Terminated: return "TERMINATED";
    }
    return "UNKNOWN";
}

// ----------------------------------------------------------------- TraceLog

TraceLog::TraceLog() : start_(std::chrono::steady_clock::now()) {}

void TraceLog::add(const std::string& message) {
    // lock_guard locks the mutex now and unlocks it automatically when it goes
    // out of scope (even if an exception is thrown). Only one thread at a time
    // can be inside this block - that is the synchronization.
    std::lock_guard<std::mutex> guard(mutex_);
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start_)
                       .count();
    char stamp[32];
    std::snprintf(stamp, sizeof stamp, "[+%4lld ms] ", ms);
    lines_.push_back(std::string(stamp) + message);
}

std::vector<std::string> TraceLog::snapshot() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return lines_;  // returns a COPY, so the caller can read it without holding the lock
}

// ------------------------------------------------------------- ThreadRunner

thread_local ThreadRunner* ThreadRunner::current_ = nullptr;

ThreadRunner::ThreadRunner(const std::string& name, Runnable* target, TraceLog* trace)
    : name_(name), target_(target), trace_(trace), state_(ThreadState::New) {
    if (trace_) trace_->add(name_ + ": created -> state NEW");
}

ThreadRunner::~ThreadRunner() {
    // If a std::thread is destroyed while still joinable the program terminates,
    // so we always join here as a safety net.
    if (thread_.joinable()) thread_.join();
}

void ThreadRunner::setState(ThreadState next, bool log) {
    state_.store(next);
    if (log && trace_) trace_->add(name_ + ": state " + toString(next));
}

void ThreadRunner::start() {
    if (state_.load() != ThreadState::New) {
        throw std::logic_error("Thread '" + name_ + "' was already started");  // like Java's IllegalThreadStateException
    }
    setState(ThreadState::Runnable, true);
    thread_ = std::thread(&ThreadRunner::body, this);  // the new thread starts running body()
}

void ThreadRunner::join() {
    if (thread_.joinable()) thread_.join();
}

void ThreadRunner::body() {
    current_ = this;
    setState(ThreadState::Running, true);
    try {
        run();  // virtual call: runs the subclass version or the target's run()
    } catch (const std::exception& e) {
        if (trace_) trace_->add(name_ + ": run() failed: " + e.what());
    }
    setState(ThreadState::Terminated, true);
    current_ = nullptr;
}

void ThreadRunner::run() {
    if (target_) target_->run();
}

void ThreadRunner::sleepMs(int milliseconds) {
    ThreadRunner* self = current_;  // nullptr when called from a thread we did not create
    if (self) self->setState(ThreadState::TimedWaiting, self->traceSleeps_);
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    if (self) self->setState(ThreadState::Running, self->traceSleeps_);
}

void ThreadRunner::yieldNow() { std::this_thread::yield(); }
