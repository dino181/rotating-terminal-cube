#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ncurses.h>

// display variables
#define FPS 60
#define CUBE_SIZE 100
#define CELL_WIDTH 2

// Object variables
#define MODEL_SIZE 1
#define N_FACES 6
#define N_CORNERS 8

#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

struct Face{
	int corners[4];
	char fill;
	bool is_visible;
};

void debug_print_corners(float og_corners[N_CORNERS][3], float rotated_corners[N_CORNERS][3], float rotation[3]){
	printf("Original (x,y,z) => rotated (x,y,z)\n");
	printf("Rotations: %15f,%15f,%15f\n", rotation[0],rotation[1],rotation[2]);
	for (int i =0; i < N_CORNERS; i++){
		printf("%15f,%15f,%15f => %15f,%15f,%15f\n", og_corners[i][0],og_corners[i][1],og_corners[i][2],rotated_corners[i][0],rotated_corners[i][1],rotated_corners[i][2]);
	}
}

void flood_fill(int x, int y, char* side, char fill){
	if (side[CELL_WIDTH * y * CUBE_SIZE + CELL_WIDTH * x + y] == fill){
		return;
	}

	side[CELL_WIDTH * y * CUBE_SIZE + CELL_WIDTH * x + y] = fill;
	side[CELL_WIDTH * y * CUBE_SIZE + CELL_WIDTH * x + y + 1] = fill;

	flood_fill(x + 1, y, side, fill);
	flood_fill(x - 1, y, side, fill);
	flood_fill(x, y + 1, side, fill);
	flood_fill(x, y - 1, side, fill);
}

void print_side(float corners[4][2], int n_vertices, char fill, char side[CELL_WIDTH*CUBE_SIZE*CUBE_SIZE + CUBE_SIZE]){
	int center = CUBE_SIZE / 2.0;

	for (int k = 0; k < n_vertices; k++){
		int x1 = center + CUBE_SIZE/4.0 * corners[k][0];
		int y1 = center + CUBE_SIZE/4.0 * corners[k][1];
		int x2 = center + CUBE_SIZE/4.0 * corners[(k+1) % n_vertices][0];
		int y2 = center + CUBE_SIZE/4.0 * corners[(k+1) % n_vertices][1];


		// print diagonal line
		if (abs(x2-x1) == abs(y2-y1)){
			if ((y2 -y1) * (x2-x1) < 0){
				for (int i = min(y1,y2); i <= max(y1,y2); i++){
					side[CELL_WIDTH * i * CUBE_SIZE + CELL_WIDTH * (max(x2,x1) - i + min(y1,y2))+ i] = fill;
					side[CELL_WIDTH * i * CUBE_SIZE + CELL_WIDTH * (max(x2,x1) - i + min(y1,y2)) + i + 1] = fill;
				}
			}
			else{
				int min_x  = min(x1,x2);
				int min_y  = min(y1,y2);

				for (int i =0; i <= abs(y2-y1); i++){
					side[CELL_WIDTH * (min_y + i) * CUBE_SIZE + CELL_WIDTH * (min_x + i) + (min_y + i)] = fill;
					side[CELL_WIDTH * (min_y + i) * CUBE_SIZE + CELL_WIDTH * (min_x + i) + (min_y + i) + 1] = fill;

				}
			}
		}

		// bresenham
		int sign = 1;
		float slope = (-1.0 * (y2-y1))/ (1.0*(x2-x1));
		bool switch_x_y = false;

		if (slope < 0){
			y1 *= -1;
			y2 *= -1;
			sign = -1;
			slope = (-1.0 * (y2-y1))/ (1.0*(x2-x1));
		}

		if (slope > 1){
			int temp1 = x1;
			int temp2 = x2;
			x2 = y2;
			x1 = y1;
			y1 = temp1;
			y2 = temp2;
			switch_x_y = true;
		}


		int d[2] = {x2-x1 , -1 * (y2-y1)};
		int pos[2] = {x1, y1};

		if (x1 > x2){
			d[0] = x1-x2;
			d[1] = -1 * (y1-y2);
			pos[0] = x2;
			pos[1] = y2;
		}

		int diff = 2 * d[1]-d[0];

		for (int cur_step = 0; cur_step <= d[0]; cur_step++){
			if (switch_x_y){
				side[CELL_WIDTH * sign * pos[0] * CUBE_SIZE + CELL_WIDTH * pos[1] + sign * pos[0]] = fill;
				side[CELL_WIDTH * sign * pos[0] * CUBE_SIZE + CELL_WIDTH * pos[1] + sign * pos[0] + 1] = fill;
			}
			else {
				side[CELL_WIDTH * sign * pos[1] * CUBE_SIZE + CELL_WIDTH * pos[0] + sign * pos[1]] = fill;
				side[CELL_WIDTH * sign * pos[1] * CUBE_SIZE + CELL_WIDTH * pos[0] + sign * pos[1] + 1] = fill;
			}

			if (diff <= 0){
				diff += 2 * d[1];
			}
			else {
				diff += 2 * (d[1] - d[0]);
				pos[1]--;
			}
			pos[0]++;
		}
	}

	// fill
	int x1 = center + CUBE_SIZE/4.0 * corners[0][0];
	int y1 = center + CUBE_SIZE/4.0 * corners[0][1];
	int x2 = center + CUBE_SIZE/4.0 * corners[2][0];
	int y2 = center + CUBE_SIZE/4.0 * corners[2][1];

	int fill_center_x = x1 + ((x2 - x1) / 2);
	int fill_center_y = y1 + ((y2 - y1) / 2);

	if(side[CELL_WIDTH * (fill_center_y-1) * CUBE_SIZE + CELL_WIDTH * fill_center_x + fill_center_y - 1 ] !=  fill &&
	   side[CELL_WIDTH * (fill_center_y+1) * CUBE_SIZE + CELL_WIDTH * fill_center_x + fill_center_y + 1 ] !=  fill &&
	   side[CELL_WIDTH * fill_center_y * CUBE_SIZE + CELL_WIDTH * (fill_center_x - 1) + fill_center_y] !=  fill && 
	   side[CELL_WIDTH * fill_center_y * CUBE_SIZE + CELL_WIDTH * (fill_center_x + 1) + fill_center_y] !=  fill ){
		flood_fill(fill_center_x, fill_center_y, side, fill);
	};
}

