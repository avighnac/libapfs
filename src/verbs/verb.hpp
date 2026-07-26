#pragma once

#include <Apfs.hpp>
#include <string>
#include <memory>

struct Verb {
  const std::string name;
  const std::string description;

  Verb(std::string name, std::string description)
    : name(std::move(name)), description(std::move(description)) {}

  virtual int handler(Apfs &apfs, const std::vector<std::string> &args) = 0;
  virtual ~Verb() = default;
};

std::vector<std::unique_ptr<Verb>> &verbs();

template <typename T>
struct RegisterVerb {
  RegisterVerb() { verbs().push_back(std::make_unique<T>()); }
};

#define REGISTER_VERB(T) static RegisterVerb<T> _register_##T;