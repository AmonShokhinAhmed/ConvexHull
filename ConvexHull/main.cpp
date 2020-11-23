#include <SFML/Graphics.hpp>
#include<iostream>
#include<chrono>

#include "datastructures.h"
#include "scene.h"

#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <set>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGTH 600

// Delimiter that defines the position of a point in a file (e.g. 10,20).
#define DATA_DELIMITER ','

enum class ArgumentType { LOAD, POINT_AMOUNT, VISUAL_MODE, HELP };

std::map<std::string, ArgumentType> argumentMap{
	{"--load", ArgumentType::LOAD},
	{"--points", ArgumentType::POINT_AMOUNT},
	{"-p", ArgumentType::POINT_AMOUNT},
	{"--visual", ArgumentType::VISUAL_MODE},
	{"-v", ArgumentType::VISUAL_MODE},
	{"--help", ArgumentType::HELP},
};


#pragma region Function declarations
//Setup functions
void handleArguments(int argc, char* argv[]);
void showWrongArguments();
void showHelp();
std::vector<Point> generatePointsFromFile(const std::string& filePath);
std::vector<Point> generateRandomPoints(unsigned int amount);
std::vector<Point> generateRandomPointsOnCircle(unsigned int amount);

//Optimized calculation functions
Hull calculateHull(std::vector<Point>& points);
Hull generateSmallestHull(std::vector<Point>& points);
Hull merge(Hull left, Hull right);
Line findUpperTangentOfHulls(Hull left, Hull right, Node* leftRight, Node* rightLeft);
bool isUpperTangentOfHull(Line tangent, Hull hull);

//Visual calculation functions
Hull calculateHullVisual(std::vector<Point>& points, Scene& scene);
Hull generateSmallestHullVisual(std::vector<Point>& points, Scene& scene);
Hull mergeVisual(Hull left, Hull right, Scene& scene);
Line visualFindUpperTangentOfHulls(Hull left, Hull right, Node* leftRight, Node* rightLeft, Scene& scene);
bool visualIsUpperTangentOfHull(Line tangent, Hull hull, Scene& scene);

//Utility functions
bool isPointLeftOfLine(Line line, Node* point);
#pragma endregion

bool visualMode = false;
std::string filePath = "";
//std::string filePath = "..\\Testcases\\TwoTriangles.txt";
int pointAmount = 150;


int main(int argc, char* argv[])
{
	handleArguments(argc, argv);

	std::vector<Point> points;
	if (filePath != "")
		points = generatePointsFromFile(filePath);
	else
	{
		points = generateRandomPoints(pointAmount);
		// points = generateRandomPointsOnCircle(pointAmount);
	}

	//TODO: Better sorting algorithm (quicksort?).
	// Sort points by their x coordinates.
	std::sort(points.begin(), points.end(), [](Point& a, Point& b) { return a.X == b.X ? a.Y < b.Y : a.X < b.X; });

	//TODO: Fix it smarter maybe, i dont fucking know lol
	//Quick fix removing duplicates because we still dont catch all edge cases
	points.erase(std::unique(points.begin(), points.end()), points.end());
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGTH), "Divide and Conquer");
	Scene scene(window);
	scene.AddDefaultPoints(points);
	scene.Render();
	Hull hull;
	if (visualMode) {
		hull = calculateHullVisual(points, scene);
	}
	else {
		auto start = std::chrono::high_resolution_clock::now();
		hull = calculateHull(points);
		auto end = std::chrono::high_resolution_clock::now();
		std::cout << "Time: " << (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) << " microseconds" << std::endl;
	}
	scene.ClearAll();
	scene.AddDefaultPoints(points);
	scene.AddCorrectHull(hull);
	scene.Render();
	
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}
	}

	//TODO: memory cleanup
}
#pragma region Argument Handling
void handleArguments(int argc, char* argv[])
{
	// Skip first argument (it's the .exe).
	for (unsigned int i = 1; i < argc; i++)
	{
		std::string argType = std::string(argv[i]);
		std::string argData = i + 1 < argc ? std::string(argv[i + 1]) : std::string();

		switch (argumentMap[argType])
		{
		case ArgumentType::LOAD:
			if (argData.empty())
				showWrongArguments();
			filePath = argData;
			i++;
			break;
		case ArgumentType::POINT_AMOUNT:
			if (argData.empty())
				showWrongArguments();
			pointAmount = std::stoi(argData);
			i++;
			break;
		case ArgumentType::VISUAL_MODE:
			visualMode = true;
			break;
		case ArgumentType::HELP:
			showHelp();
			std::exit(0);
			break;
		default:
			showWrongArguments();
			break;
		}
	}
}

