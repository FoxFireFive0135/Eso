#define _CRT_SECURE_NO_WARNINGS // Thanks, Microsoft.

#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <unordered_map>

// My own codes
static constexpr auto SUCCESS = 0;
static constexpr auto NO_FILENAME = 1;
static constexpr auto FILE_NOT_FOUND = 2;
static constexpr auto HALTED = 3;
static constexpr auto ARG_ERROR = 4;

// About 4MB of memory
static constexpr auto MEM_SIZE = 1 << 24;

// Data pointer
static int dp = 0; // You can change this in the command line!

// Memory
static std::vector<int> mem;

// Variables
static std::unordered_map<std::string, int> vars;

// Source code file
static FILE* src = nullptr;

// All of Eso's instructions, counting newlines and spaces too.
static const std::string insts = "`~!@#$%^&*()-=_+[]\\{}|;\':\",./<>?\n\r ";

// I don't like typing this everywhere
static int get(void) {
	return fgetc(src);
}

// I also don't like typing this everywhere.
static void unget(int val) {
	ungetc(val, src);
}

// Is the character an instruction?
static bool is_inst(int c) {
	return insts.find(static_cast<char>(c)) != std::string::npos ? true : false;
}

// Only for the "wait" instruction.
static void sleep(std::string t) {
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(std::stoll(t))));
}

// Only for loops that need to check the next memory address
static int index(int i) {
	if (i >= MEM_SIZE - 1) return MEM_SIZE - 1;
	if (i <= 0) return 0;
	return i;
}

// Get string. Basically the spine of this whole language.
static std::string gets(void) {
	std::string str;
	int val;

	while (true) {
		val = get();

		if (val == EOF) break;
		if (val == '/') break;
		if (is_inst(val)) {
			unget(val);
			break;
		}

		str += static_cast<char>(val);
	}

	return str;
}

// Increment Data Pointer
static void inc_dp(void) {
	if (dp < MEM_SIZE - 1) dp++;
}

// Decrement Data Pointer
static void dec_dp(void) {
	if (dp > 0) dp--;
}

