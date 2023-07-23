// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <private.h>

#include <string.h>

ui_t* ui_create(char* const backend_name, uint64_t* args) {
	backend_t backend = { 0 };

	LOG_VERBOSE("Creating '%s' UI backend", backend_name);

#if defined(WITH_WGPU)
	if (strcmp(backend_name, "wgpu") == 0) {
		aquabsd_alps_win_t* const win = (void*) args[0];
		backend.internal = backend_wgpu_create(win);

		if (backend.internal != NULL) {
			backend.valid = true;
			backend.name = "WebGPU";

			backend.destroy = backend_wgpu_destroy;
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

	return ui;
}

void ui_destroy(ui_t* ui) {
	ui->backend.destroy(ui->backend.internal);
	free(ui);
}
