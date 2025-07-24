#include "list.hpp"

#include <catch2/catch_test_macros.hpp>

#include <overload.hpp>

#include <list>
#include <random>
#include <source_location>
#include <variant>
#include <vector>

class CorrectList {
  public:
    CorrectList(const CorrectList&) = delete;
    CorrectList& operator=(const CorrectList&) = delete;

    CorrectList(CorrectList&& other) : inner_(std::move(other.inner_)) {
    }

    CorrectList& operator=(CorrectList&& other) {
        inner_ = std::move(other.inner_);
        return *this;
    }

    CorrectList() {
    }

    void PushBack(int value) {
        inner_.push_back(value);
    }

    void PushFront(int value) {
        inner_.push_front(value);
    }

    void PopBack() {
        inner_.pop_back();
    }

    void PopFront() {
        inner_.pop_front();
    }

    int& Back() {
        return inner_.back();
    }

    int& Front() {
        return inner_.front();
    }

    bool IsEmpty() const {
        return inner_.empty();
    }

    void Swap(CorrectList& other) {
        inner_.swap(other.inner_);
    }

    void Splice(CorrectList& other) {
        inner_.splice(inner_.end(), other.inner_);
    }

    template <class F>
    void ForEachElement(F&& f) const {
        for (auto x : inner_) {
            f(x);
        }
    }

  private:
    std::list<int> inner_;
};

template <class T>
std::vector<int> CollectToVec(const T& l) {
    std::vector<int> values;
    l.ForEachElement([&](int v) { values.push_back(v); });
    return values;
}

TEST_CASE("Functioning") {
    List l1;

    REQUIRE(l1.IsEmpty());

    {
        List l2;
        l2.Swap(l1);
        REQUIRE(l1.IsEmpty());
        REQUIRE(l2.IsEmpty());
    }

    {
        List l2(std::move(l1));
        REQUIRE(l1.IsEmpty());
        REQUIRE(l2.IsEmpty());
    }

    l1.PushBack(4);
    REQUIRE(!l1.IsEmpty());
    REQUIRE(l1.Front() == 4);
    REQUIRE(l1.Back() == 4);

    l1.PushFront(3);
    REQUIRE(l1.Front() == 3);
    REQUIRE(l1.Back() == 4);

    l1.PushBack(5);
    l1.PushFront(2);
    l1.PushFront(1);
    l1.PushBack(6);
    REQUIRE(!l1.IsEmpty());
    REQUIRE(l1.Front() == 1);
    REQUIRE(l1.Back() == 6);

    REQUIRE(CollectToVec(l1) == std::vector{1, 2, 3, 4, 5, 6});

    {
        List l2;
        l2.PushBack(7);
        l2.PushBack(8);
        l2.PushBack(9);
        REQUIRE(CollectToVec(l2) == std::vector{7, 8, 9});

        l2.Swap(l1);
        REQUIRE(CollectToVec(l2) == std::vector{1, 2, 3, 4, 5, 6});

        l2.Splice(l1);
        l1.Swap(l2);
        REQUIRE(CollectToVec(l1) == std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9});
        REQUIRE(l2.IsEmpty());
        REQUIRE(CollectToVec(l2) == std::vector<int>{});
    }

    {
        List l2;
        l2.Splice(l1);
        REQUIRE(l1.IsEmpty());
        REQUIRE(CollectToVec(l1) == std::vector<int>{});

        l2.Splice(l1);
        REQUIRE(l1.IsEmpty());
        REQUIRE(CollectToVec(l1) == std::vector<int>{});

        List l3;
        l3.Splice(l1);
        REQUIRE(CollectToVec(l1) == std::vector<int>{});
        REQUIRE(l1.IsEmpty());
        REQUIRE(CollectToVec(l3) == std::vector<int>{});
        REQUIRE(l3.IsEmpty());

        l2.Swap(l1);
        REQUIRE(l1.Front() == 1);
        REQUIRE(l1.Back() == 9);
    }

    {
        List l2;
        l2.PushBack(1);
        l1.Swap(l2);
        REQUIRE(CollectToVec(l1) == std::vector{1});
    }

    {
        List l2;
        l1.Swap(l2);
        REQUIRE(CollectToVec(l2) == std::vector{1});
        REQUIRE(l1.IsEmpty());
        REQUIRE(!l2.IsEmpty());
    }

    {
        List l2{std::move(l1)};
        REQUIRE(CollectToVec(l2) == std::vector<int>{});
    }

    l1.PushBack(1);
    l1.PushBack(2);

    {
        List l2{std::move(l1)};
        REQUIRE(CollectToVec(l2) == std::vector{1, 2});
        REQUIRE(CollectToVec(l1) == std::vector<int>{});
    }
}

