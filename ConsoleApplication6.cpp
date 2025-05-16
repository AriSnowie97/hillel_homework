#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <functional>
#include <memory>

using namespace std;
class INumberReader {
public:
    virtual ~INumberReader() = default;
    virtual vector<int> read(const string& filename) = 0;
};
class FileReader : public INumberReader {
public:
    vector<int> read(const string& filename) override {
        vector<int> numbers;
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error{ "Could not open file: " + filename };
        }
        string line;
        while (getline(file, line)) {
            stringstream ss{ line };
            string number_str;
            while (ss >> number_str) {
                try {
                    numbers.push_back(stoi(number_str));
                }
                catch (const invalid_argument& e) {
                    cerr << "Warning: Invalid number in file: " << number_str << ". Skipping.\n";
                }
                catch (const out_of_range& e) {
                    cerr << "Warning: Number out of range in file: " << number_str << ". Skipping.\n";
                }
            }
        }
        file.close();
        return numbers;
    }
};
class INumberFilter {
public:
    virtual ~INumberFilter() = default;
    virtual bool keep(int number) const = 0;
};
class EvenNumberFilter : public INumberFilter {
public:
    bool keep(int number) const override {
        return number % 2 == 0;
    }
};
class OddNumberFilter : public INumberFilter {
public:
    bool keep(int number) const override {
        return number % 2 != 0;
    }
};
class GreaterThanFilter : public INumberFilter {
private:
    int threshold_;
public:
    GreaterThanFilter(int threshold) : threshold_(threshold) {}
    bool keep(int number) const override {
        return number > threshold_;
    }
};
class FilterFactory {
private:
    using FilterCreator = function<unique_ptr<INumberFilter>(const string&)>;
    map<string, FilterCreator> creators_;

public:
    FilterFactory() {
        registerFilter("EVEN", [](const string&) { return make_unique<EvenNumberFilter>(); });
        registerFilter("ODD", [](const string&) { return make_unique<OddNumberFilter>(); });
        registerFilter("GT", [](const string& arg) {
            try {
                int threshold = stoi(arg);
                return make_unique<GreaterThanFilter>(threshold);
            }
            catch (const invalid_argument& e) {
                throw invalid_argument{ "Invalid argument for GT filter: " + arg };
            }
            catch (const out_of_range& e) {
                throw out_of_range{ "Argument out of range for GT filter: " + arg };
            }
            });
    }

    void registerFilter(const string& filterName, FilterCreator creator) {
        creators_[filterName] = creator;
    }

    unique_ptr<INumberFilter> createFilter(const string& filterType, const string& filterArg = "") const {
        if (auto it = creators_.find(filterType); it != creators_.end()) {
            return it->second(filterArg);
        }
        throw invalid_argument{ "Unknown filter type: " + filterType };
    }
};
class INumberObserver {
public:
    virtual ~INumberObserver() = default;
    virtual void on_number(int number) = 0;
    virtual void on_finished() = 0;
};


class PrintObserver : public INumberObserver {
public:
    void on_number(int number) override {
        cout << "Read and filtered number: " << number << endl;
    }
    void on_finished() override {
        cout << "Number processing finished.\n";
    }
};


class CountObserver : public INumberObserver {
private:
    int count_ = 0;
public:
    void on_number(int number) override {
        count_++;
    }
    void on_finished() override {
        cout << "Total number of filtered numbers: " << count_ << endl;
    }
};
class NumberProcessor {
private:
    INumberReader& reader_;
    INumberFilter& filter_;
    vector<INumberObserver*> observers_;

public:
    NumberProcessor(INumberReader& reader, INumberFilter& filter, const vector<INumberObserver*>& observers)
        : reader_(reader), filter_(filter), observers_(observers) {
    }

    void run(const string& filename) {
        try {
            vector<int> numbers = reader_.read(filename);
            for (int number : numbers) {
                if (filter_.keep(number)) {
                    notifyObservers(number);
                }
            }
            notifyFinished();
        }
        catch (const runtime_error& e) {
            cerr << "Error during processing: " << e.what() << endl;
        }
    }

private:
    void notifyObservers(int number) {
        for (INumberObserver* observer : observers_) {
            observer->on_number(number);
        }
    }

    void notifyFinished() {
        for (INumberObserver* observer : observers_) {
            observer->on_finished();
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <filter> <file>\n";
        cerr << "Available filters: EVEN, ODD, GT<n>\n";
        return 1;
    }

    string filter_arg = argv[1];
    string filename = argv[2];
    string filter_type;
    string filter_value;

    if (filter_arg.starts_with("GT")) {
        filter_type = "GT";
        filter_value = filter_arg.substr(2);
    }
    else {
        filter_type = filter_arg;
    }

    FilterFactory factory;
    unique_ptr<INumberFilter> filter;

    try {
        filter = factory.createFilter(filter_type, filter_value);
    }
    catch (const invalid_argument& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    catch (const out_of_range& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    FileReader reader;
    PrintObserver print_observer;
    CountObserver count_observer;
    vector<INumberObserver*> observers = { &print_observer, &count_observer };

    NumberProcessor processor(reader, *filter, observers);
    processor.run(filename);

    return 0;
}