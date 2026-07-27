#include <Apfs.hpp>
#include <Error.hpp>
#include <checksum.hpp>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <types.hpp>
#include <util.hpp>
#include <vector>
#include <verb.hpp>

int _main(int argc, char **argv);

void print_error(const std::string &msg) {
  std::cout << color::red("error: ") << msg << '\n';
}

std::map<std::string, std::string> parse_options(const std::vector<std::string> argv) {
  std::map<std::string, std::string> map;

  for (int i = 0; i < int(argv.size()); ++i) {
    if (!argv[i].starts_with("--")) {
      if (map.contains("_default")) {
        throw Error("too many arguments passed");
      }
      map["_default"] = argv[i];
    } else {
      if (map.contains(argv[i].substr(2))) {
        throw Error("option " + argv[i] + " specified more than once");
      }
      if (i == int(argv.size()) - 1 || argv[i + 1].starts_with("--")) {
        throw Error("missing option: " + argv[i]);
      }
      map[argv[i].substr(2)] = argv[i + 1];
      i++;
    }
  }

  return map;
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

struct HelpVerb : Verb {
  HelpVerb();
  int handler(std::map<std::string, std::string> options) override;
};

int _main(int argc, char **_argv) {
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

  throw Error("no matching verb");
}