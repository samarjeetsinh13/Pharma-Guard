# C++ concepts - where to find them

Concepts are used only where they fit the application. Run `pharma_guard --demo` (or open the **C++ Concepts**
tab) to see each one produce output. Paths are inside `backend/`.

| Concept | Where (file: class / function) | Why it is there |
|---------|-------------------------------|-----------------|
| **Inheritance** | `Models.h`: `Medicine : Searchable, Printable`; `Illness : Searchable, Printable`; `Checkers.h`: `SuitabilityChecker / InteractionChecker / CounterfeitChecker : Checker` | Medicines and illnesses share id/name/alias behaviour; the three modules share one `Checker` interface. |
| **Access specifiers** | `Models.h`: `Record` has `public` getters, `protected` `name_`, `private` `id_` | Data hiding: only `Record` may touch `id_`, derived classes may use `name_`. |
| **Constructors** | `Models.cpp`: `Record()`, `Record(id,name)`, `Record(const Record&)`; `Medicine(...)` | Default, parameterized and copy constructors; constructor order with virtual bases. |
| **Virtual inheritance** | `Models.h`: `Searchable : virtual public Record`, `Printable : virtual public Record` | `Medicine` inherits both, yet contains only **one** `Record` (no diamond problem). |
| **Virtual functions** | `Models.h`: `Record::describe()`, `Searchable::matches()`, pure virtual `Printable::summary()`; `Checkers.h`: `Checker::execute()` | Runtime polymorphism. |
| **Object / derived pointers** | `Checkers.cpp`: `makeChecker()` returns `Checker*` (base pointer to derived object); `ConceptDemo.cpp` | The server calls `checker->execute()` without knowing which module it is. |
| **Object slicing** | `ConceptDemo.cpp`: `Record sliced = original;` | Shows what is lost when a derived object is copied into a base object. |
| **Static variables / functions** | `Models.h`: `Record::count_` + `Record::instanceCount()`; `Checkers.h`: `Checker::runs_` + `totalRuns()`; `Text.h`: all-static helper class | Shared counters; utility functions that need no object. |
| **`final`** | `Checkers.h`: `class CounterfeitChecker final`; `InteractionChecker::moduleName() override final` | Forbids further inheritance / overriding. |
| **`this` pointer** | `Models.cpp`: `Record::setName` returns `*this` (method chaining); `Results.cpp`: `CheckResult::addNote` | Chaining and disambiguation. |
| **Operator overloading (member)** | `Models.h`: `Medicine::operator==`, `operator<` | Compare by id, sort by name. |
| **Friend operator overloading** | `Models.h`: `friend operator<<(ostream&, Medicine)`, `friend operator==(BatchNumber, BatchNumber)`; `Results.h` | `std::cout << medicine`; batch comparison ignores spaces/dashes/case. |
| **Method overloading** | `Text.h`: `join(parts)` / `join(parts, sep)`; `Database.h`: `findInteraction(idA,idB)` / `findInteraction(Medicine,Medicine)`; `Json.h`, `Results.h` | Same job, different parameters. |
| **Basic-to-class conversion** | `Models.h`: `BatchNumber(const std::string&)`, `BatchNumber(const char*)`, `Severity(int)` | `BatchNumber b = "pcm 2501-a12";` |
| **Class-to-basic conversion** | `Models.h`: `BatchNumber::operator std::string()`, `Severity::operator int()` | Use a `BatchNumber` where a string / number is expected. |
| **Class-to-class conversion** | `Results.h`: `CheckResult::operator ResultSummary()` | Any result converts to a small summary object. |
| **Nested classes** | `Database.h`: `Database::Stats`; `Results.h`: `CounterfeitResult::Check` | A helper type that only makes sense inside its owner. |
| **Templates** | `Repository.h`: `template <typename T> class Repository` (used as `Repository<Medicine>`, `Repository<Illness>`); function templates `findByQuery<T>` (`Database.cpp`) and `maxOf` (`Results.cpp`) | One container for several record types. |
| **C++ threads** | `Concurrency.h/.cpp`: `ThreadRunner` wraps `std::thread`; `Checkers.cpp`: `InteractionChecker` starts worker threads | Pairs are checked in parallel, off the request thread. |
| **Runnable-style abstraction** | `Concurrency.h`: `class Runnable { virtual void run() = 0; }`; `Checkers.h`: `PairScanTask : Runnable` | See the mapping below. |
| **Thread lifecycle** | `Concurrency.h`: `enum class ThreadState { New, Runnable, Running, TimedWaiting, Terminated }` | Shown live in the *Thread activity* trace. |
| **sleep / join / yield** | `ThreadRunner::sleepMs`, `join`, `yieldNow`; used in `Checkers.cpp` | `sleep` simulates a slow lookup, `yield` hands the CPU over, `join` waits for workers. |
| **Mutex synchronization** | `Checkers.h`: `SharedFindings` (`std::mutex` + `std::lock_guard`); `Concurrency.h`: `TraceLog` | Worker threads append to one shared list safely; the lock count is reported. |

## Java-only concepts → closest C++ equivalent

C++ has no `Thread` base class and no `Runnable` interface, so the project builds them from standard parts:

| Java | This project |
|------|--------------|
| `interface Runnable { void run(); }` | `class Runnable { virtual void run() = 0; }` (abstract class) |
| `class A implements Runnable` | `class PairScanTask : public Runnable` |
| `new Thread(runnable)` | `ThreadRunner t("worker-1", &task);` |
| `class B extends Thread` | `class SleepyThread : public ThreadRunner` overriding `run()` (see `ConceptDemo.cpp`) |
| `t.start()` / `t.join()` | `ThreadRunner::start()` (creates a `std::thread`) / `ThreadRunner::join()` |
| `Thread.sleep(ms)` / `Thread.yield()` | `ThreadRunner::sleepMs(ms)` / `ThreadRunner::yieldNow()` |
| `synchronized (lock) { }` | `std::lock_guard<std::mutex> guard(mutex_);` |
| `Thread.getState()` | `ThreadRunner::state()` returning our `ThreadState` |
| `final class` / `final` method | `class X final` / `override final` |