void project_cube(float cube[N_CORNERS][3], float projected_cube[N_CORNERS][3], float cam[3]){
	float near = cam[2] / 10.0; 
	float far = cam[2] * 10; 
	float angleOfView = 60;

	float m[4][4] = {0};

	float scale = 1 / tan(angleOfView * 0.5 * M_PI / 180); 
	m[0][0] = scale;  // scale the x coordinates of the projected point 
	m[1][1] = scale;  // scale the y coordinates of the projected point 
	m[2][2] = -(far + near) / (far - near);  // used to remap z to [0,1] 
	m[2][3] = -2.0 * far * near / (far - near);  // used to remap z [0,1] 
	m[3][2] = 1.0;  // set w = -z 
	m[3][3] = 0.0; 

	for (int i =0; i < 8; i++){
		cube[i][2] += cam[2];

		projected_cube[i][0] = cube[i][0] * m[0][0] + cube[i][1] * m[0][1] + cube[i][2] * m[0][2] + m[0][3]; 
		projected_cube[i][1] = cube[i][0] * m[1][0] + cube[i][1] * m[1][1] + cube[i][2] * m[1][2] + m[1][3]; 
		projected_cube[i][2] = cube[i][0] * m[2][0] + cube[i][1] * m[2][1] + cube[i][2] * m[2][2] + m[2][3]; 
		float w		     = cube[i][0] * m[3][0] + cube[i][1] * m[3][1] + cube[i][2] * m[3][2] + m[3][3]; 

		if (w != 1){
			projected_cube[i][0] /= w;
			projected_cube[i][1] /= w;
			projected_cube[i][2] /= w;
		}
	}
}



void rotate_cube(float cube[N_CORNERS][3], float rotated_cube[N_CORNERS][3], float rotations[3]){
	float rot_x[3][3] = {
		{1, 0 ,0 },
		{0, cos(rotations[0]), -sin(rotations[0])},
		{0, sin(rotations[0]), cos(rotations[0])},
	};

	float rot_y[3][3] = {
		{cos(rotations[1]), 0, sin(rotations[1])},
		{0, 1, 0 },
		{-sin(rotations[1]), 0, cos(rotations[1])},
	};

	float rot_z[3][3] = {
		{cos(rotations[2]), -sin(rotations[2]), 0},
		{sin(rotations[2]), cos(rotations[2]), 0},
		{0, 0 ,1 },
	};

	for (int i = 0; i < N_CORNERS; i++){
		float x_rot_x = cube[i][0] * rot_x[0][0] + cube[i][1] * rot_x[0][1] + cube[i][2] * rot_x[0][2];
		float y_rot_x = cube[i][0] * rot_x[1][0] + cube[i][1] * rot_x[1][1] + cube[i][2] * rot_x[1][2];
		float z_rot_x = cube[i][0] * rot_x[2][0] + cube[i][1] * rot_x[2][1] + cube[i][2] * rot_x[2][2];

		float x_rot_y = x_rot_x * rot_y[0][0] + y_rot_x * rot_y[0][1] + z_rot_x * rot_y[0][2];
		float y_rot_y = x_rot_x * rot_y[1][0] + y_rot_x * rot_y[1][1] + z_rot_x * rot_y[1][2];
		float z_rot_y = x_rot_x * rot_y[2][0] + y_rot_x * rot_y[2][1] + z_rot_x * rot_y[2][2];

		rotated_cube[i][0] = x_rot_y * rot_z[0][0] + y_rot_y * rot_z[0][1] + z_rot_y * rot_z[0][2];
		rotated_cube[i][1] = x_rot_y * rot_z[1][0] + y_rot_y * rot_z[1][1] + z_rot_y * rot_z[1][2];
		rotated_cube[i][2] = x_rot_y * rot_z[2][0] + y_rot_y * rot_z[2][1] + z_rot_y * rot_z[2][2];
	}
}