void showWrongArguments()
{
	std::cerr << "Wrong arguments. Use arguments as following:" << std::endl;
	showHelp();
	std::exit(1);
}

void showHelp()
{
	std::cout << "--load <file> \t\t\t-> Filename to read from." << std::endl;
	std::cout << "--points [-p] <numberOfPoints> \t-> Number of random points to be generated." << std::endl;
	std::cout << "--visual [-v] \t\t\t-> Show visualization." << std::endl;
	std::cout << "--help \t\t\t\t-> Prints out this message." << std::endl;
}
#pragma endregion

#pragma region Setup Functions
std::vector<Point> generatePointsFromFile(const std::string& filePath)
{
	// Generate points from file.
	std::vector<Point> points;
	if (filePath != "") {
		std::ifstream input;
		input.open(filePath);
		if (!input.is_open()) {
			std::cerr << "Could not open file. Please check the path." << std::endl;
			std::exit(1);
		}
		// Go through each line and get x and y coordinates.
		while (input.peek() != EOF) {
			std::string xAsString, yAsString;
			std::getline(input, xAsString, DATA_DELIMITER);
			std::getline(input, yAsString);
			points.push_back(Point(std::stof(xAsString), std::stof(yAsString)));
		}
	}
	return points;
}
std::vector<Point> generateRandomPoints(unsigned int amount)
{
	std::vector<Point> points;
	for (int i = 0; i < amount; i++) {
		//TODO: use better rand function
		Point point(rand() % WINDOW_WIDTH, rand() % WINDOW_HEIGTH);
		/*bool contains = false;
		for (auto p : points)
		{
			if (p.X == point.X && p.Y == point.Y)
				contains = true;
		}
		if (contains)
			i--;
		else*/
			points.push_back(point);

	}
	return points;
}
std::vector<Point> generateRandomPointsOnCircle(unsigned int amount)
{
	std::vector<Point> points;
	float radius = 200;
	float periphery = M_PI * 2;
	float steps = periphery / amount;
	Point center = Point(WINDOW_WIDTH / 2, WINDOW_HEIGTH / 2);
	for (float angle = 0; angle <= periphery; angle += steps) {
		Point point(center.X + radius * cos(angle), center.Y + radius * sin(angle));
		points.push_back(point);
	}
	return points;
}
#pragma endregion

#pragma region Optimized Hull Calculation
Hull calculateHull(std::vector<Point>& points)
{
	unsigned int half = points.size() / 2;
	std::vector<Point> left(points.begin(), points.begin() + half);
	std::vector<Point> right(points.begin() + half, points.end());

	Hull leftHull;
	Hull rightHull;

	// Split until each vector has 3 or 2 values.
	if (left.size() > 3) {
		leftHull = calculateHull(left);
	}
	else {
		leftHull = generateSmallestHull(left);
	}

	if (right.size() > 3) {
		rightHull = calculateHull(right);
	}
	else {
		rightHull = generateSmallestHull(right);
	}

	Hull hull = merge(leftHull, rightHull);
	return hull;
}

Hull generateSmallestHull(std::vector<Point>& points)
{
	Hull hull = Hull();
	if (points.size() == 1) {
		Node* node = new Node(points[0]);
		node->clockwiseNext = node;
		node->counterclockNext = node;
		hull.left = node;
		hull.right = node;
	}
	else if (points.size() == 2) {
		Node* left = new Node(points[0]);
		Node* right = new Node(points[1]);
		left->clockwiseNext = right;
		left->counterclockNext = right;
		right->clockwiseNext = left;
		right->counterclockNext = left;
		hull.left = left;
		hull.right = right;
	}
	else {
		Node* left = new Node(points[0]);
		Node* middle = new Node(points[1]);
		Node* right = new Node(points[2]);

		// Check if middle node is up or down to determine neighbors.

		bool middleIsUp = isPointLeftOfLine(Line(left, right), middle);

		left->clockwiseNext = middleIsUp ? middle : right;
		left->counterclockNext = middleIsUp ? right : middle;
		middle->clockwiseNext = middleIsUp ? right : left;
		middle->counterclockNext = middleIsUp ? left : right;
		right->clockwiseNext = middleIsUp ? left : middle;
		right->counterclockNext = middleIsUp ? middle : left;

		hull.left = left;
		hull.right = right;
	}
	return hull;
}

