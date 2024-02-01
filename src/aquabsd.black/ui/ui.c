// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <ui.h>

#include <string.h>

ui_t* ui_create(char const* backend_name, uint64_t* args) {
	backend_t backend = {
		.destroy = backend_dummy_destroy,
		.prerender = backend_dummy_prerender,
		.postrender = backend_dummy_postrender,
	};

	LOG_VERBOSE("Creating '%s' UI backend", backend_name);

#if defined(WITH_WGPU)
	if (strcmp(backend_name, "wgpu") == 0) {
		aquabsd_alps_win_t* const win = (void*) args[0];
		backend.internal = backend_wgpu_create(win);

		if (backend.internal != NULL) {
			backend.valid = true;
			backend.name = "WebGPU";

			backend.destroy = backend_wgpu_destroy;

			backend.prerender = backend_wgpu_prerender;
			backend.postrender = backend_wgpu_postrender;
		}
	}
#endif

	if (!backend.valid) {
		LOG_ERROR("Backend '%s' is not valid", backend_name);
		return NULL;
	}

	LOG_VERBOSE("Creating UI instance");

	ui_t* const ui = calloc(1, sizeof *ui);
	memcpy(&ui->backend, &backend, sizeof ui->backend);

	LOG_VERBOSE("Creating root element");

	elem_t dummy = { .ui = ui };
	ui->root = elem_create(&dummy, ELEM_ROOT);

	LOG_VERBOSE("Creating font manager");

	if (font_man_create(&ui->font_man) < 0) {
		ui_destroy(ui);
		return NULL;
	}

	LOG_VERBOSE("Creating cache");

	if (cache_create(&ui->cache) < 0) {
		ui_destroy(ui);
		return NULL;
	}

	return ui;
}

void ui_destroy(ui_t* ui) {
	ui->backend.destroy(ui->backend.internal);

	if (ui->root) {
		elem_destroy(ui->root);
	}

	font_man_destroy(&ui->font_man);
	cache_destroy(&ui->cache);

	free(ui);
}

int ui_render(ui_t* ui) {
	if (ui->backend.prerender(ui->backend.internal) < 0) {
		return -1;
	}

	// TODO

	if (ui->backend.postrender(ui->backend.internal) < 0) {
		return -1;
	}

	return 0;
}
