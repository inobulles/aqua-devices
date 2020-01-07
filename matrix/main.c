
#include <math.h>
#include <stdint.h>
#include <string.h>

#define PRECISION 1000000.0

typedef struct {
	float matrix[4][4];
} matrix_t;

static void multiply_matrices(matrix_t* output_matrix, matrix_t* x_matrix, matrix_t* y_matrix) {
	matrix_t temp_matrix;
	
	for (int i = 0; i < 4; i++) {
		temp_matrix.matrix[i][0] = \
			(x_matrix->matrix[i][0] * y_matrix->matrix[0][0]) + \
			(x_matrix->matrix[i][1] * y_matrix->matrix[1][0]) + \
			(x_matrix->matrix[i][2] * y_matrix->matrix[2][0]) + \
			(x_matrix->matrix[i][3] * y_matrix->matrix[3][0]);
		
		temp_matrix.matrix[i][1] = \
			(x_matrix->matrix[i][0] * y_matrix->matrix[0][1]) + \
			(x_matrix->matrix[i][1] * y_matrix->matrix[1][1]) + \
			(x_matrix->matrix[i][2] * y_matrix->matrix[2][1]) + \
			(x_matrix->matrix[i][3] * y_matrix->matrix[3][1]);
		
		temp_matrix.matrix[i][2] = \
			(x_matrix->matrix[i][0] * y_matrix->matrix[0][2]) + \
			(x_matrix->matrix[i][1] * y_matrix->matrix[1][2]) + \
			(x_matrix->matrix[i][2] * y_matrix->matrix[2][2]) + \
			(x_matrix->matrix[i][3] * y_matrix->matrix[3][2]);
		
		temp_matrix.matrix[i][3] = \
			(x_matrix->matrix[i][0] * y_matrix->matrix[0][3]) + \
			(x_matrix->matrix[i][1] * y_matrix->matrix[1][3]) + \
			(x_matrix->matrix[i][2] * y_matrix->matrix[2][3]) + \
			(x_matrix->matrix[i][3] * y_matrix->matrix[3][3]);
	}
	
	memcpy(output_matrix, &temp_matrix, sizeof(matrix_t));
}

static void load_identity(matrix_t* matrix) {
	memset(matrix, 0, sizeof(matrix_t));
	
	matrix->matrix[0][0] = 1.0;
	matrix->matrix[1][1] = 1.0;
	matrix->matrix[2][2] = 1.0;
	matrix->matrix[3][3] = 1.0;
}

uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x62) { // bytes
		kos_bda[0] = sizeof(matrix_t);
		
	} else if (data[0] == 0x69) { // identity
		matrix_t* output_matrix = (matrix_t*) data[1];
		load_identity(output_matrix);
		
	} else if (data[0] == 0x6D) { // multiply
		matrix_t* output_matrix = (matrix_t*) data[1];
		
		matrix_t* x_matrix = (matrix_t*) data[2];
		matrix_t* y_matrix = (matrix_t*) data[3];
		
		multiply_matrices(output_matrix, x_matrix, y_matrix);
		
	} else if (data[0] == 0x73) { // scale
		matrix_t* output_matrix = (matrix_t*) data[1];
		matrix_t* input_matrix = (matrix_t*) data[2];
		
		float scale_x = (float) (int64_t) data[3] / PRECISION;
		float scale_y = (float) (int64_t) data[4] / PRECISION;
		float scale_z = (float) (int64_t) data[5] / PRECISION;
		
		for (int x = 0; x < 4; x++) output_matrix->matrix[0][x] = input_matrix->matrix[0][x] * scale_x;
		for (int x = 0; x < 4; x++) output_matrix->matrix[1][x] = input_matrix->matrix[1][x] * scale_y;
		for (int x = 0; x < 4; x++) output_matrix->matrix[2][x] = input_matrix->matrix[2][x] * scale_z;
		
	} else if (data[0] == 0x74) { // translate
		matrix_t* output_matrix = (matrix_t*) data[1];
		matrix_t* input_matrix = (matrix_t*) data[2];
		
		float translation_x = (float) (int64_t) data[3] / PRECISION;
		float translation_y = (float) (int64_t) data[4] / PRECISION;
		float translation_z = (float) (int64_t) data[5] / PRECISION;
		
		for (int x = 0; x < 4; x++) {
			output_matrix->matrix[3][x] = input_matrix->matrix[3][x] + (input_matrix->matrix[0][x] * translation_x + input_matrix->matrix[1][x] * translation_y + input_matrix->matrix[2][x] * translation_z);
		}
		
	} else if (data[0] == 0x72) { // rotate
		matrix_t* output_matrix = (matrix_t*) data[1];
		matrix_t* input_matrix = (matrix_t*) data[2];
		
		float rotation_angle = (float) (int64_t) data[3] / PRECISION;
		
		float rotation_x = (float) (-(int64_t) data[4]) / PRECISION;
		float rotation_y = (float) (-(int64_t) data[5]) / PRECISION;
		float rotation_z = (float) (-(int64_t) data[6]) / PRECISION;
		
		float magnitude = sqrtf(rotation_x * rotation_x + rotation_y * rotation_y + rotation_z * rotation_z);
		
		float sin_angle = sinf(rotation_angle);
		float cos_angle = cosf(rotation_angle);
		
		rotation_x /= magnitude;
		rotation_y /= magnitude;
		rotation_z /= magnitude;
		
		float xx = rotation_x * rotation_x;
		float yy = rotation_y * rotation_y;
		float zz = rotation_z * rotation_z;
		
		float xy = rotation_x * rotation_y;
		float yz = rotation_y * rotation_z;
		float zx = rotation_z * rotation_x;
		
		float xs = rotation_x * sin_angle;
		float ys = rotation_y * sin_angle;
		float zs = rotation_z * sin_angle;
		
		float one_minus_cos = 1.0 - cos_angle;
		
		matrix_t rotation_matrix;
		memset(&rotation_matrix, 0, sizeof(matrix_t));
		
		rotation_matrix.matrix[0][0] = (one_minus_cos * xx) + cos_angle;
		rotation_matrix.matrix[0][1] = (one_minus_cos * xy) - zs;
		rotation_matrix.matrix[0][2] = (one_minus_cos * zx) + ys;
		
		rotation_matrix.matrix[1][0] = (one_minus_cos * xy) + zs;
		rotation_matrix.matrix[1][1] = (one_minus_cos * yy) + cos_angle;
		rotation_matrix.matrix[1][2] = (one_minus_cos * yz) - xs;
		
		rotation_matrix.matrix[2][0] = (one_minus_cos * zx) - ys;
		rotation_matrix.matrix[2][1] = (one_minus_cos * yz) + xs;
		rotation_matrix.matrix[2][2] = (one_minus_cos * zz) + cos_angle;
		
		rotation_matrix.matrix[3][3] = 1.0;
		multiply_matrices(output_matrix, &rotation_matrix, input_matrix);
		
	} else if (data[0] == 0x66) { // frustum
		matrix_t* output_matrix = (matrix_t*) data[1];
		
		matrix_t frustum_matrix;
		memset(&frustum_matrix, 0, sizeof(matrix_t));
		
		float near   = (float) (int64_t) data[6] / PRECISION;
		float far    = (float) (int64_t) data[7] / PRECISION;
		
		float left   = (float) (int64_t) data[2] / PRECISION * near;
		float right  = (float) (int64_t) data[3] / PRECISION * near;
		float bottom = (float) (int64_t) data[4] / PRECISION * near;
		float top    = (float) (int64_t) data[5] / PRECISION * near;
		
		float deltax = right - left;
		float deltay = top - bottom;
		float deltaz = far - near;
		
		frustum_matrix.matrix[0][0] = 2 * near / deltax;
		frustum_matrix.matrix[1][1] = 2 * near / deltay;
		
		frustum_matrix.matrix[2][0] = (right + left) / deltax;
		frustum_matrix.matrix[2][1] = (top + bottom) / deltay;
		frustum_matrix.matrix[2][2] = -(near + far)  / deltaz;
		
		frustum_matrix.matrix[2][3] = -1.0;
		frustum_matrix.matrix[3][2] = -2 * near * far / deltaz;
		
		multiply_matrices(output_matrix, &frustum_matrix, output_matrix);
		
	} else if (data[0] == 0x6F) { // ortho
		matrix_t* output_matrix = (matrix_t*) data[1];
		
		matrix_t ortho_matrix;
		load_identity(&ortho_matrix);
		
		float near   = (float) (int64_t) data[6] / PRECISION;
		float far    = (float) (int64_t) data[7] / PRECISION;
		
		float left   = (float) (int64_t) data[2] / PRECISION;
		float right  = (float) (int64_t) data[3] / PRECISION;
		float bottom = (float) (int64_t) data[4] / PRECISION;
		float top    = (float) (int64_t) data[5] / PRECISION;
		
		float deltax = right - left;
		float deltay = top - bottom;
		float deltaz = far - near;
		
		ortho_matrix.matrix[0][0] = 2.0 / deltax;
		ortho_matrix.matrix[3][0] = -(right + left) / deltax;
		
		ortho_matrix.matrix[1][1] = 2.0 / deltay;
		ortho_matrix.matrix[3][1] = -(top + bottom) / deltay;
		
		ortho_matrix.matrix[2][2] = 2.0 / deltax;
		ortho_matrix.matrix[3][2] = -(near + far) / deltaz;
		
		multiply_matrices(output_matrix, &ortho_matrix, output_matrix);
	}
}