// Entry point for Eso
int main(int argc, char* argv[]) {
EsoStart: // This label is used by the reset(=) command. No clue if this is safe at all.
	std::string src_name;

	// Parse args
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];

		if (arg.empty()) continue;

		if (arg[0] == '!')
		{
			arg.erase(0, 1); // Remove garbage "!"
			src_name = arg;

			if (src_name.empty()) return NO_FILENAME;

			continue;
		}

		else if (arg == "--version" || arg == "-version") {
			printf("Eso %s \n", "1.0.0");
			continue;
		}

		else if (arg == "--dp" || arg == "-dp") {
			i++;
			arg = argv[i];

			if (arg.empty() || arg[0] == '-') {
				i--;
				dp = 0;
				continue;
			}

			dp = std::stoi(arg);
		}

		else return SUCCESS;
	}

	// Open the source code
	src = fopen(src_name.c_str(), "rb");

	if (src == nullptr) {
		printf("File not found: \"%s\"\n", src_name.c_str());
		return FILE_NOT_FOUND;
	}

	// Initialize memory
	mem.resize(MEM_SIZE, 0);

	// Main loop
	while (true) {
		int inst = get();
		if (inst == EOF) break; // If we hit the end of the file, stop running.

		switch (inst) {
		// Declare Variable
		case '`': vars.insert({gets(), std::stoi(gets())}); break;

		// Copy variable into data pointer
		case '~': dp = vars[gets()]; break;

		// Invert arg / memory
		case '!': {
			int val = get();

			if (is_inst(val)) {
				unget(val);
				mem[dp] = ~mem[dp];
			}
			else {
				std::string var;
				var += static_cast<char>(val);
				var += gets();
				vars[var] = ~vars[var];
			}

			break;
		}

		// Insert string into memory. " must follow after this.
		case '@': {
			std::string str;
			int val = get();

			if (val != '\"') {
				unget(val);
				break;
			}

			while (true) {
				val = get();
				if (val == '\"') break;
				str += static_cast<char>(val);
			}

			for (auto c : str) {
				mem[dp] = c;
				inc_dp();
			}

			break;
		}

		// Create file
		case '#': {
			FILE* f = fopen(gets().c_str(), "wb");
			if (f == nullptr) break;
			fclose(f);
			break;
		}

		// Read contents of file into memory
		case '$': {
			FILE* f = fopen(gets().c_str(), "rb");
			if (f == nullptr) break;

			while (true) {
				int c = fgetc(f);
				if (c == EOF) break;
				mem[dp] = c;
				inc_dp();
			}

			fclose(f);
			break;
		}

		// Delete file
		case '%': remove(gets().c_str()); break;

		// Wait
		case '^': sleep(gets()); break;

		// Set data pointer
		case '&': dp = std::stoi(gets()); break;

		// Set memory
		case '*': mem[dp] = std::stoi(gets()); break;

		// If first arg / memory is greater than the second arg, jump to the command after the matching )
		case '(': {
			int depth = 1;
			int val = get();

			if (is_inst(val)) {
				unget(val);

				// Memory
				if (mem[dp] > mem[index(dp + 1)]) {
					while (depth > 0) {
						val = get();

						if (val == '(') depth++;
						else if (val == ')') depth--;
					}
				}
			}

			else {
				// Variable stuff
				std::string var1;
				std::string var2;

				var1 += static_cast<char>(val);
				var1 += gets();
				var2 += gets();

				if (var2.empty() && mem[dp] > vars[var1]) {
					while (depth > 0) {
						val = get();

						if (val == '(') depth++;
						else if (val == ')') depth--;
					}

					gets(); // Used to get to the end of the }
				}
				else if (vars[var1] > vars[var2]) {
					while (depth > 0) {
						val = get();

						if (val == '(') depth++;
						else if (val == ')') depth--;
					}

					gets();
					gets(); // These are used to get to the end of the }
				}
			}

			break;
		}

		// If the first arg / memory is lesser than the second arg, jump back to the command after the matching (
		case ')': {
			int depth = 1;
			int val = get();

			if (is_inst(val)) {
				unget(val);

				// Memory
				if (mem[dp] < mem[index(dp + 1)]) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == ')') depth++;
						else if (val == '(') depth--;
					}
				}
			}

			else {
				// Variable stuff
				std::string var1;
				std::string var2;

				var1 += static_cast<char>(val);
				var1 += gets();
				var2 += gets();

				if (var2.empty() && mem[dp] < vars[var1]) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == ')') depth++;
						else if (val == '(') depth--;
					}

					gets(); // Used to get to the end of the {
				}
				else if (vars[var1] < vars[var2]) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == ')') depth++;
						else if (val == '(') depth--;
					}

					gets();
					gets(); // These are used to get to the end of the {
				}
			}

			break;
		}

		// Decrement Variable
		case '-': {
			int val = get();

			if (is_inst(val)) {
				unget(val);
				mem[dp]--;
			}
			else {
				std::string var_name;

				var_name += static_cast<char>(val);
				var_name += gets();

				vars[var_name]--;
			}

			break;
		}

		// Reset
		case '=': {
			inst = 0;
			dp = 0;
			fclose(src);
			mem.clear();
			vars.clear();
			goto EsoStart;
		}

		// Put value of arg into memory
		case '_': mem[dp] = vars[gets()]; break;

		// Increment arg / memory
		case '+': {
			int val = get();

			if (is_inst(val)) {
				unget(val);
				mem[dp]++;
			}
			else {
				std::string var_name;

				var_name += static_cast<char>(val);
				var_name += gets();

				vars[var_name]++;
			}

			break;
		}

		// If the arg / memory is zero, jump to the command after the matching ]
		case '[': {
			int depth = 1;
			int val = get();

			if (is_inst(val)) {
				unget(val);
				
				// Memory
				if (mem[dp] == 0) {
					while (depth > 0) {
						val = get();

						if (val == '[') depth++;
						else if (val == ']') depth--;
					}
				}
			}

			else {
				// Variable stuff
				std::string var;

				var += static_cast<char>(val);
				var += gets();

				if (vars[var] == 0) {
					while (depth > 0) {
						val = get();

						if (val == '[') depth++;
						else if (val == ']') depth--;
					}

					gets(); // Only used to get to the actual end of the ]
				}
			}

			break;
		}

		// If the arg / memory is nonzero, jump back to the command after the matching [
		case ']': {
			int depth = 1;
			int val = get();

			if (is_inst(val)) {
				unget(val);

				// Memory
				if (mem[dp] != 0) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == ']') depth++;
						else if (val == '[') depth--;
					}
				}
			}

			else {
				// Variable stuff
				std::string var;

				var += static_cast<char>(val);
				var += gets();

				if (vars[var] != 0) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == ']') depth++;
						else if (val == '[') depth--;
					}

					gets(); // To get to the actual start of the loop
				}
			}

			break;
		}

		// Copy data pointer into arg / memory
		case '\\': {
			int val = get();

			if (is_inst(val)) {
				unget(val);
				mem[dp] = dp;
			}
			else {
				std::string var;
				var += static_cast<char>(val);
				var += gets();
				vars[var] = dp;
			}

			break;
		}

		// If the first arg / memory is equal to the second arg, jump to the command after the matching }
		case '{': {
			int depth = 1;
			int val = get();

			if (is_inst(val)) {
				unget(val);

				// Memory
				if (mem[dp] == mem[index(dp + 1)]) {
					while (depth > 0) {
						val = get();

						if (val == '{') depth++;
						else if (val == '}') depth--;
					}
				}
			}

			else {
				// Variable stuff
				std::string var1;
				std::string var2;

				var1 += static_cast<char>(val);
				var1 += gets();
				var2 += gets();

				if (var2.empty() && mem[dp] == vars[var1]) {
					while (depth > 0) {
						val = get();

						if (val == '{') depth++;
						else if (val == '}') depth--;
					}

					gets(); // Used to get to the end of the }
				}
				else if (vars[var1] == vars[var2]) {
					while (depth > 0) {
						val = get();

						if (val == '{') depth++;
						else if (val == '}') depth--;
					}

					gets();
					gets(); // These are used to get to the end of the }
				}
			}

			break;
		}

		// If the first arg / memory is not equal to the second arg, jump back to the command after the matching {
		case '}': {
			int depth = 1;
			int val = get();

			if (is_inst(val)) {
				unget(val);

				// Memory
				if (mem[dp] != mem[index(dp + 1)]) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == '}') depth++;
						else if (val == '{') depth--;
					}
				}
			}

			else {
				// Variable stuff
				std::string var1;
				std::string var2;

				var1 += static_cast<char>(val);
				var1 += gets();
				var2 += gets();

				if (var2.empty() && mem[dp] != vars[var1]) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == '}') depth++;
						else if (val == '{') depth--;
					}

					gets(); // Used to get to the end of the {
				}
				else if (vars[var1] != vars[var2]) {
					while (depth > 0) {
						fseek(src, -2, SEEK_CUR);
						val = get();

						if (val == '}') depth++;
						else if (val == '{') depth--;
					}

					gets();
					gets(); // These are used to get to the end of the {
				}
			}

			break;
		}

		// Loop break
		case '|': {
			int depth = 2; // This is expected to be the last thing in a loop, i.e. { 'code' {'check something'|} 'more code' {'more stuff'} } 'more code'

			while (depth > 0) {
				int val = get();

				if (val == '{' || val == '[' || val == '(') depth++;
				if (val == '}' || val == ']' || val == ')') depth--;
			}

			break;
		}

		// Halt Execution
		case ';': exit(HALTED); break;

		// Comment
		case '\'': {
			int val;

			while (true) {
				val = get();
				if (val == '\'' || val == EOF) break;
			}

			break;
		}

		// Put value of memory into arg
		case ':': vars[gets()] = mem[dp]; break;

		// Accept input into arg / memory
		case ',': {
			int val = get();

			if (is_inst(val)) {
				unget(val);
				mem[dp] = getc(stdin);
			}
			else {
				std::string var_name;

				var_name += static_cast<char>(val);
				var_name += gets();

				vars[var_name] = getc(stdin);
			}

			break;
		}

		// Output arg / memory
		case '.': {
			int val = get();

			if (is_inst(val)) {
				unget(val);
				printf("%c", mem[dp]);
			}
			else {
				std::string var_name;
				var_name += static_cast<char>(val);
				var_name += gets();

				printf("%c", vars[var_name]);
			}

			break;
		}

		// Move data pointer left
		case '<': {
			dec_dp();
			break;
		}

		// Move data pointer right
		case '>': {
			inc_dp();
			break;
		}

		// Write block of memory to file
		case '?': {
			std::string fname = gets();
			int bytes = std::stoi(gets());

			FILE* f = fopen(fname.c_str(), "wb");
			if (f == nullptr) break;

			for (int x = 0; x < bytes; x++) {
				fprintf(f, "%c", mem[dp]);
				inc_dp();
			}

			fclose(f);
			break;
		}

		// Newlines, Spaces, etc.
		default: break;
		}
	}

	// Clean-up everything
	fclose(src);
	mem.clear();
	mem.shrink_to_fit();
	std::vector<int>().swap(mem);
	vars.clear();
	std::unordered_map<std::string, int>().swap(vars);

	return SUCCESS;
}