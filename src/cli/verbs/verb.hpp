#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

/// @brief A generic verb for the command-line interface.
/// To find out exactly which options are required by each verb, use the `help` verb.
struct Verb {
  const std::string name;
  const std::string description;

  Verb(std::string name, std::string description) : name(std::move(name)), description(std::move(description)) {}

  virtual int handler(std::map<std::string, std::string> options) = 0;
  virtual ~Verb() = default;
};

std::vector<std::unique_ptr<Verb>> &verbs();

template <typename T>
struct RegisterVerb {
  RegisterVerb() { verbs().push_back(std::make_unique<T>()); }
};

#define REGISTER_VERB(T) static RegisterVerb<T> _register_##T;