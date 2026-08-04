#include <libapfs/Error.hpp>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <util.hpp>
#include <vector>
#include <verb.hpp>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

int _main(int argc, const std::vector<std::string> &_argv);

void print_error(const std::string &msg) {
  std::cerr << color::red("error: ") << msg << '\n';
}

std::map<std::string, std::string> parse_options(const std::vector<std::string> argv) {
  std::map<std::string, std::string> map;

  for (int i = 0; i < int(argv.size()); ++i) {
    if (!argv[i].starts_with("--")) {
      if (map.contains("_default")) {
        throw apfs::Error("too many arguments passed");
      }
      map["_default"] = argv[i];
    } else {
      if (map.contains(argv[i].substr(2))) {
        throw apfs::Error("option " + argv[i] + " specified more than once");
      }
      if (i == int(argv.size()) - 1 || argv[i + 1].starts_with("--")) {
        throw apfs::Error("missing option: " + argv[i]);
      }
      map[argv[i].substr(2)] = argv[i + 1];
      i++;
    }
  }

  return map;
}

int main(int argc, char **_argv) {
#if defined(_WIN32) || defined(_WIN64)
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  // Copy over command line arguments
  std::vector<std::string> argv(argc);
  for (int i = 0; i < argc; ++i) {
    argv[i] = _argv[i];
  }
  try {
    return _main(argc, argv);
  } catch (const apfs::Error &e) {
    print_error(e.what());
    return 1;
  }
}

struct HelpVerb : Verb {
  HelpVerb() : Verb("help", "Prints this message") {}
  int handler(std::map<std::string, std::string> options) override;
};

int _main(int argc, const std::vector<std::string> &_argv) {
  if (argc == 1) {
    HelpVerb help;
    help.handler({});
    return 0;
  }

  for (auto &verb : verbs()) {
    if (_argv[1] == verb->name) {
      std::vector<std::string> argv(argc - 2);
      for (int i = 2; i < argc; ++i) {
        argv[i - 2] = _argv[i];
      }
      return verb->handler(parse_options(argv));
    }
  }

  throw apfs::Error("no matching verb");
}