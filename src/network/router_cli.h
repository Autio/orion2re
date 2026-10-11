#pragma once

namespace network::router {

    bool Is_Router_CLI_Command(int argc, char** argv);
    int Run_Router_CLI(int argc, char** argv);

} // namespace network::router