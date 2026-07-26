#include <Apfs.hpp>
#include <Error.hpp>
#include <checksum.hpp>
#include <iostream>
#include <string>
#include <types.hpp>
#include <vector>
#include <verb.hpp>
#include <util.hpp>

int _main(int argc, char **argv);

void print_help() {
  std::cout << color::white("Usage") << '\n';
  std::cout << color::dim("  apfs <verb> <filename>") << "\n\n";

  std::cout << color::white("Verbs") << '\n';
  for (auto &verb : verbs()) {
    std::cout << color::dim("  └─ ") << color::bold(verb->name) << "  " << verb->description << '\n';
  }
}

void print_error(const std::string &msg) {
  std::cout << color::red("error: ") << msg << '\n';
}

int main(int argc, char **argv) {
  // Copy over command line arguments
  try {
    return _main(argc, argv);
  } catch (const Error &e) {
    print_error(e.what());
    return 1;
  }
}

int _main(int argc, char **_argv) {
  if (argc == 1) {
    print_help();
    return 0;
  }
  if (argc < 3) {
    throw Error("insufficient arguments passed");
  }

  for (auto &verb : verbs()) {
    if (_argv[1] == verb->name) {
      Apfs apfs(_argv[2]);
      std::vector<std::string> argv(argc - 3);
      for (int i = 3; i < argc; ++i) {
        argv[i - 3] = _argv[i];
      }
      return verb->handler(apfs, argv);
    }
  }

  throw Error("no matching verb");
}