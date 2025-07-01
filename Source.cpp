#include <iostream>
#include <vector>
#include <cmath>
#include <conio.h>
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <string>
using namespace std;

const float PI = 3.1415926535f;
const int row = 100;
const int col = 220;
float hfov = 90;
float cameratoScreen = col / (2 * (tan(((hfov * PI) / 180.0f) / 2)));

vector<char> validInputsMap = { 'w', 'a', 's', 'd', ' ', 'v', 72, 80, 75, 77, 'r', 'o' , 'e', 'q'};
vector<vector<char>> screen(row, vector<char>(col, ' '));
vector<vector<bool>> screenpoints(row, vector<bool>(col, 0));
const char ch = '*';

class point3d
{
public:
	float x, y, z;

	point3d(){x = 0, y = 0, z = 0;}
	point3d(float X, float Y, float Z){x = X, y = Y, z = Z;}

	bool operator==(const point3d& other) const
	{
		return (x == other.x && y == other.y && z == other.z);
	}
};

void setCursorPosition(int x, int y)
{
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coord = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(hOut, coord);
}

float toradian(float angle){return ((angle * PI) / 180.0f);}

class Camera
{
public:
	float x = 0;
	float y = 0;
	float z = 1.5;

	float yaw = 0;  //degrees, towards y positive (North) = 0, right = pos until 360
	float pitch = 90; //degrees, up = 0, down = 180, straight = 90

	void cameramove(char direction, float degree)
	{
		if (direction == 'r')
		{
			yaw += degree;
		}
		else if (direction == 'l')
		{
			yaw -= degree;
		}
		else if (direction == 'u')
		{
			pitch -= degree;
		}
		else if (direction == 'd')
		{
			pitch += degree;
		}
		if (yaw < 0)
		{
			yaw += 360;
		}
		else if (yaw > 360)
		{
			yaw -= 360;
		}
		if (pitch >= 180)
		{
			pitch = 179.9;
		}
		else if (pitch <= 0)
		{
			pitch = 0.1;
		}
	}
	void move(char direction, float amount)
	{
		float modifiedyaw = (90 - yaw); //0 to 360 similar to yaw, changes depending on the direction of movement
		float movementvectorangle, movementvectorx, movementvectory;

		if (!(direction == 'u' || direction == 'd'))
		{
			if (direction == 'b')
			{
				modifiedyaw = (90 - yaw) + 180;
			}
			else if (direction == 'l')
			{
				modifiedyaw = (90 - yaw) + 90;
			}
			else if (direction == 'r')
			{
				modifiedyaw = (90 - yaw) - 90;
			}

			movementvectorangle = modifiedyaw;
			movementvectorx = amount * cos(toradian(movementvectorangle));
			movementvectory = amount * sin(toradian(movementvectorangle));

			x += movementvectorx;
			y += movementvectory;
		}
		else
		{
			if (direction == 'u')
			{
				z += amount;
			}
			else if (direction == 'd')
			{
				z -= amount;
			}
		}
	}
};
Camera camera;

