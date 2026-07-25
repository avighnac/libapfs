#include <Error.hpp>
#include <checksum.hpp>
#include <iostream>
#include <string>
#include <types.hpp>
#include <vector>

std::vector<std::string> verbs = {"test"};
std::vector<std::string> descriptions = {"prints information about the container"};

void print_help() {
  std::cout << "Usage: apfs <verb> <filename>\n";
  std::cout << "Verbs:\n";
  for (int i = 0; i < int(verbs.size()); ++i) {
    std::cout << "  " << verbs[i] << ": " << descriptions[i] << '\n';
  }
}

std::string red(const std::string &s) { return "\033[1;31m" + s + "\033[0m"; }

void print_error(const std::string &msg) {
  std::cout << red("error: ") << msg << '\n';
}

int _main(int argc, const std::vector<std::string> &argv);
int verb_test(int argc, const std::vector<std::string> &argv);

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
    throw Error("insufficient arguments passed");
  }

  if (argv[1] == "test") {
    return verb_test(argc, argv);
  } else {
    throw Error("no matching verb");
  }
}