Hull merge(Hull left, Hull right) {

	//finding both tangents
	auto upperTangent = findUpperTangentOfHulls(left, right, left.right, right.left);

	auto lowerTangent = findUpperTangentOfHulls(right, left,  right.left, left.right);

	if (upperTangent.first != lowerTangent.second) {
		//deleting nodes that are not connected to the new hull anymore
		auto currentNode = upperTangent.first->clockwiseNext;
		while (currentNode != lowerTangent.second) {
			auto deleteNode = currentNode;
			currentNode = currentNode->clockwiseNext;
			delete deleteNode;
		}
	}
	if (upperTangent.second != lowerTangent.first) {
		auto currentNode = lowerTangent.first->clockwiseNext;
		while (currentNode != upperTangent.second) {
			auto deleteNode = currentNode;
			currentNode = currentNode->clockwiseNext;
			delete deleteNode;
		}
	}

	//connecting points on both tangents to each other, so the hull is correctly connected
	upperTangent.first->clockwiseNext = upperTangent.second;
	upperTangent.second->counterclockNext = upperTangent.first;

	// Edge case "line": Connect to extremes to prevent infinite loop (missing second tangent -> missing path).
	if (upperTangent.first == lowerTangent.second && upperTangent.second == lowerTangent.first) {
		left.left->counterclockNext = right.right;
		right.right->clockwiseNext = left.left;
	}
	else {
		lowerTangent.first->clockwiseNext = lowerTangent.second;
		lowerTangent.second->counterclockNext = lowerTangent.first;
	}

	//creating a new hull and setting left and right to corresponding values of previous hulls, so we dont have to sort again
	Hull newHull;

	newHull.left = left.left;
	newHull.right = right.right;
	return newHull;
}

Line findUpperTangentOfHulls(Hull left, Hull right , Node* leftRight, Node* rightLeft) {

	//starting with the line connecting the opposite extreme points of both hulls
	Line upperTangent(leftRight, rightLeft);

	//checking if the line is already viable for both hull
	bool isUpperTangentOfLeft = isUpperTangentOfHull(upperTangent, left);
	bool isUpperTangentOfRight = isUpperTangentOfHull(upperTangent, right);
	//while it is not viable for both hulls continue trying different points
	while (!isUpperTangentOfLeft || !isUpperTangentOfRight) {
		//while it is not viable for the left hull
		while (!isUpperTangentOfLeft) {
			//move the point on the left hull on step further
			upperTangent.first = upperTangent.first->counterclockNext;
			//and check again
			isUpperTangentOfLeft = isUpperTangentOfHull(upperTangent, left);

		}

		//is new line tangent of right hull
		isUpperTangentOfRight = isUpperTangentOfHull(upperTangent, right);
		//if its not
		while (!isUpperTangentOfRight) {
			//continue moving point on right hull
			upperTangent.second = upperTangent.second->clockwiseNext;
			//and rechecking it until the line is viable
			isUpperTangentOfRight = isUpperTangentOfHull(upperTangent, right);
		}
		//is new line still viable for the left hull
		isUpperTangentOfLeft = isUpperTangentOfHull(upperTangent, left);
	}
	//if it is were done
	return upperTangent;
}

bool isUpperTangentOfHull(Line tangent, Hull hull) {

	auto currentPoint = hull.left;

	if (tangent.first->point.X == tangent.second->point.X && tangent.first->point.Y == tangent.second->point.Y)
		return false;

	//iterating over the whole hull
	do {
		//if a point is left of the line, life cannot be upper tangent of whole whole
		if (isPointLeftOfLine(tangent, currentPoint)) {
			return false;
		}
		currentPoint = currentPoint->clockwiseNext;
	} while (currentPoint != hull.left);
}

#pragma endregion

#pragma region Visual Hull Calculation
Hull calculateHullVisual(std::vector<Point>& points, Scene& scene)
{
	unsigned int half = points.size() / 2;
	std::vector<Point> left(points.begin(), points.begin() + half);
	std::vector<Point> right(points.begin() + half, points.end());
	scene.AddWorkingPoints(points);
	scene.Render();
	scene.ClearWorkingPoints();

	Hull leftHull;
	Hull rightHull;

	// Split until each vector has 3 or 2 values.
	if (left.size() > 3) {
		leftHull = calculateHullVisual(left, scene);
	}
	else {
		leftHull = generateSmallestHullVisual(left, scene);
	}

	if (right.size() > 3) {
		rightHull = calculateHullVisual(right, scene);
	}
	else {
		rightHull = generateSmallestHullVisual(right, scene);
	}

	Hull hull = mergeVisual(leftHull, rightHull, scene);
	return hull;
}

