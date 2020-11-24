#include <SFML/Graphics.hpp>
#include<iostream>
#include<chrono>

#include "datastructures.h"
#include "scene.h"

#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <set>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGTH 800
#define POINT_GENERATION_BORDER 10

#define DRAW_GRID_SIZE 10

// Delimiter that defines the position of a point in a file (e.g. 10,20).
#define DATA_DELIMITER ','

enum class ArgumentType { LOAD, POINT_AMOUNT, VISUAL_MODE, DRAW_MODE, BENCHMARK_MODE, HELP };

std::map<std::string, ArgumentType> argumentMap{
	{"--load", ArgumentType::LOAD},
	{"--points", ArgumentType::POINT_AMOUNT},
	{"-p", ArgumentType::POINT_AMOUNT},
	{"--visual", ArgumentType::VISUAL_MODE},
	{"-v", ArgumentType::VISUAL_MODE},
	{"--draw", ArgumentType::DRAW_MODE},
	{"--benchmark", ArgumentType::BENCHMARK_MODE},
	{"--help", ArgumentType::HELP},
};


#pragma region Function declarations
void BenchmarkMode();
void DivideAndConquer(std::vector<Point>& points, Scene& scene);
//Setup functions
void handleArguments(int argc, char* argv[]);
void showWrongArguments();
void showHelp();
std::vector<Point> generatePointsFromFile(const std::string& filePath);
std::vector<Point> generateRandomPoints(unsigned int amount);
std::vector<Point> generateRandomPointsOnCircle(unsigned int amount);

//Optimized calculation functions
Hull calculateHull(std::vector<Point>& points, int left, int right);
Hull generateSmallestHull(std::vector<Point>& points, int left, int right);
Hull merge(Hull& left, Hull& right);
Line findUpperTangentOfHulls(Hull& left, Hull& right, Node* leftRight, Node* rightLeft);
bool isUpperTangentOfHull(Line& tangent, Hull& hull);

//Visual calculation functions
Hull calculateHullVisual(std::vector<Point>& points, Scene& scene);
Hull generateSmallestHullVisual(std::vector<Point>& points, Scene& scene);
Hull mergeVisual(Hull left, Hull right, Scene& scene);
Line visualFindUpperTangentOfHulls(Hull left, Hull right, Node* leftRight, Node* rightLeft, Scene& scene);
bool visualIsUpperTangentOfHull(Line tangent, Hull hull, Scene& scene);

//Utility functions
bool isPointLeftOfLine(Line line, Node* point);
#pragma endregion

bool benchmarkMode = true;
unsigned int benchmarkIterations = 3;
unsigned int benchmarkRandomIterations = 2;

bool visualMode = false;
bool drawMode = false;

std::string filePath = "";
//std::string filePath = "..\\Testcases\\LineVertical.txt";
int pointAmount = 100;

