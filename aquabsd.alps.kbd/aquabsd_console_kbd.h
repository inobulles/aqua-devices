#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <termios.h>
#include <sys/ioctl.h>

static inline int init_aquabsd_console_kbd(void) {
	// set TTY how we want it

	struct termios tty;
	tcgetattr(0, &tty);

	cfmakeraw(&tty);

	tty.c_iflag  =   IGNPAR | IGNBRK;
	tty.c_oflag  =   OPOST  | ONLCR;
	tty.c_cflag  =   CREAD  | CS8;
	tty.c_lflag &= ~(ICANON | ECHO | ISIG);

	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN ] = 0;

	cfsetispeed(&tty, 9600);
	cfsetospeed(&tty, 9600);

	tcsetattr(0, TCSANOW | TCSAFLUSH, &tty);

	return 0;
}

static inline int update_aquabsd_console_kbd(kbd_t* kbd, void* _) {
	uint8_t ch;

	// reset keyboard structure

	memset(kbd->buttons, 0, sizeof kbd->buttons);

	// continuously process while there still are keys pressed

	while (read(0, &ch, 1) > 0) {
		if (ch != 0x1B) {
			continue; // currently not processing anything other than escape sequences
		}

		uint8_t esc[2];

		if (read(0, esc, sizeof esc) != sizeof esc) {
			continue; // something went wrong
		}

		if (esc[0] == 217 && esc[1] == 62) {
			kbd->buttons[BUTTON_ESC] = 1;
		}

		else if (esc[0] == '[') { // arrow keys
			if      (esc[1] == 'A') kbd->buttons[BUTTON_UP]    = 1;
			else if (esc[1] == 'B') kbd->buttons[BUTTON_DOWN]  = 1;
			else if (esc[1] == 'C') kbd->buttons[BUTTON_RIGHT] = 1;
			else if (esc[1] == 'D') kbd->buttons[BUTTON_LEFT]  = 1;
		}
	}

	return 0;
}
