#include <SFML/Graphics.hpp>
#include <vector>

#define WINDOWX 800
#define WINDOWY 800
#define getrandom(min, max) ((rand()%(int)(((max) + 1) - (min)))+ (min))

class Point {
public:
	float x, y;
	sf::Color c = sf::Color(255, 255, 255); // Only for debug
	Point() {
		this->x = (float)getrandom(4, WINDOWX - 4);
		this->y = (float)getrandom(4, WINDOWY - 4);
	}

	Point(float x, float y) {
		this->x = x;
		this->y = y;
	}
};

class BoundingBox {
public:
	float x, y;
	float w, h;

	BoundingBox() {
		this->x = 0.0;
		this->y = 0.0;
		this->w = 0.0;
		this->h = 0.0;
	}

	BoundingBox(float x, float y, float w, float h) {
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}

	bool contains(const Point* p) {
		return (
			p->x >= this->x &&
			p->x <= this->x + this->w &&
			p->y >= this->y &&
			p->y <= this->y + this->h);
	}

	bool intersects(const BoundingBox& box) {
		return !(
			box.x >= this->x + this->w ||
			box.x + box.w <= this->x ||
			box.y >= this->y + this->h ||
			box.y + box.h <= this->y
			);
	}

};

class QuadTreeNode {
public:
	BoundingBox boundary = BoundingBox();
	std::vector<Point*> points = {};
	std::vector<QuadTreeNode*> childNodes = {};
	int max_capacity;
	int capacity;
	bool divided = false;

	// Graphics
	sf::RectangleShape rect;

	// Tbh not sure if I can clean this up.
	QuadTreeNode(float BoundingX, float BoundingY, float BoundingW, float BoundingH, int nCapacity) {
		this->boundary.x = BoundingX;
		this->boundary.y = BoundingY;
		this->boundary.w = BoundingW;
		this->boundary.h = BoundingH;
		this->capacity = nCapacity;

		// Graphics
		rect.setPosition(this->boundary.x, this->boundary.y);
		rect.setSize(sf::Vector2f(this->boundary.w, this->boundary.h));
		rect.setOutlineColor(sf::Color::White);
		rect.setOutlineThickness(0.5f);
		rect.setFillColor(sf::Color(255, 255, 255, 0));
	}

	void insert(Point* p) {
		if (!this->boundary.contains(p))
			return;

		if (this->points.size() < this->capacity) { // If we have space for the pointer, push it to its vector.
			this->points.push_back(p);
		}
		else {
			if (!this->divided) {
				this->divided = true;
				this->subdivide(p);
			}
			else { // If this Node already divided itself, and there is no space for a new pointer, try to insert it into its child-nodes.
				if (!this->childNodes.size())
					return;

				this->childNodes[0]->insert(p);
				this->childNodes[1]->insert(p);
				this->childNodes[2]->insert(p);
				this->childNodes[3]->insert(p);
			}
		}

	}

	void subdivide(Point* p) {
		float x = this->boundary.x;
		float y = this->boundary.y;
		float w = this->boundary.w;
		float h = this->boundary.h;

		// This is a mental note to make sure that I delete these Child Nodes when re-rendering the QuadTree.
		this->childNodes.push_back(new QuadTreeNode(x, y, w / 2, h / 2, this->capacity)); // North West
		this->childNodes.push_back(new QuadTreeNode(x + (w / 2), y, w / 2, h / 2, this->capacity)); // North East
		this->childNodes.push_back(new QuadTreeNode(x, y + (h / 2), w / 2, h / 2, this->capacity)); // South West
		this->childNodes.push_back(new QuadTreeNode(x + (w / 2), y + (h / 2), w / 2, h / 2, this->capacity)); // South East
		this->childNodes[0]->insert(p);
		this->childNodes[1]->insert(p);
		this->childNodes[2]->insert(p);
		this->childNodes[3]->insert(p);
	}

	void query(BoundingBox& range, std::vector<Point*> *found) {
		if (!this->boundary.intersects(range)) // If the node isn't in the search area, stop!
			return;

		for (int i = 0; i < this->points.size(); i++) { // For every point in the node
			if (range.contains(this->points[i])) { // If it is in the search area
				found->push_back(this->points[i]); // Add to found array
				this->points[i]->c = sf::Color(255, 0, 0);
			}
			else {
				this->points[i]->c = sf::Color(255, 255, 255);
			}
		}
		if (this->divided) { // If the node has children, check them all for the same thing.
			if (!this->childNodes.size())
				return;

			this->childNodes[0]->query(range, found);
			this->childNodes[1]->query(range, found);
			this->childNodes[2]->query(range, found);
			this->childNodes[3]->query(range, found);
		}
	}

	void draw(sf::RenderWindow& window) {
		window.draw(rect);

		// If node has children draw them too.
		if (this->divided) {
			this->childNodes[0]->draw(window);
			this->childNodes[1]->draw(window);
			this->childNodes[2]->draw(window);
			this->childNodes[3]->draw(window);
		}
		for (int i = 0; i < this->points.size(); i++) { // Plot points
			sf::CircleShape point;
			point.setPosition(this->points[i]->x, this->points[i]->y);
			point.setRadius(4.f);
			point.setFillColor(this->points[i]->c);
			window.draw(point);
		}
	}
};

int main()
{
	sf::RenderWindow window(sf::VideoMode(WINDOWX, WINDOWY), "QuadTree", sf::Style::Close);

	QuadTreeNode qtree = QuadTreeNode(0.f, 0.f, (float)WINDOWX, (float)WINDOWY, 5);

	std::vector<Point*> PointsArray = {};

	BoundingBox searchArea = BoundingBox(350, 350, 160, 160);

	// Green box for search Area.
	sf::RectangleShape searchRect;
	searchRect.setFillColor(sf::Color(0, 255, 0, 50));
	searchRect.setSize(sf::Vector2f(searchArea.w, searchArea.h));

	bool mousePressed = false;
	sf::Event event;
	while (window.isOpen()) {
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear(sf::Color::Black);
		sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

		// Updating Search Area Rectangle
		searchArea.x = mousePos.x - (searchArea.w * 0.5f);
		searchArea.y = mousePos.y - (searchArea.h * 0.5f);
		searchRect.setPosition(searchArea.x, searchArea.y);

		if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
			if (!mousePressed) {
				mousePressed = true;
				// Place down point.
				PointsArray.push_back(new Point(mousePos.x, mousePos.y));
				qtree.insert(PointsArray.back());
			}
		}
		else {
			mousePressed = false;
		}

		std::vector<Point*> foundPoints = {};
		qtree.query(searchArea, &foundPoints);

		qtree.draw(window);
		window.draw(searchRect);
		window.display();

	}
	return 0;
}