int main(int argc, char* argv[])
{
	handleArguments(argc, argv);

	if (benchmarkMode) {
		BenchmarkMode();
		return 0;
	}

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGTH), "Divide and Conquer");
	Scene scene(window);

	std::vector<Point> points;
	// Generate random points, if there are no
	if (!drawMode && filePath != "")
		points = generatePointsFromFile(filePath);
	else if (!drawMode)
	{
		points = generateRandomPoints(pointAmount);
		// points = generateRandomPointsOnCircle(pointAmount);
	}

	if (!drawMode) {
		scene.AddDefaultPoints(points);
		scene.Render();
	}

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			// Allow user drawing.
			if (drawMode) {
				sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
				sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);
				int x = ((std::abs((int)worldPos.x) + DRAW_GRID_SIZE / 2) / DRAW_GRID_SIZE) * DRAW_GRID_SIZE;
				int y = ((std::abs((int)worldPos.y) + DRAW_GRID_SIZE / 2) / DRAW_GRID_SIZE) * DRAW_GRID_SIZE;
				Point mousePos = Point(x, y);
				scene.AddWorkingPoint(mousePos);

				// Add point at current mouse pos.
				if ((event.type == sf::Event::MouseButtonPressed) && (event.key.code == sf::Mouse::Left)) {
					scene.AddDefaultPoint(mousePos);
					points.push_back(mousePos);
				}

				// Remove point at current mouse pos.
				if ((event.type == sf::Event::MouseButtonPressed) && (event.key.code == sf::Mouse::Right)) {
					auto it = points.begin();
					while (it != points.end())
					{
						if (*it == mousePos) {
							points.erase(it);
							break;
						}
						it++;
					}
					scene.AddDefaultPoints(points);
				}
				scene.Render();
				scene.ClearWorkingPoints();
			}

			// Clear window. Only works before and after the algorithm.
			if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::R)) {
				drawMode = true;
				scene.ClearAll();
				scene.IsAnimating = false;
				scene.Render();
				points.clear();
			}

			// Start algorithm.
			if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Enter)) {
				if (points.size() < 2) {
					std::cerr << "At least 2 points are required!" << std::endl;
					continue;
				}
				scene.ClearAll();
				scene.Render();
				DivideAndConquer(points, scene);
				scene.IsAnimating = false;
			}
			// Activate/Deactivate step by step.
			if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::S))
				scene.GoStepByStep = !scene.GoStepByStep;
		}
	}

	//TODO: memory cleanup
}

void BenchmarkMode()
{
	// TODO: Add other point generations (Circle, Row, ...).
	std::vector<unsigned int> pointAmounts = { 10,100,1000,10000,100000,1000000,10000000, 20000000 };
	std::vector<double> averageTimes;
	std::cout << "================== Random numbers ==================" << std::endl;
	std::cout << "Number of Random Iterations: " << benchmarkRandomIterations << std::endl;
	std::cout << "Number of Iterations (per random iteration): " << benchmarkIterations << std::endl;
	std::cout << "====================================================" << std::endl << std::endl;
	std::chrono::duration<double> amountIterationTime = std::chrono::duration<double>();
	for (unsigned int n = 0; n < pointAmounts.size(); n++)
	{
		unsigned int pointAmount = pointAmounts[n];
		std::cout << "Point amount: " << pointAmount;
		// Set random seed.
		srand(time(NULL));
		for (unsigned int i = 0; i < benchmarkRandomIterations; i++)
		{
			// Calculate random values for this iteration.
			std::vector<Point> points = generateRandomPoints(pointAmount);
			// Sort them.
			std::sort(points.begin(), points.end(), [](Point& a, Point& b) { return a < b; });
			// Remove duplicates.
			points.erase(std::unique(points.begin(), points.end()), points.end());
			std::cout << std::endl << "---Random iteration " << i + 1 << " (used points: " << points.size() << ")" << std::endl;
			// Keep track of hull for clean-up.
			Hull hull;
			std::chrono::duration<double> randomIterationTime = std::chrono::duration<double>();
			for (unsigned int j = 0; j < benchmarkIterations; j++)
			{
				// Calculate hull with random values n-times and avg the time.
				auto start = std::chrono::high_resolution_clock::now();
				hull = calculateHull(points, 0, points.size());
				auto end = std::chrono::high_resolution_clock::now();
				// Save time.
				std::chrono::duration<double> time = (end - start);
				randomIterationTime += time;
				std::cout << "------Iteration " << j + 1 << ": " << time.count() << " seconds" << std::endl;

				// Clean-up before next iteration.
				Node* node = hull.left->clockwiseNext;
				while (node->clockwiseNext != hull.left)
				{
					auto deleteNode = node;
					node = node->clockwiseNext;
					delete deleteNode;
				}
				delete hull.left;
			}
			std::cout << "---Random iteration average: " << randomIterationTime.count() / benchmarkIterations << " seconds" << std::endl;
			amountIterationTime += randomIterationTime / benchmarkIterations;
		}
		double averageTime = amountIterationTime.count() / benchmarkRandomIterations;
		averageTimes.push_back(averageTime);
		std::cout << "Complete average time: " << averageTime << " seconds" << std::endl << std::endl;
	}
	// Print out summary.
	std::cout << "===================== Summary ======================" << std::endl;
	for (unsigned int i = 0; i < pointAmounts.size(); i++)
	{
		std::cout << "Number of Points: " << pointAmounts[i] << " - " << "Time: " << averageTimes[i];
		if (i != 0)
			std::cout << " -> factor: " << (pointAmounts[i] / pointAmounts[i - 1]) / (averageTimes[i] / averageTimes[i - 1]);

		std::cout << std::endl;
	}
	std::cout << "====================================================" << std::endl;
}

