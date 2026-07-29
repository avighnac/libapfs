#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <util.hpp>
#include <libapfs/Error.hpp>
#include <util.hpp>
#include <vector>
#include <verb.hpp>

struct HelpVerb : Verb {
  HelpVerb() : Verb("help", "Prints this message") {}

  int handler(std::map<std::string, std::string> options) override {
    if (options.empty()) {
      std::cout << color::bold("Usage") << '\n';
      std::cout << "  " << color::bold("apfs") << color::dim(" <verb> --option1 <option1> --option2 <option2> <filename>") << "\n\n";

      size_t max_len = (*std::max_element(verbs().begin(), verbs().end(), [](auto &a, auto &b) { return a->name.length() < b->name.length(); }))->name.length() + 2;

      std::cout << color::bold("Verbs") << '\n';
      for (auto &verb : verbs()) {
        std::cout << color::dim(verb == verbs().back() ? "└─ " : "├─ ") << color::bold(verb->name)
                  << std::string(max_len - verb->name.length(), ' ') << verb->description << '\n';
      }

      std::cout << '\n';
      std::cout << "To find out which options are required for a given " << color::bold("verb") << ", run " << color::bold("apfs help") << color::dim(" <verb>") << ".\n";
      return 0;
    }
    if (!options.contains("_default")) {
      throw Error("missing verb");
    }
    std::string verb = options["_default"];
    bool found = false;
    for (auto &Verb : verbs()) {
      found |= Verb->name == verb;
    }
    if (!found) {
      throw Error(verb + " is not a verb");
    }

    if (verb == "help") {
      std::cout << color::bold("_default: ") << "verb " << color::red("required") << '\n';
    } else if (verb == "diskinfo") {
      std::cout << color::bold("_default: ") << "filename " << color::red("required") << '\n';
    } else if (verb == "cat") {
      std::cout << color::bold("_default: ") << "disk file " << color::red("required") << '\n';
      std::cout << color::bold("volume: ") << "volume name " << color::yellow("maybe deducible") << '\n';
      std::cout << color::bold("path: ") << "file path " << color::red("required") << '\n';
      std::cout << color::bold("part: ") << "partition guid " << color::yellow("maybe deducible") << '\n';
    } else if (verb == "ls") {
      std::cout << color::bold("_default: ") << "disk file " << color::red("required") << '\n';
      std::cout << color::bold("volume: ") << "volume name " << color::yellow("maybe deducible") << '\n';
      std::cout << color::bold("path: ") << "directory path " << color::red("required") << '\n';
      std::cout << color::bold("part: ") << "partition guid " << color::yellow("maybe deducible") << '\n';
    } else if (verb == "info") {
      std::cout << color::bold("_default: ") << "filename " << color::red("required") << '\n';
      std::cout << color::bold("part: ") << "partition guid " << color::yellow("maybe deducible") << '\n';
    } else {
      std::cout << "No help found for " << verb << '\n';
    }
    return 0;
  }
};

REGISTER_VERB(HelpVerb);