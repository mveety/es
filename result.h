#ifndef __es_result
#define __es_result

typedef struct Result Result;

struct Result {
	union {
		int64_t i;
		double f;
		char *str;
		void *ptr;
	};
	int status;
};

#endif

