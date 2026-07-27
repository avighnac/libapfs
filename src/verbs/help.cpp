#include <iostream>
#include <map>
#include <string>
#include <util.hpp>
#include <vector>
#include <verb.hpp>

struct HelpVerb : Verb {
  HelpVerb() : Verb("help", "Prints this message") {}

  int handler(std::map<std::string, std::string> options) override {
    if (options.empty()) {
      std::cout << color::white("Usage") << '\n';
      std::cout << "  " << color::white("apfs") << color::dim(" <verb> --option1 <option1> --option2 <option2> <filename>") << "\n\n";

      size_t max_len = (*std::max_element(verbs().begin(), verbs().end(), [](auto &a, auto &b) { return a->name.length() < b->name.length(); }))->name.length() + 2;

      std::cout << color::white("Verbs") << '\n';
      for (auto &verb : verbs()) {
        std::cout << color::dim(verb == verbs().back() ? "└─ " : "├─ ") << color::bold(verb->name)
                  << std::string(max_len - verb->name.length(), ' ') << verb->description << '\n';
      }

      std::cout << '\n';
      std::cout << "To find out which options are required for a given " << color::white("verb") << ", run " << color::white("apfs help") << color::dim(" <verb>") << ".\n";
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
      std::cout << color::white("_default: ") << "verb " << color::red("required") << '\n';
    } else if (verb == "diskinfo") {
      std::cout << color::white("_default: ") << "filename " << color::red("required") << '\n';
    } else if (verb == "cat") {
      std::cout << color::white("_default: ") << "disk file " << color::red("required") << '\n';
      std::cout << color::white("volume: ") << "volume name " << color::yellow("maybe deducible") << '\n';
      std::cout << color::white("path: ") << "file path " << color::red("required") << '\n';
      std::cout << color::white("part: ") << "partition guid " << color::yellow("maybe deducible") << '\n';
    } else if (verb == "ls") {
      std::cout << color::white("_default: ") << "disk file " << color::red("required") << '\n';
      std::cout << color::white("volume: ") << "volume name " << color::yellow("maybe deducible") << '\n';
      std::cout << color::white("path: ") << "directory path " << color::red("required") << '\n';
      std::cout << color::white("part: ") << "partition guid " << color::yellow("maybe deducible") << '\n';
    } else if (verb == "info") {
      std::cout << color::white("_default: ") << "filename " << color::red("required") << '\n';
      std::cout << color::white("part: ") << "partition guid " << color::yellow("maybe deducible") << '\n';
    } else {
      std::cout << "No help found for " << verb << '\n';
    }
    return 0;
  }
};

REGISTER_VERB(HelpVerb);