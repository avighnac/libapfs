#include <Error.hpp>
#include <checksum.hpp>
#include <iostream>
#include <string>
#include <types.hpp>
#include <vector>
#include <util.hpp>

std::vector<std::string> verbs = {"info"};
std::vector<std::string> descriptions = {"prints information about the container"};

void print_help() {
  std::cout << color::white("Usage") << '\n';
  std::cout << color::dim("  apfs <verb> <filename>") << "\n\n";

  std::cout << color::white("Verbs") << '\n';
  std::cout << color::dim("  └─ ") << color::bold("info") << "  Prints information about the container\n";
}

void print_error(const std::string &msg) {
  std::cout << color::red("error: ") << msg << '\n';
}

int _main(int argc, const std::vector<std::string> &argv);
int verb_info(int argc, const std::vector<std::string> &argv);

int main(int argc, char **_argv) {
  // Copy over command line arguments
  std::vector<std::string> argv(argc);
  for (int i = 0; i < argc; ++i) {
    argv[i] = _argv[i];
  }
  try {
    return _main(argc, argv);
  } catch (const Error &e) {
    print_error(e.what());
    return 1;
  }
}

int _main(int argc, const std::vector<std::string> &argv) {
  if (argc == 1) {
    print_help();
    return 0;
  }
  if (argc != 3) {
    throw Error("insufficient (or too many) arguments passed");
  }

  if (argv[1] == "info") {
    return verb_info(argc, argv);
  } else {
    throw Error("no matching verb");
  }
}