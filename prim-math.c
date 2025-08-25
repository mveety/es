#include <stdlib.h>
#include "es.h"
#include "prim.h"

PRIM(add) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&add", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&add", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&add", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&add", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&add", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a + b;
	return mklist(mkstr(str("%ld", res)), NULL);
}

PRIM(sub) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&sub", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&sub", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&sub", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&sub", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&sub", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a - b;
	return mklist(mkstr(str("%ld", res)), NULL);
}

PRIM(mul) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&mul", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&mul", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&mul", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&mul", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&mul", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a * b;
	return mklist(mkstr(str("%ld", res)), NULL);
}

PRIM(div) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&div", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&div", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&div", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&div", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&div", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if (b == 0)
		fail("$&div", "divide by zero");
	res = a / b;
	return mklist(mkstr(str("%ld", res)), NULL);
}

PRIM(mod) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&mod", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&mod", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&mod", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&mod", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&mod", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if (b == 0)
		fail("$&mod", "divide by zero");
	res = a % b;
	return mklist(mkstr(str("%ld", res)), NULL);
}

PRIM(eq) {
	int64_t a = 0;
	int64_t b = 0;

	if(list == NULL)
		return list_false;
	if (list->next == NULL)
		return list_false;
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	if (a == b)
		return list_true;
	return list_false;
}

PRIM(gt) {
	int64_t a = 0;
	int64_t b = 0;

	if (list == NULL)
		return list_false;
	if (list->next == NULL)
		return list_false;
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	if (a > b)
		return list_true;
	return list_false;
}

PRIM(lt) {
	int64_t a = 0;
	int64_t b = 0;

	if (list == NULL)
		return list_false;
	if (list->next == NULL)
		return list_false;
	errno = 0;

	a = strtoll(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	} 

	b = strtoll(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	} 

	if (a < b)
		return list_true;
	return list_false;
}

PRIM(tobase) {
	int64_t base, num;
	char *s, *se;
	List *res = NULL; Root r_res;

	if(list == NULL || list->next == NULL)
		fail("$&tobase", "missing arguments");
	errno = 0;

	base = strtoll(getstr(list->term), NULL, 10);
	if(base == 0){
		switch(errno){
		case EINVAL:
			fail("$&tobase", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&tobase", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}

	}

	num = strtoll(getstr(list->next->term), NULL, 10);
	if(num == 0){
		switch(errno){
		case EINVAL:
			fail("$&tobase", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&tobase", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if(base < 2)
		fail("$&tobase", "base < 2");
	if(base > 16)
		fail("$&tobase", "base > 16");

	res = list;
	gcref(&r_res, (void**)&res);

	gcdisable();

	if(num == 0){
		gcenable();
		gcrderef(&r_res);
		return mklist(mkstr(str("0")), NULL);
	}

	s = ealloc(256);
	se = &s[254];
	memset(s, 0, 256);

	while(num) {
		*--se = "0123456789abcdef"[num%base];
		num = num/base;
		if(se == s) {
			free(s);
			gcenable();
			fail("$&tobase", "overflow");
		}
	}

	res = mklist(mkstr(str("%s", se)), NULL);
	free(s);

	gcenable();
	gcrderef(&r_res);
	return res;
}

PRIM(frombase) {
	int64_t base, num;

	if(list == NULL || list->next == NULL)
		fail("$&frombase", "missing arguments");
	errno = 0;

	base = strtoll(getstr(list->term), NULL, 10);
	if(base == 0){
		switch(errno){
		case EINVAL:
			fail("$&frombase", str("invalid input: $1 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&frombase", str("conversion overflow: $1 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if(base < 2)
		fail("&frombase", "base < 2");
	if(base > 16)
		fail("&frombase", "base > 16");

	num = strtoll(getstr(list->next->term), NULL, base);
	if(num == 0){
		switch(errno){
		case EINVAL:
			fail("$&frombase", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&frombase", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}
	return mklist(mkstr(str("%ld", num)), NULL);
}

Dict*
initprims_math(Dict *primdict)
{
	X(add);
	X(sub);
	X(mul);
	X(div);
	X(mod);
	X(eq);
	X(gt);
	X(lt);
	X(tobase);
	X(frombase);

	return primdict;
}