Hull generateSmallestHullVisual(std::vector<Point>& points, Scene& scene)
{
	Hull hull = Hull();
	if (points.size() == 1) {
		Node* node = new Node(points[0]);
		node->clockwiseNext = node;
		node->counterclockNext = node;
		hull.left = node;
		hull.right = node;
	}
	else if (points.size() == 2) {
		Node* left = new Node(points[0]);
		Node* right = new Node(points[1]);
		left->clockwiseNext = right;
		left->counterclockNext = right;
		right->clockwiseNext = left;
		right->counterclockNext = left;
		hull.left = left;
		hull.right = right;

		scene.AddWorkingPoint(left->point);
		scene.AddWorkingPoint(right->point);
		scene.Render();

		scene.AddCorrectPoint(left->point);
		scene.Render();
		scene.AddCorrectPoint(right->point);
		scene.Render();
		scene.AddDefaultLine(Line(left, right));
		scene.Render();
	}
	else {
		Node* left = new Node(points[0]);
		Node* middle = new Node(points[1]);
		Node* right = new Node(points[2]);

		scene.AddWorkingPoint(left->point);
		scene.AddWorkingPoint(right->point);
		scene.AddWorkingPoint(middle->point);
		scene.Render();

		scene.AddCorrectPoint(left->point);
		scene.AddWorkingLine(Line(left, middle));
		scene.Render();
		scene.AddSecondWorkingLine(Line(left, right));
		scene.Render();
		scene.ClearSecondWorkingLines();

		bool middleIsUp = isPointLeftOfLine(Line(left, right), middle);

		if (middleIsUp) {
			scene.ClearWorkingPoints();
			scene.ClearWorkingLines();
			scene.AddDefaultLine(Line(left, middle));
			scene.AddCorrectPoint(middle->point);
			scene.Render();
			scene.AddDefaultLine(Line(middle, right));
			scene.AddCorrectPoint(right->point);
			scene.Render();
		}
		else {
			scene.ClearWorkingLines();
			scene.AddErrorLine(Line(left, middle));
			scene.AddErrorPoint(middle->point);
			scene.Render();
			scene.ClearErrorLines();
			scene.AddDefaultLine(Line(left, right));
			scene.AddCorrectPoint(right->point);
			scene.Render();
			scene.AddDefaultLine(Line(right, middle));
			scene.AddCorrectPoint(middle->point);
			scene.Render();
		}

		// Check if middle node is up or down to determine neighbors.


		left->clockwiseNext = middleIsUp ? middle : right;
		left->counterclockNext = middleIsUp ? right : middle;
		middle->clockwiseNext = middleIsUp ? right : left;
		middle->counterclockNext = middleIsUp ? left : right;
		right->clockwiseNext = middleIsUp ? left : middle;
		right->counterclockNext = middleIsUp ? middle : left;

		hull.left = left;
		hull.right = right;
	}
	scene.ClearAllExtras();
	scene.ClearDefaultLines();
	scene.AddCorrectHull(hull);
	scene.Render();
	return hull;
}

Hull mergeVisual(Hull left, Hull right, Scene& scene) {
	scene.AddCorrectHull(left);
	scene.AddCorrectHull(right);
	scene.Render();

	//finding both tangents

	auto upperTangent = visualFindUpperTangentOfHulls(left, right, left.right, right.left, scene);
	scene.AddWorkingLine(upperTangent);
	scene.Render();

	auto lowerTangent = visualFindUpperTangentOfHulls(right, left, right.left, left.right, scene);
	scene.AddWorkingLine(lowerTangent);
	scene.Render();

	scene.ClearCorrectHulls();
	scene.AddErrorHull(left);
	scene.AddErrorHull(right);
	scene.Render();

	if (upperTangent.first != lowerTangent.second) {
		//deleting nodes that are not connected to the new hull anymore
		auto currentNode = upperTangent.first->clockwiseNext;
		while (currentNode != lowerTangent.second) {
			auto deleteNode = currentNode;
			currentNode = currentNode->clockwiseNext;
			delete deleteNode;
		}
	}
	if (upperTangent.second != lowerTangent.first) {
		auto currentNode = lowerTangent.first->clockwiseNext;
		while (currentNode != upperTangent.second) {
			auto deleteNode = currentNode;
			currentNode = currentNode->clockwiseNext;
			delete deleteNode;
		}
	}

	//connecting points on both tangents to each other, so the hull is correctly connected
	upperTangent.first->clockwiseNext = upperTangent.second;
	upperTangent.second->counterclockNext = upperTangent.first;

	// Edge case "line": Connect to extremes to prevent infinite loop (missing second tangent -> missing path).
	if (upperTangent.first == lowerTangent.second && upperTangent.second == lowerTangent.first) {
		left.left->counterclockNext = right.right;
		right.right->clockwiseNext = left.left;
	}
	else {
		lowerTangent.first->clockwiseNext = lowerTangent.second;
		lowerTangent.second->counterclockNext = lowerTangent.first;
	}

	//creating a new hull and setting left and right to corresponding values of previous hulls, so we dont have to sort again
	Hull newHull;

	newHull.left = left.left;
	newHull.right = right.right;

	scene.ClearErrorHulls();
	scene.ClearWorkingLines();
	scene.AddCorrectHull(newHull);
	scene.Render();
	return newHull;
}

