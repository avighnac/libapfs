#include <memory>
#include <verb.hpp>

std::vector<std::unique_ptr<Verb>> &verbs() {
  static std::vector<std::unique_ptr<Verb>> all_verbs;
  return all_verbs;
}