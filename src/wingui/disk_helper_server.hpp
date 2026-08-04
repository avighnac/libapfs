#pragma once

// Entry point for the elevated disk-helper process. Launched by
// disk_helper_client::start() (see disk_helper_client.hpp) via a UAC
// prompt, this is what actually opens \\.\PhysicalDriveN paths --
// something a non-admin process on Windows can't do, even for read-only
// access -- and hands the resulting handles off to whichever unprivileged
// process asked for one (the GUI itself, or a mount helper -- see
// mount_daemon.cpp).
//
// The GUI process (see main.cpp) recognizes this mode via a "--disk-helper"
// argument, the same way it recognizes a mount-helper invocation -- one exe
// serves three roles depending on argv.
namespace disk_helper_server {

// argv[1] is "--disk-helper", argv[2] the pipe name to serve, argv[3] the
// PID of the process that launched us (watched so we exit the moment it
// does, however that happens). Never returns under normal operation.
int run(int argc, char **argv);

} // namespace disk_helper_server