void DivideAndConquer(std::vector<Point>& points, Scene& scene)
{
	//TODO: Better sorting algorithm (quicksort?).
	// Sort points by their x coordinates. If on same x, sort by y.
	std::sort(points.begin(), points.end(), [](Point& a, Point& b) { return a < b; });

	//TODO: Fix it smarter maybe, i dont fucking know lol
	//Quick fix removing duplicates because we still dont catch all edge cases
	points.erase(std::unique(points.begin(), points.end()), points.end());

	scene.AddDefaultPoints(points);
	scene.GoStepByStep = false;
	scene.Render();

	Hull hull;
	if (visualMode) {
		scene.GoStepByStep = true;
		scene.IsAnimating = true;
		hull = calculateHullVisual(points, scene);
	}
	else {
		auto start = std::chrono::high_resolution_clock::now();
		hull = calculateHull(points, 0, points.size());
		auto end = std::chrono::high_resolution_clock::now();
		std::cout << "Time: " << (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) << " microseconds" << std::endl;
	}

	// Draw result
	scene.ClearAll();
	scene.AddDefaultPoints(points);
	scene.AddCorrectHull(hull);
	scene.Render();
}

#pragma region Argument Handling
void handleArguments(int argc, char* argv[])
{
	// Skip first argument (it's the .exe).
	for (unsigned int i = 1; i < argc; i++)
	{
		std::string argType = std::string(argv[i]);
		std::string argData = i + 1 < argc ? std::string(argv[i + 1]) : std::string();
		std::string argData2 = i + 2 < argc ? std::string(argv[i + 1]) : std::string();

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
		case ArgumentType::DRAW_MODE:
			drawMode = true;
			break;
		case ArgumentType::BENCHMARK_MODE:
			if (argData.empty() || argData2.empty())
				showWrongArguments();
			benchmarkMode = true;
			benchmarkRandomIterations = std::stoi(argData);
			benchmarkIterations = std::stoi(argData2);
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
	std::cout << "--load <file> \t\t\t\t\t-> Filename to read from." << std::endl;
	std::cout << "--points [-p] <numberOfPoints> \t\t\t-> Number of random points to be generated." << std::endl;
	std::cout << "--visual [-v] \t\t\t\t\t-> Show visualization." << std::endl;
	std::cout << "--draw \t\t\t\t\t\t-> Allows the user to add points with the mouse." << std::endl;
	std::cout << "--benchmark <randomIterations> <iterations> \t-> Runs a benchmark by calculating the hull with different amounts of points and multiple iterations." << std::endl;
	std::cout << "--help \t\t\t\t\t\t-> Prints out this message." << std::endl;
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
		float x = POINT_GENERATION_BORDER + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (WINDOW_WIDTH - POINT_GENERATION_BORDER - POINT_GENERATION_BORDER)));
		float y = POINT_GENERATION_BORDER + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (WINDOW_HEIGTH - POINT_GENERATION_BORDER - POINT_GENERATION_BORDER)));

		Point point(x, y);
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

