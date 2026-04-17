#include "es.h"
#include "prim.h"

PRIM(add) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if(list == nil || list->next == nil)
		fail("$&add", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&add", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&add", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&add", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&add", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a + b;
	return mklist(mkstr(str("%ld", res)), nil);
}

PRIM(sub) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if(list == nil || list->next == nil)
		fail("$&sub", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&sub", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&sub", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&sub", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&sub", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a - b;
	return mklist(mkstr(str("%ld", res)), nil);
}

PRIM(mul) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if(list == nil || list->next == nil)
		fail("$&mul", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&mul", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&mul", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&mul", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&mul", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a * b;
	return mklist(mkstr(str("%ld", res)), nil);
}

PRIM(div) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if(list == nil || list->next == nil)
		fail("$&div", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&div", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&div", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&div", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&div", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if(b == 0)
		fail("$&div", "divide by zero");
	res = a / b;
	return mklist(mkstr(str("%ld", res)), nil);
}

PRIM(mod) {
	int64_t a = 0;
	int64_t b = 0;
	int64_t res = 0;

	if(list == nil || list->next == nil)
		fail("$&mod", "missing arguments");
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&mod", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&mod", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&mod", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&mod", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if(b == 0)
		fail("$&mod", "divide by zero");
	res = a % b;
	return mklist(mkstr(str("%ld", res)), nil);
}

PRIM(eq) {
	int64_t a = 0;
	int64_t b = 0;

	if(list == nil)
		return list_false;
	if(list->next == nil)
		return list_false;
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	if(a == b)
		return list_true;
	return list_false;
}

PRIM(gt) {
	int64_t a = 0;
	int64_t b = 0;

	if(list == nil)
		return list_false;
	if(list->next == nil)
		return list_false;
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	if(a > b)
		return list_true;
	return list_false;
}

PRIM(lt) {
	int64_t a = 0;
	int64_t b = 0;

	if(list == nil)
		return list_false;
	if(list->next == nil)
		return list_false;
	errno = 0;

	a = strtoll(getstr(list->term), nil, 10);
	if(a == 0) {
		switch(errno) {
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	b = strtoll(getstr(list->next->term), nil, 10);
	if(b == 0) {
		switch(errno) {
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	if(a < b)
		return list_true;
	return list_false;
}

PRIM(tobase) {
	int64_t base, num;
	char *s, *se;
	List *res = nil; Root r_res;

	if(list == nil || list->next == nil)
		fail("$&tobase", "missing arguments");
	errno = 0;

	base = strtoll(getstr(list->term), nil, 10);
	if(base == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&tobase", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&tobase", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	num = strtoll(getstr(list->next->term), nil, 10);
	if(num == 0) {
		switch(errno) {
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
	gcref(&r_res, (void **)&res);

	gcdisable();

	if(num == 0) {
		gcenable();
		gcrderef(&r_res);
		return mklist(mkstr(str("0")), nil);
	}

	s = ealloc(256);
	se = &s[254];
	memset(s, 0, 256);

	while(num) {
		*--se = "0123456789abcdef"[num % base];
		num = num / base;
		if(se == s) {
			efree(s);
			gcenable();
			fail("$&tobase", "overflow");
		}
	}

	res = mklist(mkstr(str("%s", se)), nil);
	efree(s);

	gcenable();
	gcrderef(&r_res);
	return res;
}

PRIM(frombase) {
	int64_t base, num;

	if(list == nil || list->next == nil)
		fail("$&frombase", "missing arguments");
	errno = 0;

	base = strtoll(getstr(list->term), nil, 10);
	if(base == 0) {
		switch(errno) {
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

	num = strtoll(getstr(list->next->term), nil, base);
	if(num == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&frombase", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&frombase", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}
	return mklist(mkstr(str("%ld", num)), nil);
}

Primitive prim_math[] = {
	DX(add), DX(sub), DX(mul), DX(div), DX(mod), DX(eq), DX(gt), DX(lt), DX(tobase), DX(frombase),
};

Dict *
initprims_math(Dict *primdict)
{
	size_t i = 0;

	for(i = 0; i < nelem(prim_math); i++)
		primdict = dictput(primdict, prim_math[i].name, (void *)prim_math[i].symbol);

	return primdict;
}
