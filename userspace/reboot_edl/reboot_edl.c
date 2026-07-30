// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <linux/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	sync();

	if (syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
		    LINUX_REBOOT_CMD_RESTART2, "edl") == -1) {
		perror("reboot edl");
		return EXIT_FAILURE;
	}

	fputs("EDL reboot returned without resetting the device\n", stderr);
	return EXIT_FAILURE;
}