void backface_culling(float cube[N_CORNERS][3], struct Face faces[N_FACES], float cam[3]){
	for (int i =0; i< N_FACES; i++){
		float v1[3] = {
			cube[faces[i].corners[0]][0] - cube[faces[i].corners[1]][0],
			cube[faces[i].corners[0]][1] - cube[faces[i].corners[1]][1],
			cube[faces[i].corners[0]][2] - cube[faces[i].corners[1]][2],
		};
	
		float v2[3] = {
			cube[faces[i].corners[3]][0] - cube[faces[i].corners[1]][0],
			cube[faces[i].corners[3]][1] - cube[faces[i].corners[1]][1],
			cube[faces[i].corners[3]][2] - cube[faces[i].corners[1]][2],
		};

		float normal[3] = {
				v1[1] * v2[2] - v1[2] * v2[1],
				v1[2] * v2[0] - v1[0] * v2[2],
				v1[0] * v2[1] - v1[1] * v2[0],
		};

		float norm  = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);

		float dot = normal[0]/norm * cam[0] + normal[1]/norm * cam[1] + -1 * normal[2]/norm * cam[2];

		faces[i].is_visible = dot > 1;
	}
}

void print_cube(float cube[N_CORNERS][3], struct Face faces[N_FACES]){
	char side[CELL_WIDTH*CUBE_SIZE*CUBE_SIZE + CUBE_SIZE] = {0};
	for (int i = 0; i < CUBE_SIZE ; i++){
		for (int j = 0; j < CUBE_SIZE ; j++){
			side[CELL_WIDTH * i * CUBE_SIZE + CELL_WIDTH * j + i] = ' ';
			side[CELL_WIDTH * i * CUBE_SIZE + CELL_WIDTH * j + i + 1] = ' ';
		}
		side[CELL_WIDTH * i * CUBE_SIZE + CELL_WIDTH * CUBE_SIZE + i] = '\n';
	}

	int center = CUBE_SIZE / 2.0;

	for (int i = 0; i < N_FACES; i++){
		if (!faces[i].is_visible){
			continue;
		}

		float corners[4][2];
		for (int j = 0; j < 4; j++){
			corners[j][0] = cube[faces[i].corners[j]][0];
			corners[j][1] = cube[faces[i].corners[j]][1];
		}

		print_side(corners, 4, faces[i].fill, side);
	}

	mvprintw(0,0, "%s", side);
	refresh();
}
void init_window(){
	initscr();
	noecho();
	curs_set(0);
	nodelay(stdscr, true); 
}

void destroy_window(){
	endwin();
}

int main(){
	init_window();

	int rotations[3] = {0, 0, 0}; 
	float cam[3] = {0,0,3*MODEL_SIZE};

	// x,y,z
	float rotated_cube[N_CORNERS][3];
	float projected_cube[N_CORNERS][3];
	float cube[N_CORNERS][3] = {
		{-1, -1, -1},
		{-1, -1,  1},
		{ 1, -1, -1},
		{ 1, -1,  1},
		{-1,  1, -1},
		{-1,  1,  1},
		{ 1,  1, -1},
		{ 1,  1,  1},
	};

	struct Face faces[N_FACES] = {
		{{0, 2, 6, 4}, '#'},
		{{2, 3, 7, 6}, '.'},
		{{3, 1, 5, 7}, '@'},
		{{1, 0, 4, 5}, ':'},
		{{1, 3, 2, 0}, '+'},
		{{4, 6, 7, 5}, '='},
	};

	// rotation speed in degrees
	int rot_x_speed = 0;
	int rot_y_speed = 0;
	int rot_z_speed = 0;

	bool running = true;
	while (running){

		rotations[0] += rot_x_speed;
		rotations[1] += rot_y_speed;
		rotations[2] += rot_z_speed;

		for (int i = 0; i < 3; i++){
			rotations[i] %= 360;
		}

		float radians[3] = {
			rotations[0] * 2 * M_PI / 360,
			rotations[1] * 2 * M_PI / 360,
			rotations[2] * 2 * M_PI / 360,
		};

		rotate_cube(cube, rotated_cube, radians);

		backface_culling(rotated_cube, faces, cam);

		project_cube(rotated_cube, projected_cube, cam);

		print_cube(projected_cube, faces);

		switch(getch()){
			case 'x':
				rot_x_speed++;
				break;
			case 'y':
				rot_y_speed++;
				break;
			case 'z':
				rot_z_speed++;
				break;
			case 'X':
				rot_x_speed--;
				break;
			case 'Y':
				rot_y_speed--;
				break;
			case 'Z':
				rot_z_speed--;
				break;
			case 'q':
				running = false;
				break;
			default:
				break;
		}
		
		usleep(1000 * 1000 / FPS);
	}

	destroy_window();
	exit(EXIT_SUCCESS);
}
