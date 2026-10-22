// A stand-in for oxipng and optipng, built once under each name (see tests/tst_optimizer), driven by environment
// variables:
//   OPTIMIZER_STUB_LOG    file to which each run appends "start <name> <arguments>" when it starts and "end <input>"
//                         when it exits
//   OPTIMIZER_STUB_MODE   "ok" (default): writes the input plus "<name>-stub" to the output and exits 0
//                         "fail": truncates the output to a few bytes and exits 1
//                         "nooutput": writes nothing and exits 0 (like optipng on an already optimized file)
//                         "hang": never exits (until killed)
//   OPTIMIZER_STUB_SLEEP  milliseconds to wait before working
// The output is the file after "-out" (optipng) or "--out" (oxipng), or the input itself when there is none. Like
// optipng, it refuses to overwrite an existing output file.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace {
void log(const std::string &line)
{
    if (const char *log = std::getenv("OPTIMIZER_STUB_LOG")) {
        std::ofstream(log, std::ios::app | std::ios::binary) << line << "\n";
    }
}

// The executable's name without directory and extension.
std::string toolName(const char *argv0)
{
    std::string name = argv0;
    const auto slash = name.find_last_of("/\\");

    if (slash != std::string::npos) {
        name = name.substr(slash + 1);
    }

    const auto dot = name.find('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}
}

int main(int argc, char *argv[])
{
    const std::string name = toolName(argv[0]);
    std::string input;
    std::string output;
    std::string arguments;

    for (int i = 1; i < argc; ++i) {
        arguments += std::string(" ") + argv[i];

        if ((std::strcmp(argv[i], "-out") == 0 || std::strcmp(argv[i], "--out") == 0) && i + 1 < argc) {
            output = argv[++i];
            arguments += std::string(" ") + argv[i];
        } else if (argv[i][0] != '-') {
            input = argv[i];
        }
    }

    log("start " + name + arguments);

    struct End {
        std::string input;
        ~End() { log("end " + input); }
    } end{ input };

    if (const char *sleep = std::getenv("OPTIMIZER_STUB_SLEEP")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(sleep)));
    }

    if (input.empty()) {
        return 2;
    }

    if (output.empty()) {
        output = input;
    } else if (std::ifstream(output).good()) {
        std::fprintf(stderr, "Error: The output file exists\n");
        return 1;
    }

    const char *modeEnv = std::getenv("OPTIMIZER_STUB_MODE");
    const std::string mode = modeEnv ? modeEnv : "ok";

    if (mode == "hang") {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    if (mode == "nooutput") {
        return 0;
    }

    if (mode == "fail") {
        std::ofstream(output, std::ios::binary | std::ios::trunc) << "partial";
        return 1;
    }

    std::ifstream in(input, std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    std::ofstream out(output, std::ios::binary | std::ios::trunc);
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    out << name << "-stub";
    return out.good() ? 0 : 1;
}