static void save()
{
	ofstream outputFile;
	outputFile.open("Save File.txt");
	outputFile << "camera.x=" << camera.x << endl;
	outputFile << "camera.y=" << camera.y << endl;
	outputFile << "camera.z=" << camera.z << endl;
	outputFile << "camera.yaw=" << camera.yaw << endl;
	outputFile << "camera.pitch=" << camera.pitch << endl;
	outputFile.close();
}
static void load()
{
	ifstream savefile;
	savefile.open("Save File.txt");
	string line;
	while (getline(savefile, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		size_t delimiter_pos = line.find('=');
		string key = line.substr(0, delimiter_pos);
		string value = line.substr(delimiter_pos + 1);
		try
		{
			if (key == "camera.x")
				camera.x = stof(value);
			else if (key == "camera.y")
				camera.y = stof(value);
			else if (key == "camera.z")
				camera.z = stof(value);
			else if (key == "camera.yaw")
				camera.yaw = stof(value);
			else if (key == "camera.pitch")
				camera.pitch = stof(value);
		}
		catch (const std::invalid_argument& e)
		{
			cerr << "Warning: Corrupted or invalid data in save file for key: " << key << endl;
		}
		catch (const std::out_of_range& e)
		{
			cerr << "Warning: Value out of range in save file for key: " << key << endl;
		}
	}
	savefile.close();
}

struct TriangleToRender
{
	point3d p1, p2, p3;
	char symbol;
	float avg_dist;
};

vector<TriangleToRender> queue;

class point //point on projection plane
{
public:
	int x, y;

	point() : x(0), y(0) {}

	point(int X, int Y)
	{
		x = (row - 1) / 2 - Y, y = X + (col - 1) / 2;
	}
};

void screenSet(int x, int y, char c = ch)
{
	int _row, _col;
	_row = row / 2 - y;
	_col = x + col / 2;
	if (_row < row && _row >= 0 && _col < col && _col >= 0)
		screen[_row][_col] = c;
}
void screenPointSet(int x, int y)
{
	int _row, _col;
	_row = row / 2 - y;
	_col = x + col / 2;
	if (_row < row && _row >= 0 && _col < col && _col >= 0)
		screenpoints[_row][_col] = 1;
}

void pointConnect(point point1, point point2, bool show = 0)
{
	float x1, y1, x2, y2;
	x1 = point1.y - (col % 2 ? (col - 1) / 2 : col / 2);
	x2 = point2.y - (col % 2 ? (col - 1) / 2 : col / 2);
	y1 = (row % 2 ? (row - 1) / 2 : row / 2) - point1.x;
	y2 = (row % 2 ? (row - 1) / 2 : row / 2) - point2.x;

	if (x1 == x2 && y1 == y2)
	{
		return;
	}
	if (x1 == x2)
	{
		if (y1 < y2)
		{
			for (int i = y1; i <= y2; i++)
			{
				if (show)
				{
					screenSet(x1, i);
				}
				screenPointSet(x1, i);
			}
		}
		else if (y2 < y1)
		{
			for (int i = y2; i <= y1; i++)
			{
				if (show)
				{
					screenSet(x1, i);
				}
				screenPointSet(x1, i);
			}
		}
	}
	else if (y1 == y2)
	{
		if (x1 < x2)
		{
			for (int i = x1; i <= x2; i++)
			{
				if (show)
				{
					screenSet(i, y1);
				}
				screenPointSet(i, y1);
			}
		}
		else if (x2 < x1)
		{
			for (int i = x2; i <= x1; i++)
			{
				if (show)
				{
					screenSet(i, y1);
				}
				screenPointSet(i, y1);
			}
		}
	}

	else
	{
		float m = (y2 - y1) / (x2 - x1);
		float c = y1 - m * x1;
		if (m <= 1 && m >= -1)
		{
			float y;
			if (x1 < x2)
			{
				for (int i = x1; i <= x2; i++)
				{
					y = m * i + c;
					y = round(y);

					if (show)
					{
						screenSet(i, y);
					}
					screenPointSet(i, y);
				}
			}
			else if (x2 < x1)
			{
				for (int i = x2; i <= x1; i++)
				{
					y = m * i + c;
					y = round(y);

					if (show)
					{
						screenSet(i, y);
					}
					screenPointSet(i, y);
				}
			}
		}
		else
		{
			float x;
			if (y1 < y2)
			{
				for (int i = y1; i <= y2; i++)
				{
					x = (i - c) / m;
					x = round(x);

					if (show) screenSet(x, i);

					screenPointSet(x, i);
				}
			}
			else if (y2 < y1)
			{
				for (int i = y2; i <= y1; i++)
				{
					x = (i - c) / m;
					x = round(x);

					if (show)
					{
						screenSet(x, i);
					}
					screenPointSet(x, i);
				}
			}
		}
	}
}

void triangle(point point1, point point2, point point3, char c = ch)
{
	for (auto& rowVec : screenpoints)
		fill(rowVec.begin(), rowVec.end(), false);

	pointConnect(point1, point2);
	pointConnect(point2, point3);
	pointConnect(point3, point1);


	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			if (screenpoints[i][j] == 1)
			{
				bool yes = 0;
				int it;
				for (it = j + 1; it < col; it++)
				{
					if (screenpoints[i][it] == 1)
					{
						yes = 1;
						break;
					}
				}
				if (yes)
				{
					for (int count = j; count <= it; count++)
					{
						screen[i][count] = c;
					}
					j = it - 1;
				}
			}
		}
	}
}

