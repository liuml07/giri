#include <stdio.h>

const char* get_hwtype (const char *name) {
	return name;
}

int main(int argc, char **argv) {
	const char *hw = NULL;
	const char *ap = NULL;
	int hw_set = 0;
	switch (*argv[1]) {
		case 'A': case 'p': {
			ap = argv[2];
			break;
		}
		case 'H': case 't': {
			hw = get_hwtype(argv[3]);
			hw_set = 1;
			break;
		}
		default : break;
	}
	if (hw_set && *argv[1] != 'H')
		hw = get_hwtype ("DFLT_HW");
	printf("%s %s\n", ap, hw);

    return 0;
}
