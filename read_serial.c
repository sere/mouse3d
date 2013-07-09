/*
 * Test program which reads input from a serial device and moves the mouse
 * pointer accordingly.
 * The codes used for the different movements:
 * 1: right, 2: left, 3: up, 4: down
 *
 * Signed-off-by: Giorgio Guerzoni <giorgio.guerzoni@gmail.com>
 * Signed-off-by: Serena Ziviani <senseriumi@gmail.com>
 * Signed-off-by: Arianna Avanzini <avanzini.arianna@gmail.com>
 *
 * Configure ACM usb device with:
 * # stty -F /dev/ttyACM0 cs8 38400 ignbrk -brkint -imaxbel -opost -onlcr \
 * # -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon \
 * # -crtscts
 * To run this program with Arduino Uno:
 * # ./read_serial <special_device_name>
 * e.g., ./read_serial /dev/ttyACM0
 */

#include <stdio.h>		/* perror(), printf() */
#include <stdlib.h>		/* exit() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>		/* open() */
#include <unistd.h>		/* write(), close() */
#include <assert.h>		/* assert() */
#include <linux/input.h>	/* struct uinput_user_dev */
#include <linux/uinput.h>	/* struct input_event */
#include <string.h>		/* memset() */

#define	MOVEMENT_SIZE	10

int setup_uinput (void)
{
	struct uinput_user_dev uinp;
	int ufile, i, retcode;

	if(!(ufile = open("/dev/uinput", O_WRONLY | O_NDELAY ))) {
		perror("Could not open uinput");
		return -1;
	}

	memset(&uinp, 0, sizeof(uinp));
	strncpy(uinp.name, "simulated mouse", 20);
	uinp.id.version = 4;
	uinp.id.bustype = BUS_USB;

	ioctl(ufile, UI_SET_EVBIT, EV_KEY);
	ioctl(ufile, UI_SET_EVBIT, EV_REL);
	ioctl(ufile, UI_SET_RELBIT, REL_X);
	ioctl(ufile, UI_SET_RELBIT, REL_Y);

	for (i = 0; i < 256; i++)
		ioctl(ufile, UI_SET_KEYBIT, i);

	ioctl(ufile, UI_SET_KEYBIT, BTN_MOUSE);

	/* create input device in input subsystem */
	if(write(ufile, &uinp, sizeof(uinp)) == -1) {
		perror("Error writing uimp");
		fprintf(stderr,"Maybe you hadn't write permission on the file\n");
		return -1;
	}

	if((retcode = (ioctl(ufile, UI_DEV_CREATE)))) {
		printf("Error create uinput device %d.\n", retcode);
		return -1;
	}

	/* this allows to server X to get aware of the new input device */
	sleep(1);

	return ufile;
}

int teardown_uinput (int ufile)
{
	return close(ufile);
}

int move_cursor (int ufile, int code, int value)
{
	struct input_event event;

	assert(ufile >= 0);
	memset(&event, 0, sizeof(event));

	/* prepare uinput event structure and write the codes */
	event.type = EV_REL;
	event.code = code;
	event.value = value;
	if (write(ufile, &event, sizeof(event)) == -1) {
		perror("write() EV");
		return -1;
	}
	event.type = EV_SYN;
	event.code = SYN_REPORT;
	event.value = 0;
	if (write(ufile, &event, sizeof(event)) == -1) {
		perror("write() SYN");
		return -1;
	}

	return 0;
}

int main (int argc, char** argv)
{
	int ufile = -1, fd = -1;
	ssize_t ret;
	unsigned char buf;

	if (argc < 2) {
		printf("FATAL: no serial filename specified\n");
		exit(1);
	}

	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("open()");
		exit(1);
	}

	if ((ufile = setup_uinput()) < 0) {
		printf("FATAL: error while opening uinput device\n");
		goto out_fd;
		exit(1);
	}

	while ((ret = read(fd, &buf, sizeof(buf))) != 0) {
		if (ret == -1) {
			perror("read()");
			goto out;
		}
		switch (buf) {
			/*
			 * ignore NULL bytes and newlines from serial
			 * device
			 */
			case 0 :
			case '\n':
				/* do nothing */
				break;
			case '1':
				/* right */
				move_cursor(ufile, REL_X, MOVEMENT_SIZE);
				break;
			case '2':
				/* left */
				move_cursor(ufile, REL_X, -MOVEMENT_SIZE);
				break;
			case '3':
				/* up */
				move_cursor(ufile, REL_Y, MOVEMENT_SIZE);
				break;
			case '4':
				/* down */
				move_cursor(ufile, REL_Y, -MOVEMENT_SIZE);
				break;
			default:
				printf("Wrong code (received %d)\n", buf);
		}
	}

out:
	teardown_uinput(ufile);
out_fd:
	close(fd);
	return ret;
}

