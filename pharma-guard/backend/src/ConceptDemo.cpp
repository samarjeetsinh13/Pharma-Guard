#include "ConceptDemo.h"

#include <mutex>
#include <sstream>
#include <type_traits>

#include "Checkers.h"
#include "Concurrency.h"
#include "Json.h"
#include "Models.h"
#include "Repository.h"
#include "Results.h"
#include "Text.h"

namespace {

// ---- Runnable style ("implements Runnable"): a task that adds to a shared counter.
class CounterTask : public Runnable {
public:
    CounterTask(int& counter, std::mutex& mutex, int times) : counter_(counter), mutex_(mutex), times_(times) {}
    void run() override {
        for (int i = 0; i < times_; ++i) {
            {
                std::lock_guard<std::mutex> guard(mutex_);  // lock ...
                ++counter_;                                  // ... only one thread can be here ...
            }                                                // ... unlock (end of scope)
            if (i % 250 == 0) ThreadRunner::yieldNow();
        }
    }

private:
    int& counter_;
    std::mutex& mutex_;
    int times_;
};

// ---- "extends Thread" style: inherit from ThreadRunner and override run().
class SleepyThread : public ThreadRunner {
public:
    SleepyThread(const std::string& name, TraceLog* log) : ThreadRunner(name, nullptr, log), log_(log) {
        traceSleeps(true);  // also record the TIMED_WAITING state in the trace
    }
    void run() override {
        log_->add(name() + ": my own run() is executing, going to sleep 30 ms");
        sleepMs(30);
        log_->add(name() + ": woke up");
    }

private:
    TraceLog* log_;
};

std::string lines(const std::vector<std::string>& v) { return Text::join(v, "\n"); }

std::string yesNo(bool b) { return b ? "yes" : "no"; }

}  // namespace

