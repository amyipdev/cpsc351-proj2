// NOTE: the requirements do not require that we handle when a string starts
// with a non-alphanumeric character. As such, that is not currently handled.
// However, it would be fairly trivial to implement with a check in both functions.

#include <iostream>
#include <atomic>
#include <vector>
#include <string>
#include <cctype>
#include <pthread.h>

#ifndef __GNUG__
	#ifndef __clang__
		#error "Must compile with G++/GNU C++, Clang(++), or equivalent"
	#endif
#endif

#ifndef __unix__
	#error "Target system must be Unix"
#endif

#ifndef __x86_64__
	#error "Must be x86_64 (amd64) compile target"
#endif

// Function prototypes; actual definition below
void *alpha(void *);
void *numeric(void *);

static std::vector<std::string> *vec;
static size_t cpos;
// ONLY USE WITH LOCK BTS
static uint32_t bts = 0;

int main(int argc, char **argv) {
	if (argc <= 1) {
		std::cout << "Error: must pass at least 1 argument" << std::endl;
		return 1;
	}

	std::string inp(argv[1]);
	vec = new std::vector<std::string>;

	// Split the string
	size_t _np = inp.find(' ');
	size_t _ip = 0;
	while (_np != std::string::npos) {
		vec->push_back(inp.substr(_ip, _np-_ip));
		_ip = _np + 1;
		_np = inp.find(' ', _ip);
	}
	vec->push_back(inp.substr(_ip, std::min(_np, inp.size())-_ip+1));

	// Make sure bts starts at zero
	bts = 0;

	// Make the alpha thread
	pthread_t t_alpha;
	if (pthread_create(&t_alpha, NULL, &alpha, NULL)) {
		std::cout << "Error: failed to make alpha thread" << std::endl;
		return 2;
	}

	// Make the numeric thread
	pthread_t t_numeric;
	if (pthread_create(&t_numeric, NULL, &numeric, NULL)) {
		std::cout << "Error: failed to make numeric thread" << std::endl;
		return 2;
	}

	// Wait for threads to finish
	pthread_join(t_alpha, NULL);
	pthread_join(t_numeric, NULL);

	// Clean up and return
	delete vec;
	return 0;
}

inline void spinlock_set() {
	bool res;
	while (true) {
		res = 0;
		asm volatile (
			"lock btsl $0,%0\n\t"
			"adc $0,%1"
			: "+m" (bts), "+r" (res)
			:
		);
		if (res == 0) {
			return;
		}
	}
}

inline void spinlock_clear() {
	bts = 0;
	// If we spend some time busy-waiting performance can increase slightly
	// For our use-case though this is unnecessary
	/*asm volatile (
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop"
	);*/
}

void *alpha(void *_a) {
	while (true) {
		spinlock_set();
		if (cpos >= vec->size()) {
			spinlock_clear();
			return nullptr;
		}
		if (!isalpha((int)(vec->at(cpos)[0]))) {
			spinlock_clear();
			continue;
		}
		std::cout << "alpha: " << (*vec)[cpos] << std::endl;
		++cpos;
		spinlock_clear();
	}
}

void *numeric(void *_a) {
	while (true) {
		spinlock_set();
		if (cpos >= vec->size()) {
			spinlock_clear();
			return nullptr;
		}
		if (!isdigit((int)(vec->at(cpos)[0]))) {
			spinlock_clear();
			continue;
		}
		std::cout << "numeric: " << (*vec)[cpos] << std::endl;
		++cpos;
		spinlock_clear();
	}
}
