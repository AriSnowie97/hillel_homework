#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <cctype>
#include <algorithm>
#include <source_location>
#include <sstream> 
#include <iomanip>

using namespace std;
struct LogSink {
    virtual void write(const string& msg) = 0;
    virtual ~LogSink() = default;
};
struct ConsoleSink : public LogSink {
    void write(const string& msg) override {
        cout << msg << endl;
    }
};
struct FileSink : public LogSink {
    FileSink() : file_("app.log", ios::app), file_open_(file_.is_open()) {
        if (!file_open_) {
            cerr << "Error opening file app.log for writing." << endl;
        }
    }
    void write(const string& msg) override {
        if (file_open_) {
            file_ << msg << endl;
            if (file_.fail()) {
                cerr << "Error writing to file app.log." << endl;
            }
        }
        else {
            cerr << "Error: File app.log is not open." << endl;
        }
    }
private:
    ofstream file_;
    bool file_open_;
};

struct NullSink : public LogSink {
    void write(const string& /*msg*/) override {}
};

enum class SinkType { CONSOLE, FILE, NONE };

class Logger {
public:
    static Logger& instance() {
        static Logger instance_;
        return instance_;
    }

    void set_sink(SinkType type) {
        switch (type) {
        case SinkType::CONSOLE:
            sink_ = make_unique<ConsoleSink>();
            current_sink_type_ = SinkType::CONSOLE;
            cout << "Logging redirected to console." << endl;
            break;
        case SinkType::FILE:
            sink_ = make_unique<FileSink>();
            current_sink_type_ = SinkType::FILE;
            cout << "Logging redirected to file app.log." << endl;
            break;
        case SinkType::NONE:
            sink_ = make_unique<NullSink>();
            current_sink_type_ = SinkType::NONE;
            cout << "Logging disabled." << endl;
            break;
        default:
            cerr << "Unknown sink type. Previous sink remains." << endl;
            break;
        }
    }

    void log(const string& msg, const source_location& location = source_location::current()) {
        if (sink_) {
            sink_->write(formatLogMessage(msg, location));
        }
        else {
            cerr << "Error: Sink not set." << endl;
        }
    }

    SinkType get_current_sink_type() const {
        return current_sink_type_;
    }

private:
    unique_ptr<LogSink> sink_;
    SinkType current_sink_type_ = SinkType::CONSOLE; // Default to console
    Logger() : sink_(make_unique<ConsoleSink>()) {}
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    string formatLogMessage(const string& msg, const source_location& location) {
        stringstream ss;
        ss << "[" << location.file_name() << ":" << location.function_name() << ":" << location.line() << "] " << msg;
        return ss.str();
    }
};

SinkType parseSinkType(const string& arg) {
    string lower_arg = arg;
    transform(lower_arg.begin(), lower_arg.end(), lower_arg.begin(), ::tolower);
    if (lower_arg == "console") {
        return SinkType::CONSOLE;
    }
    else if (lower_arg == "file") {
        return SinkType::FILE;
    }
    else if (lower_arg == "none") {
        return SinkType::NONE;
    }
    else {
        return SinkType::CONSOLE;
    }
}

int main(int argc, char* argv[]) {
    SinkType sink_type = SinkType::CONSOLE;

    if (argc > 1) {
        sink_type = parseSinkType(argv[1]);
        cout << "Command line argument received: " << argv[1] << endl;
    }
    else {
        cout << "No command line argument provided. Using default console output." << endl;
    }

    Logger::instance().set_sink(sink_type);
    Logger::instance().log("First test message.");
    Logger::instance().log("Second test message.");
    Logger::instance().set_sink(SinkType::FILE);
    Logger::instance().log("Message to file.");
    Logger::instance().set_sink(SinkType::NONE);
    Logger::instance().log("This message should go nowhere.");
    Logger::instance().set_sink(SinkType::CONSOLE);
    Logger::instance().log("Back to console output.");

    cout << "Program finished." << endl;

    return 0;
}