std::vector<DemoEntry> ConceptDemo::runAll(const Database& db) {
    std::vector<DemoEntry> out;
    const Medicine& first = db.medicines().all().front();
    const Medicine& second = db.medicines().all().size() > 1 ? db.medicines().all()[1] : first;

    // 1 ---------------------------------------------------------------- inheritance + access specifiers
    {
        std::ostringstream os;
        os << "Medicine inherits from Searchable and Printable (multiple inheritance).\n"
           << "Public getter  first.id()   -> " << first.id() << "\n"
           << "Private member Record::id_  -> cannot be read directly; only the public id() getter can.\n"
           << "Protected member name_      -> usable inside Medicine::describe(), hidden from main().\n"
           << "describe() -> " << first.describe();
        out.push_back({"Inheritance and access specifiers", "Models.h: Record, Searchable, Printable, Medicine", os.str()});
    }

    // 2 ---------------------------------------------------------------- constructors
    {
        int before = Record::instanceCount();
        Medicine byDefault;                                                          // default constructor
        Medicine byParams("X1", "Demo Drug", "demo generic", "demo class", "demo");  // parameterized constructor
        Medicine byCopy(byParams);                                                   // copy constructor
        std::ostringstream os;
        os << "Default constructor      -> \"" << byDefault.name() << "\"\n"
           << "Parameterized constructor -> \"" << byParams.name() << "\"\n"
           << "Copy constructor           -> \"" << byCopy.name() << "\" (copied from the previous one)\n"
           << "Delegating constructor: BatchNumber(const char*) calls BatchNumber(std::string).\n"
           << "Records alive: " << before << " before, " << Record::instanceCount() << " while these 3 exist.";
        out.push_back({"Constructors", "Models.cpp: Record(), Record(id,name), Record(const Record&)", os.str()});
    }

    // 3 ---------------------------------------------------------------- virtual inheritance
    {
        int before = Record::instanceCount();
        int during = 0;
        {
            Medicine temp("X2", "Temp", "g", "c", "u");
            during = Record::instanceCount();
        }
        int after = Record::instanceCount();
        std::ostringstream os;
        os << "Searchable and Printable both inherit 'virtual public Record'.\n"
           << "Record objects alive: before=" << before << ", while one Medicine exists=" << during
           << ", after=" << after << ".\n"
           << "One Medicine added exactly " << (during - before)
           << " Record (not 2): the shared virtual base exists only once - no diamond problem.";
        out.push_back({"Virtual inheritance", "Models.h: class Searchable : virtual public Record", os.str()});
    }

    // 4 ---------------------------------------------------------------- virtual functions + base pointers
    {
        std::vector<std::unique_ptr<Checker>> checkers;
        checkers.push_back(makeChecker("suitability", db));
        checkers.push_back(makeChecker("interaction", db));
        checkers.push_back(makeChecker("counterfeit", db));
        std::ostringstream os;
        os << "A vector of Checker* (base pointers) holding derived objects:\n";
        for (const auto& c : checkers) os << "  checker->moduleName() -> " << c->moduleName() << "   (virtual call picks the derived version)\n";

        const Record& asRecord = first;  // base reference to a derived object
        os << "Record& ref = medicine;  ref.describe() -> " << asRecord.describe();
        out.push_back({"Virtual functions and base-class pointers", "Checkers.h: Checker::moduleName() / execute()", os.str()});

        // 5 ------------------------------------------------------------ object / derived pointers
        Checker* basePtr = checkers[1].get();
        auto* asInteraction = dynamic_cast<InteractionChecker*>(basePtr);
        auto* asCounterfeit = dynamic_cast<CounterfeitChecker*>(basePtr);
        Record* recPtr = const_cast<Medicine*>(&first);
        std::ostringstream os2;
        os2 << "Checker* basePtr = interaction checker;\n"
            << "dynamic_cast<InteractionChecker*>(basePtr) -> " << (asInteraction ? "valid pointer" : "nullptr") << "\n"
            << "dynamic_cast<CounterfeitChecker*>(basePtr) -> " << (asCounterfeit ? "valid pointer" : "nullptr")
            << "  (wrong derived type is detected safely)\n"
            << "Record* -> Medicine* cast works: " << yesNo(dynamic_cast<Medicine*>(recPtr) != nullptr)
            << ", Record* -> Illness* cast works: " << yesNo(dynamic_cast<Illness*>(recPtr) != nullptr);
        out.push_back({"Object pointers and derived-class pointers", "Checkers.cpp makeChecker(); ConceptDemo.cpp", os2.str()});
    }

    // 6 ---------------------------------------------------------------- object slicing
    {
        Medicine original = first;
        Record sliced = original;          // SLICING: only the Record part is copied
        const Record& reference = original;  // a reference keeps the whole object
        std::ostringstream os;
        os << "Record sliced = medicine;      -> the Medicine-specific data is cut off\n"
           << "  sliced.describe()    -> " << sliced.describe() << "\n"
           << "const Record& ref = medicine; -> nothing is cut off\n"
           << "  ref.describe()       -> " << reference.describe() << "\n"
           << "Lesson: pass polymorphic objects by pointer or reference, never by value.";
        out.push_back({"Object slicing", "ConceptDemo.cpp (Record copy of a Medicine)", os.str()});
    }

    // 7 ---------------------------------------------------------------- operators + friend
    {
        std::ostringstream os;
        os << "Member operator==   : (first == first)  -> " << yesNo(first == first) << ", (first == second) -> "
           << yesNo(first == second) << "\n"
           << "Member operator<    : (first < second)  -> " << yesNo(first < second) << " (compares names)\n"
           << "Friend operator<<   : cout << medicine  -> " << first << "\n"
           << "Friend operator==   : BatchNumber(\"pcm-2501-a12\") == BatchNumber(\"PCM 2501 A12\") -> "
           << yesNo(BatchNumber("pcm-2501-a12") == BatchNumber("PCM 2501 A12")) << "\n"
           << "Member operator<<   : result << \"note\" adds a note to a CheckResult (see Results.h)";
        out.push_back({"Operator overloading and friend operators", "Models.h: Medicine, BatchNumber; Results.h: CheckResult", os.str()});
    }

    // 8 ---------------------------------------------------------------- method overloading
    {
        SuitabilityResult demo("Fever", "Fever", true);
        demo.addNote("one argument").addNote("Label", "two arguments");
        std::ostringstream os;
        os << "Text::join(parts)            -> " << Text::join({"a", "b", "c"}) << "\n"
           << "Text::join(parts, \" | \")     -> " << Text::join({"a", "b", "c"}, " | ") << "\n"
           << "CheckResult::addNote(note) and addNote(label, value) are two overloads (see Results.h).\n"
           << "Database::findInteraction(idA, idB) and findInteraction(Medicine, Medicine) are two overloads.\n"
           << "JsonObject::add has overloads for string, const char*, bool, int, size_t and double.";
        out.push_back({"Method overloading", "Text.h, Results.h, Database.h, Json.h", os.str()});
    }

    // 9 ---------------------------------------------------------------- conversions
    {
        BatchNumber batch = "dolo 24-a117";  // basic -> class (const char* -> BatchNumber)
        Severity severity = 2;               // basic -> class (int -> Severity)
        std::string asText = batch;          // class -> basic (BatchNumber -> std::string)
        int asNumber = severity;             // class -> basic (Severity -> int)

        SuitabilityResult result("Fever", "Fever", true);
        SuitabilityResult::Item item{"Paracetamol", "Paracetamol", "Analgesic", "SUITABLE", "demo"};
        result.addItem(item);
        ResultSummary summary = result;      // class -> class (SuitabilityResult -> ResultSummary)

        std::ostringstream os;
        os << "BASIC -> CLASS : BatchNumber batch = \"dolo 24-a117\";  key stored as " << batch.key() << "\n"
           << "BASIC -> CLASS : Severity severity = 2;                label = " << severity.label() << "\n"
           << "CLASS -> BASIC : std::string asText = batch;           -> \"" << asText << "\"\n"
           << "CLASS -> BASIC : int asNumber = severity;              -> " << asNumber << "\n"
           << "CLASS -> CLASS : ResultSummary summary = result;       -> status=" << summary.status()
           << ", headline=\"" << summary.headline() << "\"";
        out.push_back({"Basic-to-class, class-to-basic and class-to-class conversion",
                       "Models.h: BatchNumber, Severity; Results.h: CheckResult::operator ResultSummary()", os.str()});
    }

    // 10 --------------------------------------------------------------- templates
    {
        Repository<int> numbers;
        numbers.add(5);
        numbers.add(9);
        numbers.add(2);
        const int* firstBig = numbers.findIf([](int v) { return v > 6; });
        std::ostringstream os;
        os << "Repository<int> and Repository<Medicine> come from ONE class template.\n"
           << "numbers.findIf(v > 6)          -> " << (firstBig ? std::to_string(*firstBig) : "none") << "\n"
           << "maxOf(3, 7)                    -> " << maxOf(3, 7) << "\n"
           << "maxOf(\"pear\", \"apple\") (strings) -> " << maxOf(std::string("pear"), std::string("apple")) << "\n"
           << "maxOf(Severity(1), Severity(3)) -> " << maxOf(Severity(1), Severity(3)).label() << "\n"
           << "joinValues({1,2,3}, \"-\")        -> " << joinValues(std::vector<int>{1, 2, 3}, "-") << "\n"
           << "Medicines in Repository<Medicine>: " << db.medicines().size()
           << ", illnesses in Repository<Illness>: " << db.illnesses().size();
        out.push_back({"Templates", "Repository.h; used in Database.h and Results.cpp", os.str()});
    }

    // 11 --------------------------------------------------------------- static
    {
        std::ostringstream os;
        os << "Static variables: Record::count_ and Checker::runs_ (one copy shared by all objects).\n"
           << "Static functions: Record::instanceCount() -> " << Record::instanceCount()
           << ", Checker::totalRuns() -> " << Checker::totalRuns() << ", Text::today() -> " << Text::today() << "\n"
           << "Static functions are called with ClassName::function(), no object needed.";
        out.push_back({"Static variables and static functions", "Models.h, Checkers.h, Text.h", os.str()});
    }

    // 12 --------------------------------------------------------------- final
    {
        std::ostringstream os;
        os << "class CounterfeitChecker final : public Checker   -> std::is_final = "
           << yesNo(std::is_final<CounterfeitChecker>::value) << "\n"
           << "class InteractionChecker (not final)             -> std::is_final = "
           << yesNo(std::is_final<InteractionChecker>::value) << "\n"
           << "InteractionChecker::moduleName() is 'override final': a class derived from it\n"
           << "could not override moduleName() again. The compiler enforces both rules.";
        out.push_back({"final keyword", "Checkers.h", os.str()});
    }

    // 13 --------------------------------------------------------------- this pointer
    {
        Medicine temp;
        Record& step1 = temp.setName("Alpha");
        Record& step2 = step1.setName("Beta");
        std::ostringstream os;
        os << "Record::setName stores the name through this->name_ and returns *this.\n"
           << "Is the returned reference the same object?  " << yesNo(&step1 == &temp && &step2 == &temp) << "\n"
           << "Chained call temp.setName(\"Alpha\").setName(\"Beta\") -> name is now \"" << temp.name() << "\"";
        out.push_back({"this pointer", "Models.cpp: Record::setName; Results.cpp: CheckResult::addNote", os.str()});
    }

    // 14 --------------------------------------------------------------- threads
    {
        // (a) Runnable style + mutex
        int counter = 0;
        std::mutex mutex;
        std::vector<std::unique_ptr<CounterTask>> tasks;
        std::vector<std::unique_ptr<ThreadRunner>> threads;
        for (int i = 0; i < 4; ++i) {
            tasks.push_back(std::make_unique<CounterTask>(counter, mutex, 1000));
            threads.push_back(std::make_unique<ThreadRunner>("counter-" + std::to_string(i + 1), tasks.back().get()));
        }
        for (auto& t : threads) t->start();
        for (auto& t : threads) t->join();

        // (b) "extends Thread" style + lifecycle
        TraceLog log;
        SleepyThread sleepy("sleepy", &log);
        std::string stateBefore = toString(sleepy.state());
        sleepy.start();
        sleepy.join();
        std::string stateAfter = toString(sleepy.state());

        std::ostringstream os;
        os << "(a) Runnable style: 4 threads x 1000 increments of ONE shared counter,\n"
           << "    each increment protected by std::lock_guard<std::mutex>.\n"
           << "    Final counter = " << counter << " (expected 4000) -> " << yesNo(counter == 4000) << "\n"
           << "(b) \"extends Thread\" style: SleepyThread derives from ThreadRunner and overrides run().\n"
           << "    state before start() = " << stateBefore << ", state after join() = " << stateAfter << "\n"
           << "    Lifecycle trace:\n"
           << lines(log.snapshot()) << "\n"
           << "sleep -> ThreadRunner::sleepMs, yield -> ThreadRunner::yieldNow, join -> ThreadRunner::join.\n"
           << "The Interaction module uses the same classes: see PairScanTask and SharedFindings.";
        out.push_back({"Threads, Runnable, lifecycle, sleep / join / yield, mutex",
                       "Concurrency.h, Checkers.cpp (InteractionChecker), ConceptDemo.cpp", os.str()});
    }

    return out;
}

std::string ConceptDemo::toJson(const std::vector<DemoEntry>& entries) {
    JsonArray array;
    for (const DemoEntry& e : entries) {
        JsonObject o;
        o.add("topic", e.topic).add("where", e.where).add("output", e.output);
        array.addRaw(o.str());
    }
    return array.str();
}

std::string ConceptDemo::toText(const std::vector<DemoEntry>& entries) {
    std::ostringstream os;
    int n = 1;
    for (const DemoEntry& e : entries) {
        os << "=== " << n++ << ". " << e.topic << " ===\n"
           << "Where: " << e.where << "\n"
           << e.output << "\n\n";
    }
    return os.str();
}