template <class T, class F>
void Operate(F&& report) {
    static constexpr size_t kMinLists = 5;
    static constexpr size_t kMaxLists = 20;

    std::mt19937 rng{424243};

    std::vector<T> lists(5);

    auto pick_one = [&rng, &lists] { return rng() % lists.size(); };
    auto pick_two = [&rng, &lists] {
        static_assert(kMinLists >= 2);
        auto idx1 = rng() % lists.size();
        auto idx2 = rng() % (lists.size() - 1);
        if (idx2 >= idx1) {
            ++idx2;
        }
        return std::pair<size_t, size_t>{idx1, idx2};
    };

    for (size_t i = 0; i < 200'000; ++i) {
        auto choice = rng() % 9;
        switch (choice) {
        case 0: {
            if (lists.size() == kMaxLists) {
                continue;
            }
            if ((rng() & 1) == 0) {
                T l{std::move(lists[pick_one()])};
                lists.emplace_back(std::move(l));
            } else {
                lists.emplace_back();
            }
            break;
        }
        case 1: {
            if (lists.size() == kMinLists) {
                continue;
            }
            auto idx = pick_one();
            if (idx != lists.size() - 1) {
                lists.back().Swap(lists[idx]);
            }
            report(CollectToVec(lists.back()));
            lists.pop_back();
            break;
        }
        case 2:
        case 3:
        case 4: {
            auto idx = pick_one();
            auto& l = lists[idx];
            int value = static_cast<int>(rng() % 1024);
            if (rng() & 1) {
                l.PushBack(value);
                report(l.Back());
            } else {
                l.PushFront(value);
                report(l.Front());
            }
            break;
        }
        case 5: {
            auto idx = pick_one();
            auto& l = lists[idx];
            report(l.IsEmpty());
            if (l.IsEmpty()) {
                continue;
            }
            if (rng() & 1) {
                report(l.Front());
                l.PopFront();
            } else {
                report(l.Back());
                l.PopBack();
            }
            break;
        }
        case 6: {
            auto [idx1, idx2] = pick_two();
            auto& l = lists[idx1];
            l.Swap(lists[idx2]);
            report(l.IsEmpty());
            if (!l.IsEmpty()) {
                report(l.Front());
                report(l.Back());
            }
            break;
        }
        case 7: {
            auto [idx1, idx2] = pick_two();
            auto& l = lists[idx1];
            l.Splice(lists[idx2]);
            report(l.IsEmpty());
            if (!l.IsEmpty()) {
                report(l.Front());
                report(l.Back());
            }
            break;
        }
        case 8: {
            auto [idx1, idx2] = pick_two();
            lists[idx1] = std::move(lists[idx2]);
            break;
        }
        }
    }
}

template <class F>
void CheckBehaveTheSame(F&& fun) {
    using Record = std::variant<int, std::vector<int>>;
    std::vector<std::pair<Record, std::source_location>> history;

    fun.template operator()<CorrectList>(
        [&history]<class T>(T value, std::source_location loc =
                                         std::source_location::current()) {
            history.emplace_back(std::move(value), loc);
        });

    auto format = Overload{
        [](int value) { return std::to_string(value); },
        [](const std::vector<int>& value) {
            std::string s = "{";
            bool first = true;
            for (auto x : value) {
                if (!std::exchange(first, false)) {
                    s += ", ";
                }
                s += std::to_string(x);
            }
            s += "}";
            return s;
        },
    };
    auto format_record = [format](const Record& r) {
        return std::visit(format, r);
    };

    size_t current = 0;
    fun.template operator()<List>([&history, &current, format_record]<class T>(
                                      T value,
                                      std::source_location loc =
                                          std::source_location::current()) {
        INFO("Item: " << current << ", at " << loc.file_name() << ":"
                      << loc.line());
        REQUIRE(current < history.size());
        Record actual = std::move(value);

        REQUIRE(format_record(history[current].first) == format_record(actual));
        ++current;
    });
}

TEST_CASE("StressTestBehavioral") {
    CheckBehaveTheSame(
        []<class T>(auto report) { Operate<T>(std::move(report)); });
}