int project3d(point3d pointa, char c)
{
	float _col = ((cameratoScreen / pointa.y) * pointa.x) + col / 2.0f;
	float _row = ((cameratoScreen / pointa.y) * -pointa.z) + row / 2.0f;

	if (c == 'x')
		return (_col - (col / 2.0f));
	else if (c == 'y')
		return((row / 2.0f) - _row);
}
point3d transformtoCamSpace(point3d w)
{
	float newx = w.x - camera.x;
	float newy = w.y - camera.y;
	float newz = w.z - camera.z;

	float angleRad = toradian(camera.yaw);
	float cosangleRad = cos(angleRad), sinangleRad = sin(angleRad);

	float yawedx = newx * cosangleRad - newy * sinangleRad;
	float yawedy = newx * sinangleRad + newy * cosangleRad;


	angleRad = toradian(-(90 - camera.pitch));
	cosangleRad = cos(angleRad), sinangleRad = sin(angleRad);

	float pitchedy = yawedy * cosangleRad - newz * sinangleRad;
	float pitchedz = yawedy * sinangleRad + newz * cosangleRad;

	return point3d(yawedx, pitchedy, pitchedz);
}

void renderTriangle3d(TriangleToRender tri)
{
	point3d cama = transformtoCamSpace(tri.p1);
	point3d camb = transformtoCamSpace(tri.p2);
	point3d camc = transformtoCamSpace(tri.p3);

	float near_plane = 0.1f;
	if (cama.y < near_plane || camb.y < near_plane || camc.y < near_plane)
		return;

	point point1(project3d(cama, 'x'), project3d(cama, 'y'));
	point point2(project3d(camb, 'x'), project3d(camb, 'y'));
	point point3(project3d(camc, 'x'), project3d(camc, 'y'));

	triangle(point1, point2, point3, tri.symbol);
}

void addTriangle3d(point3d pointa, point3d pointb, point3d pointc, char c = ch)
{
	TriangleToRender tri;
	tri.p1 = pointa;
	tri.p2 = pointb;
	tri.p3 = pointc;
	tri.symbol = c;

	queue.push_back(tri);
}

class block
{
public:
	point3d o;
	block(int x, int y, int z) : o(x, y, z) {}

	void add()
	{
		point3d a(o.x, o.y, o.z);
		point3d b(o.x + 1, o.y, o.z);
		point3d c(o.x, o.y, o.z + 1);
		point3d d(o.x + 1, o.y, o.z + 1);
		point3d e(o.x, o.y + 1, o.z);
		point3d f(o.x + 1, o.y + 1, o.z);
		point3d g(o.x, o.y + 1, o.z + 1);
		point3d h(o.x + 1, o.y + 1, o.z + 1);
		
		bool backmatched = 0, bottommatched = 0, leftmatched = 0, rightmatched = 0, topmatched = 0, frontmatched = 0;
		/*for (int i = 0; i < queue.size(); i++)
		{
			if (queue[i].p1 == g && queue[i].p2 == h && queue[i].p3 == e)
			{
				queue.erase(queue.begin() + i, queue.begin() + i + 2);
				backmatched = 1;
				i--;
			}
			else if (queue[i].p1 == e && queue[i].p2 ==f && queue[i].p3 == a)
			{
				queue.erase(queue.begin() + i, queue.begin() + i + 2);
				bottommatched = 1;
				i--;
			}
			else if (queue[i].p1 == a && queue[i].p2 == c && queue[i].p3 == e)
			{
				queue.erase(queue.begin() + i, queue.begin() + i + 2);
				leftmatched = 1;
				i--;
			}
			else if (queue[i].p1 == h && queue[i].p2 == f && queue[i].p3 == d)
			{
				queue.erase(queue.begin() + i, queue.begin() + i + 2);
				rightmatched = 1;
				i--;
			}
			else if (queue[i].p1 == g && queue[i].p2 == c && queue[i].p3 == h)
			{
				queue.erase(queue.begin() + i, queue.begin() + i + 2);
				topmatched = 1;
				i--;
			}
			else if (queue[i].p1 == a && queue[i].p2 == b && queue[i].p3 == c)
			{
				queue.erase(queue.begin() + i, queue.begin() + i + 2);
				frontmatched = 1;
				i--;
			}
		}*/
		if(!frontmatched)
		{
			addTriangle3d(g, h, e);    //back
			addTriangle3d(e, h, f);
		}
		if (!topmatched)
		{
			addTriangle3d(e, f, a, '.');     //bottom
			addTriangle3d(f, a, b, '.');
		}
		if (!rightmatched)
		{
			addTriangle3d(a, c, e, '#');    //left
			addTriangle3d(c, e, g, '#');
		}
		if (!leftmatched)
		{
			addTriangle3d(h, f, d, '#');    //right
			addTriangle3d(d, f, b, '#');
		}
		if (!bottommatched)
		{
			addTriangle3d(g, c, h, '@');    //top
			addTriangle3d(c, h, d, '@');
		}
		if (!backmatched)
		{
			addTriangle3d(a, b, c);    //front
			addTriangle3d(b, c, d);
		}
	}
};
vector<block> worldBlocks;

