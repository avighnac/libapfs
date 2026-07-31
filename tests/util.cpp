#include "util.hpp"

#include <cassert>
#include <errno.h>
#include <filesystem>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static std::string find_root_dir() {
  auto p = std::filesystem::current_path();

  while (true) {
    if (std::filesystem::is_regular_file(p / "WATCHME.gif")) {
      return p.string();
    }
    if (p == p.root_path()) {
      break;
    }
    p = p.parent_path();
  }

  assert(false && "Could not find project root");
  return {};
}

std::string path(const std::string &relpath) {
  static const std::string root_dir = find_root_dir();
  return root_dir + relpath;
}

void trim_end(std::string &str) {
  while (!str.empty() && isspace(str.back())) {
    str.pop_back();
  }
}

int exec(const std::string &command, std::string &data) {
  std::vector<char *> argv;
  std::string word;
  auto handle = [&]() {
    if (!word.empty()) {
      argv.push_back((char *)malloc(word.length() + 1));
      memcpy(argv.back(), word.c_str(), word.length());
      argv.back()[word.length()] = 0;
      word.clear();
    }
  };
  bool in_quotes = false;
  for (auto &c : command) {
    if (c == '"') {
      if (word.empty() || word.back() != '\\') {
        in_quotes = !in_quotes;
        if (!word.empty()) {
          word.pop_back();
        }
      }
    }
    if (!in_quotes && c == ' ') {
      handle();
      continue;
    }
    if (!(in_quotes && c == '"')) {
      word.push_back(c);
    }
  }
  handle();
  argv.push_back(0);

  int stat;
  int fds[2];
  pipe(fds);
  pid_t pid = fork();

  if (pid == 0) {
    // child code
    close(fds[0]);
    dup2(fds[1], STDOUT_FILENO);
    execvp(argv[0], argv.data());
  } else {
    // parent code
    close(fds[1]);
    char buf[4097] = {0};
    while (read(fds[0], buf, 4096) > 0) {
      data += buf;
      memset(buf, 0, 4097);
    }
    waitpid(pid, &stat, 0);
  }

  for (int i = 0; i < int(argv.size()) - 1; ++i) {
    free(argv[i]);
  }

  return WEXITSTATUS(stat);
}