Hull calculateHull(std::vector<Point>& points, int left, int right)
{
	int pointSize = right - left;
	// If smaller than 4, just calculate hull and return it. No need for merging.
	if (pointSize <= 3) {
		return generateSmallestHull(points, left, right);
	}
	//else split and merge
	int middle = left + (pointSize * 0.5f);
	Hull leftHull = calculateHull(points, left, middle);
	Hull rightHull = calculateHull(points, middle, right);
	Hull hull = merge(leftHull, rightHull);
	return hull;
}

Hull generateSmallestHull(std::vector<Point>& points, int left, int right)
{
	int pointSize = right - left;
	Hull hull = Hull();

	if (pointSize == 2) {
		Node* leftNode = new Node(points[left]);
		Node* rightNode = new Node(points[right - 1]);
		leftNode->clockwiseNext = rightNode;
		leftNode->counterclockNext = rightNode;
		rightNode->clockwiseNext = leftNode;
		rightNode->counterclockNext = leftNode;
		hull.left = leftNode;
		hull.right = rightNode;
	}
	else if (pointSize == 3) {
		Node* leftNode = new Node(points[left]);
		Node* middleNode = new Node(points[left + 1]);
		Node* rightNode = new Node(points[right - 1]);

		// Check if middle node is up or down to determine neighbors.
		bool middleIsUp = isPointLeftOfLine(Line(leftNode, rightNode), middleNode);
		if (middleIsUp) {
			leftNode->clockwiseNext = middleNode;
			leftNode->counterclockNext = rightNode;
			middleNode->clockwiseNext = rightNode;
			middleNode->counterclockNext = leftNode;
			rightNode->clockwiseNext = leftNode;
			rightNode->counterclockNext = middleNode;
		}
		else {
			leftNode->clockwiseNext = rightNode;
			leftNode->counterclockNext = middleNode;
			middleNode->clockwiseNext = leftNode;
			middleNode->counterclockNext = rightNode;
			rightNode->clockwiseNext = middleNode;
			rightNode->counterclockNext = leftNode;
		}

		hull.left = leftNode;
		hull.right = rightNode;
	}
	else {
		Node* node = new Node(points[left]);
		node->clockwiseNext = node;
		node->counterclockNext = node;
		hull.left = node;
		hull.right = node;
	}
	return hull;
}

Hull merge(Hull& left, Hull& right) {

	//finding both tangents
	auto upperTangent = findUpperTangentOfHulls(left, right, left.right, right.left);
	auto lowerTangent = findUpperTangentOfHulls(right, left, right.left, left.right);

	//memory managment makes it run slower obviously
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

Line findUpperTangentOfHulls(Hull& left, Hull& right, Node* leftRight, Node* rightLeft) {

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

bool isUpperTangentOfHull(Line& tangent, Hull& hull) {


	// TODO: not needed if we only allow unique points
	// removing for now
	//if (tangent.first->point.X == tangent.second->point.X && tangent.first->point.Y == tangent.second->point.Y)
		//return false;

	//iterating over the upper hull

	auto currentPoint = hull.left;
	do {
		//if a point is left of the line, life cannot be upper tangent of whole whole
		if (isPointLeftOfLine(tangent, currentPoint)) {
			return false;
		}
		currentPoint = currentPoint->clockwiseNext;
	} while (currentPoint != hull.left);
	return true;
}

#pragma endregion

#pragma region Visual Hull Calculation
Hull calculateHullVisual(std::vector<Point>& points, Scene& scene)
{
	if (points.size() <= 3) {
		return generateSmallestHullVisual(points, scene);
	}
	unsigned int half = points.size() / 2;
	std::vector<Point> left(points.begin(), points.begin() + half);
	std::vector<Point> right(points.begin() + half, points.end());
	scene.AddWorkingPoints(points);
	scene.Render();
	scene.ClearWorkingPoints();

	Hull leftHull;
	Hull rightHull;

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

	// TODO: not needed if we only allow unique points
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

