// A stand-in for optipng, driven by environment variables (see tests/tst_optimize):
//   OPTIPNG_STUB_LOG   file to which each run appends its arguments, one line per run
//   OPTIPNG_STUB_MODE  "ok" (default): writes the input plus a trailer to the output and exits 0
//                      "fail": truncates the output to a few bytes and exits 1
//                      "nooutput": writes nothing and exits 0 (like optipng on an already optimized file)
// The output is the file after "-out", or the input itself when there is none (optipng's in-place rewrite).
// Like optipng, it refuses to overwrite an existing "-out" file.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char *argv[])
{
    std::string input;
    std::string output;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-out") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (argv[i][0] != '-') {
            input = argv[i];
        }
    }

    if (const char *log = std::getenv("OPTIPNG_STUB_LOG")) {
        std::ofstream stream(log, std::ios::app | std::ios::binary);

        for (int i = 1; i < argc; ++i) {
            stream << (i > 1 ? " " : "") << argv[i];
        }

        stream << "\n";
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

    const char *modeEnv = std::getenv("OPTIPNG_STUB_MODE");
    const std::string mode = modeEnv ? modeEnv : "ok";

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
    out << "optipng-stub";
    return out.good() ? 0 : 1;
}
