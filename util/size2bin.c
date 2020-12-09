#include <byteswap.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv) {

	if (argc != 3) {
		printf("Usage: %s file size.bin\n", argv[0]);
		return 1;
	}

	struct stat st;
	if (stat(argv[1], &st)) {
		puts("Can't stat");
		return 1;
	}

	uint32_t size = st.st_size;
	size = bswap_32(size);

	FILE *f = fopen(argv[2], "w");
	if (fwrite(&size, 4, 1, f) != 1)
		abort();
	fclose(f);

	return 0;
}