point3d unitvectorcamera()
{
	float normalyaw = toradian(90+(360 - camera.yaw));
	float normalpitch = toradian(camera.pitch);
	
	float sinp = sin(normalpitch);

	float x = sinp * cos(normalyaw);
	float y = sinp * sin(normalyaw);
	float z = cos(normalpitch);

	return { x, y, z };
}

struct raycastResult
{
	int blockindex = -1;
	point3d placepos;
};

raycastResult raycast(bool tobreak = 0)
{
	point3d unitvect = unitvectorcamera();

	point3d currentpos = { camera.x, camera.y, camera.z };
	point3d emptypos = currentpos;

	float advance = 0.05;
	for (float d = 0; d <= 13; d+= advance)
	{
		currentpos.x += unitvect.x * advance;
		currentpos.y += unitvect.y * advance;
		currentpos.z += unitvect.z * advance;

		for(int i = 0; i < worldBlocks.size(); i++)
		{
			if (!tobreak && currentpos.z >= -1 && currentpos.z <= 0)
			{
				return { i, emptypos };
			}
			if (
				(currentpos.x >= worldBlocks[i].o.x && currentpos.x <= worldBlocks[i].o.x + 1 &&
				currentpos.y >= worldBlocks[i].o.y && currentpos.y <= worldBlocks[i].o.y + 1 &&
				currentpos.z >= worldBlocks[i].o.z && currentpos.z <= worldBlocks[i].o.z + 1)
			   )
			{
				return { i, emptypos };
			}
		}
		emptypos = currentpos;
	}
	return { -1, {} };
}

void placeBlock()
{
	raycastResult result = raycast();
	if (result.blockindex == -1) return;

	int x = floor(result.placepos.x);
	int y = floor(result.placepos.y);
	int z = floor(result.placepos.z);

	worldBlocks.emplace_back(x, y, z);
}

void breakBlock()
{
	raycastResult result = raycast(1);
	if (result.blockindex == -1) return;

	worldBlocks.erase(worldBlocks.begin() + result.blockindex);
}

void action(char c)
{
	if (c == 'w') camera.move('f', 0.5);
	else if (c == 's') camera.move('b', 0.5);
	else if (c == 'a') camera.move('l', 0.5);
	else if (c == 'd') camera.move('r', 0.5);

	else if (c == ' ') camera.move('u', 0.5);
	else if (c == 'v') camera.move('d', 0.5);

	else if(c == 72) camera.cameramove('u', 5);
	else if (c == 75) camera.cameramove('l', 5);
	else if (c == 80) camera.cameramove('d', 5);
	else if (c == 77) camera.cameramove('r', 5);

	else if (c == 'r') system("cls");

	else if (c == 'o') save();

	else if (c == 'e') placeBlock();
	else if (c == 'q') breakBlock();
}

void show()
{
	screen[row / 2][col / 2] = 'O';
	setCursorPosition(0, 0);
	for (int i = 0; i < col * 2 + 1; i++)
		cout << "_";
	cout << "\n";
	{
		string buffer;
		buffer.reserve(col * 2);
		for (int i = 0; i < row; i++)
		{
			cout << "|";
			buffer.clear();
			for (int j = 0; j < col; j++)
			{
				buffer = buffer + screen[i][j] + ' ';
			}
			cout << buffer << "|\n";
		}
	}
	for (int i = 0; i < col * 2 + 1; i++)
		cout << "_";
	cout << "\nPosition: " << camera.x << " " << camera.y << " " << camera.z << " Yaw: " << camera.yaw << " Pitch: " << camera.pitch 
	<< "                                        \n";

	for (auto& rowVec : screen)
		fill(rowVec.begin(), rowVec.end(), ' ');
}

void road()
{
	/*point3d pointa; pointa.set(10, 50, -5);
	point3d pointb; pointb.set(-10, 50, -5);*/

	point3d pointc(10, -9, -5);
	point3d pointd(-10, -9, -5);
	point3d pointe(10, -3, -5);
	point3d pointf(-10, -3, -5);

	addTriangle3d(pointc, pointd, pointe, '@');
	addTriangle3d(pointd, pointe, pointf, '@');
}

