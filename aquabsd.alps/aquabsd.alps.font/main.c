#include <aquabsd.alps.font/private.h>
#include <aquabsd.alps.font/functions.h>

typedef enum {
	CMD_LOAD_FONT     = 0x6C66, // 'lf'
	CMD_FREE_FONT     = 0x6666, // 'ff'

	CMD_CREATE_TEXT   = 0x6374, // 'ct'
	CMD_FREE_TEXT     = 0x6674, // 'ft'

	CMD_TEXT_COLOUR   = 0x7463, // 'tc'
	CMD_TEXT_SIZE     = 0x7473, // 'ts'
	CMD_TEXT_WRAP     = 0x7477, // 'tw'
	CMD_TEXT_ALIGN    = 0x7461, // 'ta'
	CMD_TEXT_MARKUP   = 0x746D, // 'tm'

	CMD_TEXT_GET_RES  = 0x6772, // 'gr'
	CMD_TEXT_POS_TO_I = 0x7069, // 'pi'
	CMD_TEXT_I_TO_POS = 0x6970, // 'ip'

	CMD_DRAW_TEXT     = 0x6474, // 'dt'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_LOAD_FONT) {
		const char* path = (void*) args[0];
		return (uint64_t) load_font(path);
	}

	else if (cmd == CMD_FREE_FONT) {
		font_t* font = (void*) args[0];
		return (uint64_t) free_font(font);
	}

	else if (cmd == CMD_TEXT_COLOUR) {
		text_t* text = (void*) args[0];

		float r = *(float*) args[1];
		float g = *(float*) args[2];
		float b = *(float*) args[3];
		float a = *(float*) args[4];

		return text_colour(text, r, g, b, a);
	}

	else if (cmd == CMD_TEXT_SIZE) {
		text_t* text = (void*) args[0];
		uint64_t size = args[1];

		return text_size(text, size);
	}

	else if (cmd == CMD_TEXT_WRAP) {
		text_t* text = (void*) args[0];

		uint64_t wrap_width  = args[1];
		uint64_t wrap_height = args[2];

		return text_wrap(text, wrap_width, wrap_height);
	}

	else if (cmd == CMD_TEXT_ALIGN) {
		text_t* text = (void*) args[0];
		align_t align = args[1];

		return text_align(text, align);
	}

	else if (cmd == CMD_TEXT_MARKUP) {
		text_t* text = (void*) args[0];
		bool markup = args[1];

		return text_markup(text, markup);
	}

	else if (cmd == CMD_TEXT_GET_RES) {
		text_t* text = (void*) args[0];

		uint64_t* x_res_ref = (void*) args[1];
		uint64_t* y_res_ref = (void*) args[2];

		return text_get_res(text, x_res_ref, y_res_ref);
	}

	else if (cmd == CMD_TEXT_POS_TO_I) {
		text_t* text = (void*) args[0];

		uint64_t x = args[1];
		uint64_t y = args[2];

		return text_pos_to_i(text, x, y);
	}

	else if (cmd == CMD_TEXT_I_TO_POS) {
		text_t* text = (void*) args[0];
		uint64_t i = args[1];

		uint64_t* x_ref      = (void*) args[2];
		uint64_t* y_ref      = (void*) args[3];
		uint64_t* width_ref  = (void*) args[4];
		uint64_t* height_ref = (void*) args[5];

		return text_i_to_pos(text, i, x_ref, y_ref, width_ref, height_ref);
	}

	else if (cmd == CMD_DRAW_TEXT) {
		text_t* text = (void*) args[0];
		uint8_t** bmp_ref = (void*) args[1];

		return draw_text(text, bmp_ref);
	}

	return -1;
}
