#ifndef RUNGUARD_HPP
#define RUNGUARD_HPP

#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sstream>
#include <thread>
#include <vector>
#include <string.h>
#include "logging.hpp"

namespace utils {
    const static auto logger = logging::create_logger("runguard");

    class RunGuard {
        unsigned time_limit_secs;
        unsigned memory_limit_mb;

    public:
        int status = EXIT_FAILURE;
        std::string message;

        RunGuard(const unsigned time_limit_secs, const unsigned memory_limit_mb)
            : time_limit_secs(time_limit_secs), memory_limit_mb(memory_limit_mb) {}

        void run(const char* command, const std::string& input, std::stringstream& output) {
            int pipe_in[2];
            int pipe_out[2];

            if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
                logger->error("Failed to create pipes");
                return;
            }

            const pid_t child_pid = fork();

            if (child_pid == 0) {
                close(pipe_in[1]);
                close(pipe_out[0]);

                dup2(pipe_in[0], STDIN_FILENO);
                dup2(pipe_out[1], STDOUT_FILENO);
                close(pipe_in[0]);
                close(pipe_out[1]);

                std::vector<char*> args;
                std::istringstream command_stream(command);
                std::string arg;
                while (command_stream >> arg) {
                    args.push_back(strdup(arg.c_str()));
                }
                args.push_back(nullptr);

                if (memory_limit_mb) {
                    struct rlimit mem_limit{};
                    mem_limit.rlim_cur = memory_limit_mb * 1048576;
                    mem_limit.rlim_max = memory_limit_mb * 1048576;
                    setrlimit(RLIMIT_AS, &mem_limit);
                }
                if (time_limit_secs) {
                    struct rlimit cpu_limit{};
                    cpu_limit.rlim_cur = time_limit_secs;
                    cpu_limit.rlim_max = time_limit_secs;
                    setrlimit(RLIMIT_CPU, &cpu_limit);
                    alarm(time_limit_secs);
                }

                execvp(args[0], args.data());
                exit(EXIT_FAILURE);
            }

            if (child_pid > 0) {
                close(pipe_in[0]);
                close(pipe_out[1]);


                std::thread input_thread([&] {
                    (void) write(pipe_in[1], input.c_str(), input.size());
                    close(pipe_in[1]);
                });


                std::thread output_thread([&] {
                    char buffer[256];
                    ssize_t count;
                    while ((count = read(pipe_out[0], buffer, sizeof(buffer) - 1)) > 0) {
                        buffer[count] = '\0';
                        output << buffer;
                    }
                    close(pipe_out[0]);
                });

                input_thread.join();
                output_thread.join();

                waitpid(child_pid, &status, 0);

                if (WIFEXITED(status)) message = "Process exited with code " + std::to_string(WEXITSTATUS(status));
                else if (WIFSIGNALED(status)) message = "Process terminated by signal " + std::to_string(WTERMSIG(status));
            }
            else {
                message = "Failed to fork process";
            }

            if (status != EXIT_SUCCESS) logger->error(message);
        }
    };
}

#endif