Line visualFindUpperTangentOfHulls(Hull left, Hull right, Node* leftRight, Node* rightLeft, Scene& scene) {

	//starting with the line connecting the opposite extreme points of both hulls
	Line upperTangent(leftRight, rightLeft);

	scene.ClearSecondWorkingLines();
	scene.AddSecondWorkingLine(upperTangent);
	scene.Render();

	//checking if the line is already viable for both hull
	bool isUpperTangentOfLeft = visualIsUpperTangentOfHull(upperTangent, left, scene);
	bool isUpperTangentOfRight = visualIsUpperTangentOfHull(upperTangent, right, scene);
	scene.Render();
	scene.ClearErrorPoints();
	scene.ClearCorrectPoints();
	//while it is not viable for both hulls continue trying different points
	while (!isUpperTangentOfLeft || !isUpperTangentOfRight) {
		//while it is not viable for the left hull
		while (!isUpperTangentOfLeft) {
			//move the point on the left hull on step further
			upperTangent.first = upperTangent.first->counterclockNext;
			scene.ClearSecondWorkingLines();
			scene.AddSecondWorkingLine(upperTangent);
			scene.Render();
			//and check again
			isUpperTangentOfLeft = visualIsUpperTangentOfHull(upperTangent, left, scene);
			scene.Render();
			scene.ClearErrorPoints();
			scene.ClearCorrectPoints();

		}

		//is new line tangent of right hull
		isUpperTangentOfRight = visualIsUpperTangentOfHull(upperTangent, right, scene);
		scene.Render();
		scene.ClearErrorPoints();
		scene.ClearCorrectPoints();
		//if its not
		while (!isUpperTangentOfRight) {
			//continue moving point on right hull
			upperTangent.second = upperTangent.second->clockwiseNext;
			scene.ClearSecondWorkingLines();
			scene.AddSecondWorkingLine(upperTangent);
			scene.Render();
			//and rechecking it until the line is viable
			isUpperTangentOfRight = visualIsUpperTangentOfHull(upperTangent, right, scene);
			scene.Render();
			scene.ClearErrorPoints();
			scene.ClearCorrectPoints();
		}
		//is new line still viable for the left hull
		isUpperTangentOfLeft = visualIsUpperTangentOfHull(upperTangent, left, scene);
		scene.Render();
		scene.ClearErrorPoints();
		scene.ClearCorrectPoints();
	}
	scene.ClearSecondWorkingLines();
	//if it is were done
	return upperTangent;
}

bool visualIsUpperTangentOfHull(Line tangent, Hull hull, Scene& scene) {

	auto currentPoint = hull.left;
	bool upper = true;

	if (tangent.first->point.X == tangent.second->point.X && tangent.first->point.Y == tangent.second->point.Y)
		return false;

	//iterating over the whole hull
	do {
		//if a point is left of the line, life cannot be upper tangent of whole whole
		if (isPointLeftOfLine(tangent, currentPoint)) {
			scene.AddErrorPoint(currentPoint->point);
			upper = false;
		}
		else {
			scene.AddCorrectPoint(currentPoint->point);
		}
		currentPoint = currentPoint->clockwiseNext;
	} while (currentPoint != hull.left);
	return upper;
}
#pragma endregion

#pragma region Utility Functions
bool isPointLeftOfLine(Line line, Node* point) {
	Point a = line.first->point;
	Point b = line.second->point;
	Point p = point->point;
	//if determinant is greater than zero, point is left of the line
	return ((b.X - a.X) * (p.Y - a.Y) - (b.Y - a.Y) * (p.X - a.X)) > 0;
}
#pragma endregion

