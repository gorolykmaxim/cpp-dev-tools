#include <iostream>
#include "process.hpp"
#include "json.hpp"

int main(int argc, const char** argv) {
	nlohmann::json message = {{"msg", "Hello World from JSON!"}};
	TinyProcessLib::Process process("echo " + message["msg"].get<std::string>());
	return process.get_exit_status();
}