void placeHouse(float x, float y, float z)
{
	for (int i = 0; i <= 4; i++)
	{
		for (int j = 3; j >= 0; j--)
		{
			worldBlocks.emplace_back(x + i, y + -1 * 4, z + j);
			if (i == 2 && (j == 1 || j == 0))
				continue;
			worldBlocks.emplace_back(x + i, y, z + j);
		}
	}
	for (int i = -1; i >= -3; i--)
	{
		for (int j = 3; j >= 0; j--)
		{
			worldBlocks.emplace_back(x, y + i, z + j);
			worldBlocks.emplace_back(x + 4, y + i, z + j);
		}
	}

	for (int i = -5; i <= 1; i++)
		worldBlocks.emplace_back(x - 1, y + i, z + 4),
		worldBlocks.emplace_back(x + 5, y + i, z + 4);
	for (int i = 0; i <= 4; i++)
		worldBlocks.emplace_back(x + i, y - 5, z + 4),
		worldBlocks.emplace_back(x + i, y + 1, z + 4);
	for(int i = -4; i <= 0; i++)
		worldBlocks.emplace_back(x, y + i, z + 5),
		worldBlocks.emplace_back(x+4, y + i, z + 5);
	for(int i = 1; i<= 3; i++)
		worldBlocks.emplace_back(x + i, y - 4, z + 5),
		worldBlocks.emplace_back(x + i, y, z + 5),
		worldBlocks.emplace_back(x + i, y - 3, z + 6),
		worldBlocks.emplace_back(x + i, y - 1, z + 6);

	worldBlocks.emplace_back(x + 3, y - 2, z + 6);
	worldBlocks.emplace_back(x + 1, y - 2, z + 6);

	worldBlocks.emplace_back(x + 2, y - 2, z + 7);
}

void tree(float x, float y, float z)
{
	for (int i = 0; i < 4; i++)
		worldBlocks.emplace_back(x, y, z+i);

	for(int i = -2; i <= 2; i++)
		for(int j = -2; j <= 2; j++)
			worldBlocks.emplace_back(x+i, y+j, z+4);

	for (int i = -1; i <= 1; i++)
		for (int j = -1; j <= 1; j++)
			worldBlocks.emplace_back(x + i, y + j, z+5);

	worldBlocks.emplace_back(x, y, z + 6);
}

void render()
{
	for (int i = 0; i < worldBlocks.size(); i++)
	{
		worldBlocks[i].add();
	}
	for (auto& tri : queue)
	{
		point3d p1_cam = transformtoCamSpace(tri.p1);
		point3d p2_cam = transformtoCamSpace(tri.p2);
		point3d p3_cam = transformtoCamSpace(tri.p3);
		tri.avg_dist = (p1_cam.y + p2_cam.y + p3_cam.y) / 3.0f;
	}

	sort(queue.begin(), queue.end(), [](const TriangleToRender& a, const TriangleToRender& b)
										{
											return a.avg_dist > b.avg_dist;
										}
	);

	for (const auto& tri : queue)
	{
		renderTriangle3d(tri);
	}
}

void prepareWorld()
{
	for(int i = 1; i <= 5; i++) //HELLO
	{
		worldBlocks.emplace_back(-5, 12, i);
		worldBlocks.emplace_back(-2, 12, i);

		worldBlocks.emplace_back(0, 12, i);

		worldBlocks.emplace_back(5, 12, i);

		worldBlocks.emplace_back(10, 12, i);
	}
	worldBlocks.emplace_back(-4, 12, 3);
	worldBlocks.emplace_back(-3, 12, 3);
	for (int i = 1; i <= 3; i++)
	{
		worldBlocks.emplace_back(i, 12, 1);
		worldBlocks.emplace_back(i, 12, 3);
		worldBlocks.emplace_back(i, 12, 5);

		worldBlocks.emplace_back(15, 12, i+1);

		worldBlocks.emplace_back(18, 12, i+1);
	}
	for (int i = 6; i <= 13; i++)
	{
		worldBlocks.emplace_back(i, 12, 1);
		if (i == 8) i = 10;
	}
	worldBlocks.emplace_back(16, 12, 1);
	worldBlocks.emplace_back(17, 12, 1);
	worldBlocks.emplace_back(16, 12, 5);
	worldBlocks.emplace_back(17, 12, 5);

	/*for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			for (int k = 0; k < 8; k++)
			{
				worldBlocks.emplace_back(i, j+20, k);
			}
		}
	}*/

	//placeHouse(-5, -5, 0);
	placeHouse(-18, -5, 0);

	tree(-24, -7, 0);
}

int main()
{
	prepareWorld();
	load();
	int inp;
	while (1)
	{
		queue.clear();
		render();
		show();
		while (1)
		{
			inp = _getch();
			if (find(validInputsMap.begin(), validInputsMap.end(), inp) != validInputsMap.end())
				break;
		}
		action(inp);
	}
	return 0;
}