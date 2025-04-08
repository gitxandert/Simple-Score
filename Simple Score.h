#pragma once

#include "resource.h"
#include <gdiplus.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <memory>

using namespace Gdiplus;

class Shape
{
public:
	Shape(Color color, int width, bool filled) : m_color(color), m_width(width), m_filled(filled)
	{
		setTools(color, width);
	}

	void setTools(Color color, int width)
	{
		m_color = color;
		m_width = width;
		m_brush = std::make_unique<SolidBrush>(color);
		m_pen = std::make_unique<Pen>(color, width);
	}

	virtual void draw(Graphics& graphics) const = 0; // Pure virtual function
	virtual bool hitTest(int x, int y) = 0; // check if point is within the shape
	virtual bool netTest(PointF tlCorn, PointF brCorn) = 0; // check if shape is within the selection rectangle
	virtual void move(float dx, float dy) = 0; // move shape
	virtual PointF getPosition() = 0; // return position of shape (relative to click point)
	virtual PointF getCorner() const = 0; // return corner of shape
	virtual int getLength() const = 0; // return length of shape ("width" is reserved for thickness)
	virtual int getHeight()const  = 0; //return height of shape
	virtual ~Shape() = default; // Virtual destructor for proper cleanup

	Color getColor() const { return m_color; }
	int getWidth() const { return m_width; }

	void setColor(Color color)
	{ 
		m_color = color;
		setTools(m_color, m_width);
	}

	virtual void setWidth(int w) = 0;

	void setFillMode(bool f) { m_filled = f; }

	void setSelX(int x) { m_selX = x; }
	void setSelY(int y) { m_selY = y; }
	int getSelX() const { return m_selX; }
	int getSelY() const { return m_selY; }

	const Pen* getPen() const { return m_pen.get(); }

	const SolidBrush* getBrush() const { return m_filled ? m_brush.get() : nullptr; }

	bool isFilled() const { return m_filled; }

	void setLock(bool b) { m_locked = b; }

	bool isLocked() const { return m_locked; }

private:
	Color m_color;
	int m_width;
	int m_selX;
	int m_selY;

	bool m_filled;
	bool m_locked{ false };
	std::unique_ptr<Pen> m_pen;
	std::unique_ptr<SolidBrush> m_brush;
};

class LineShape : public Shape
{
public:
	LineShape(PointF a, PointF b, Color color, int width) : m_a(a), m_b(b), Shape(color, width, 0) { setHiLo(); }

	~LineShape() override = default;

	void setHiLo()
	{
		if (m_a.X >= m_b.X)
		{
			hiX = m_a.X;
			loX = m_b.X;
		}
		else
		{
			hiX = m_b.X;
			loX = m_a.X;
		}

		if (m_a.Y >= m_b.Y)
		{
			hiY = m_a.Y;
			loY = m_b.Y;
		}
		else
		{
			hiY = m_b.Y;
			loY = m_a.Y;
		}
	}
	

	void setWidth(int w)
	{
		setTools(getColor(), w);
	}

	bool hitTest(int x, int y) override
	{
		if (x <= hiX && x >= loX && y <= hiY && y >= loY)
		{
			setSelX(x);
			setSelY(y);

			return true;
		}

		return false;
	}

	bool netTest(PointF tlCorn, PointF brCorn) override
	{
		if (loX > tlCorn.X && loY > tlCorn.Y && hiX < brCorn.X && hiY < brCorn.Y)
		{
			setSelX(loX);
			setSelY(loY);

			return true;
		}

		return false;
	}

	PointF getPosition() override { return PointF(getSelX(), getSelY()); }
	PointF getCorner() const override { return PointF(loX, loY); }
	int getLength() const { return hiX - loX; }
	int getHeight() const { return hiY - loY; }

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY() };
		
		m_a.X += moveX;
		m_b.X += moveX;
		m_a.Y += moveY;
		m_b.Y += moveY;

		setSelX(dx);
		setSelY(dy);

		setHiLo();
	}

	void draw(Graphics& graphics) const override
	{
		graphics.DrawLine(getPen(), m_a.X, m_a.Y, m_b.X, m_b.Y);
	}

private:
	PointF m_a;
	PointF m_b;

	int hiX, loX, hiY, loY;
};

class EllipseShape : public Shape
{
public:
	EllipseShape(PointF c, REAL width, REAL height, Color color, int w, bool f) : m_center(c), m_width(width), m_height(height), Shape(color, w, f) {}
	~EllipseShape() override = default;

	void draw(Graphics& graphics) const override
	{
		if (isFilled())
		{
			graphics.FillEllipse(getBrush(), m_center.X, m_center.Y, m_width, m_height);
		}
		else
		{
			graphics.DrawEllipse(getPen(), m_center.X, m_center.Y, m_width, m_height);
		}
	}

	void setWidth(int w)
	{
		setTools(getColor(), w);
	}

	bool hitTest(int x, int y) override
	{
		if ((x <= m_center.X + m_width) && (x >= m_center.X) && (y <= m_center.Y + m_height) && (y >= m_center.Y))
		{
			setSelX(x);
			setSelY(y);

			return true;
		}

		return false;
	}

	bool netTest(PointF tlCorn, PointF brCorn) override
	{
		if (m_center.X > tlCorn.X && m_center.Y > tlCorn.Y && m_center.X + m_width < brCorn.X && m_center.Y + m_height < brCorn.Y)
		{
			setSelX(m_center.X);
			setSelY(m_center.Y);

			return true;
		}

		return false;
	}

	PointF getPosition() override { return PointF(getSelX(), getSelY()); }
	PointF getCorner() const override { return m_center; }
	int getLength() const { return m_width; }
	int getHeight() const { return m_height; }

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY()};

		m_center.X += moveX;
		m_center.Y += moveY;

		setSelX(dx);
		setSelY(dy);
	}

private:
	PointF m_center;
	REAL m_width;
	REAL m_height;

};

class RectShape : public Shape
{
public:
	RectShape(PointF corner, int width, int height, Color color, int w, bool f) : m_corner(corner), m_width(width), m_height(height), Shape(color, w, f) {}
	~RectShape() override = default;

	void draw(Graphics& graphics) const override
	{
		if (isFilled())
		{
			graphics.FillRectangle(getBrush(), static_cast<int>(m_corner.X), static_cast<int>(m_corner.Y), m_width, m_height);
		}
		else
		{
			graphics.DrawRectangle(getPen(), static_cast<int>(m_corner.X), static_cast<int>(m_corner.Y), m_width, m_height);
		}
	}

	void setWidth(int w)
	{
		setTools(getColor(), w);
	}

	bool hitTest(int x, int y) override
	{
		if ((x <= (m_corner.X + m_width)) && (x >= m_corner.X) && (y <= (m_corner.Y + m_height)) && (y >= m_corner.Y))
		{
			setSelX(x);
			setSelY(y);

			return true;
		}

		return false;
	}

	bool netTest(PointF tlCorn, PointF brCorn) override
	{
		if(m_corner.X > tlCorn.X && m_corner.Y > tlCorn.Y && m_corner.X + m_width < brCorn.X && m_corner.Y + m_height < brCorn.Y)
		{
			setSelX(m_corner.X);
			setSelY(m_corner.Y);

			return true;
		}

		return false;
	}

	PointF getPosition() override { return PointF(getSelX(), getSelY()); }
	PointF getCorner() const override { return m_corner; }
	int getLength() const { return m_width; }
	int getHeight() const { return m_height; }

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY() };

		m_corner.X += moveX;
		m_corner.Y += moveY;

		setSelX(dx);
		setSelY(dy);
	}

private:
	PointF m_corner;
	int m_width;
	int m_height;
};

class TriShape : public Shape
{
public:
	TriShape(PointF p1, PointF p2, PointF p3, Color color, int w, bool f) : Shape(color, w, f)
	{
		m_points[0] = p1;
		m_points[1] = p2;
		m_points[2] = p3;

		setHiLo(p1, p2, p3);
	}
	~TriShape() override = default;

	void draw(Graphics& graphics) const override
	{
		if (isFilled())
		{
			graphics.FillPolygon(getBrush(), m_points, 3);
		}
		else
		{
			graphics.DrawPolygon(getPen(), m_points, 3);
		}
	}

	void setWidth(int w)
	{
		setTools(getColor(), w);
	}

	bool hitTest(int x, int y) override
	{
		PointF p(x, y);

		float areaO{ triangleArea(m_points[0], m_points[1], m_points[2]) };

		float area1{ triangleArea(p, m_points[1], m_points[2]) };
		float area2{ triangleArea(m_points[0], p, m_points[2]) };
		float area3{ triangleArea(m_points[0], m_points[1], p) };

		if ((area1 + area2 + area3) <= areaO + 100)
		{
			setSelX(x);
			setSelY(y);

			return true;
		}

		return false;
	}

	bool netTest(PointF tlCorn, PointF brCorn) override
	{
		if (m_loX > tlCorn.X && m_loY > tlCorn.Y && m_hiX < brCorn.X && m_hiY < brCorn.Y)
		{
			setSelX(m_loX);
			setSelY(m_loY);

			return true;
		}

		return false;
	}

	PointF getCorner() const override { return PointF(m_loX, m_loY); }
	int getLength() const { return m_hiX - m_loX; }
	int getHeight() const { return m_hiY - m_loY; }
	PointF getPosition() override { return PointF(getSelX(), getSelY()); }

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY() };

		m_points[0].X += moveX;
		m_points[1].X += moveX;
		m_points[2].X += moveX;

		m_points[0].Y += moveY;
		m_points[1].Y += moveY;
		m_points[2].Y += moveY;

		setSelX(dx);
		setSelY(dy);

		setHiLo(m_points[0], m_points[1], m_points[2]);
	}

private:
	PointF m_points[3];

	int m_hiX, m_loX, m_hiY, m_loY;

	float triangleArea(PointF a, PointF b, PointF c) const
	{
		return std::abs((a.X * (b.Y - c.Y) + b.X * (c.Y - a.Y) + c.X * (a.Y - b.Y)) / 2.0f);
	}

	void setHiLo(PointF a, PointF b, PointF c)
	{
		if (a.X > b.X && a.X > c.X)
		{
			m_hiX = a.X;
		}
		else if (b.X > c.X)
		{
			m_hiX = b.X;
		}
		else
		{
			m_hiX = c.X;
		}

		if (a.X < b.X && a.X < c.X)
		{
			m_loX = a.X;
		}
		else if (b.X < c.X)
		{
			m_loX = b.X;
		}
		else
		{
			m_loX = c.X;
		}

		if (a.Y > b.Y && a.Y > c.Y)
		{
			m_hiY = a.Y;
		}
		else if (b.Y > c.Y)
		{
			m_hiY = b.Y;
		}
		else
		{
			m_hiY = c.Y;
		}

		if (a.Y < b.Y && a.Y < c.Y)
		{
			m_loY = a.Y;
		}
		else if (b.Y < c.Y)
		{
			m_loY = b.Y;
		}
		else
		{
			m_loY = c.Y;
		}
	}
};

class SketchShape : public Shape
{
public:
	SketchShape(std::vector<PointF> points, Color color, int w) : m_points(points), Shape(color, w, 0)
	{
		setHiLo(points);
	}

	~SketchShape() override = default;

	void draw(Graphics& graphics) const override
	{
		graphics.DrawCurve(getPen(), m_points.data(), m_points.size());
	}

	void setWidth(int w)
	{
		setTools(getColor(), w);
	}

	bool hitTest(int x, int y) override
	{
		if (x <= m_hiX && x >= m_loX && y <= m_hiY && y >= m_loY)
		{
			setSelX(x);
			setSelY(y);

			return true;
		}

		return false;
	}

	bool netTest(PointF tlCorn, PointF brCorn) override
	{
		if (m_loX > tlCorn.X && m_loY > tlCorn.Y && m_hiX < brCorn.X && m_hiY < brCorn.Y)
		{
			setSelX(m_loX);
			setSelY(m_loY);

			return true;
		}

		return false;
	}

	PointF getPosition() override { return PointF(getSelX(), getSelY()); }
	PointF getCorner() const override { return PointF(m_loX, m_loY); }
	int getLength() const { return m_hiX - m_loX; }
	int getHeight() const { return m_hiY - m_loY; }

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY() };

		for (auto& mp : m_points)
		{
			mp.X += moveX;
			mp.Y += moveY;
		}

		m_loX += moveX;
		m_hiX += moveX;
		m_loY += moveY;
		m_hiY += moveY;

		setSelX(dx);
		setSelY(dy);
	}

private:
	std::vector<PointF> m_points;
	int m_loX{ 10000 };
	int m_hiX{ -10000 };
	int m_loY{ 10000 };
	int m_hiY{ -10000 };
	int m_selX{ 0 };
	int m_selY{ 0 };

	void setHiLo(std::vector<PointF>& points)
	{
		for (const auto& p : points)
		{
			if (p.X > m_hiX)
			{
				m_hiX = p.X;
			}
			else if (p.X < m_loX)
			{
				m_loX = p.X;
			}

			if (p.Y > m_hiY)
			{
				m_hiY = p.Y;
			}
			else if (p.Y < m_loY)
			{
				m_loY = p.Y;
			}
		}
	}
};

class DRAW_SHAPES
{
public:
	DRAW_SHAPES()
	{
		m_brush = CreateSolidBrush(RGB(0, 0, 0));
	}

	~DRAW_SHAPES()
	{
		DeleteObject(m_brush);
		m_brush = NULL;
	}

	enum SHAPE
	{
		NONE,
		LINE,
		RECT,
		ELLIPSE,
		TRIANGLE,
		SKETCH,
		SELECT
	};

	enum DIRECTION
	{
		LEFT,
		DOWN,
		RIGHT,
		UP
	};

	void setShape(SHAPE shape) { m_shape = shape; }
	SHAPE getShape() const { return m_shape; }
	void addShape(std::shared_ptr<Shape> shape) { m_shapes.push_back(shape); }

	void setWidth(int w) { m_width = w; }
	int getWidth() const { return m_width; }

	void setAlpha(uint8_t a) { m_alpha = a; }
	void setR(uint8_t r) { m_r = r; }
	void setG(uint8_t g) { m_g = g; }
	void setB(uint8_t b) { m_b = b; }

	Color getColor() const { return Color(m_alpha, m_r, m_g, m_b); }

	uint8_t getAlpha() const { return m_alpha; }

	void drawStoredShapes(Graphics& graphics)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (const auto& shape : m_shapes)
		{
			shape->draw(graphics);
		}
	}

	void setOrigin(PointF origin){ m_origin = origin; }

	REAL getOX() const{ return m_origin.X; }
	REAL getOY() const{ return m_origin.Y; }

	void setCurrent(PointF current) { m_current = current; }

	REAL getCX() const { return m_current.X; }
	REAL getCY() const { return m_current.Y; }

	void setDrawing(bool b) { m_drawing = b; }
	bool isDrawing() const { return m_drawing; }

	void setFillMode(bool b) { m_fill = b; }
	bool getFillMode() const { return m_fill; }

	void setSelectColor()
	{
		if (m_selected)
		{
			if (!m_selected->isLocked())
			{
				m_selected->setColor(Color(m_alpha, m_r, m_g, m_b));
			}
		}
		else if (m_selected_shapes.size() > 0)
		{
			for (auto& shape : m_selected_shapes)
			{
				if (!shape->isLocked())
				{
					shape->setColor(Color(m_alpha, m_r, m_g, m_b));
				}
			}
		}
	}

	void setSelectWidth()
	{
		if (m_selected)
		{
			if (!m_selected->isLocked())
			{
				m_selected->setWidth(m_width);
			}
		}
		else if (m_selected_shapes.size() > 0)
		{
			for (auto& shape : m_selected_shapes)
			{
				if (!shape->isLocked())
				{
					shape->setWidth(m_width);
				}
			}
		}
	}

	void setSelectFill()
	{
		if (m_selected)
		{
			if (!m_selected->isLocked())
			{
				m_selected->setFillMode(m_fill);
			}
		}
		else if (m_selected_shapes.size() > 0)
		{
			for (auto& shape : m_selected_shapes)
			{
				if (!shape->isLocked())
				{
					shape->setFillMode(m_fill);
				}
			}
		}
	}

	void drawCurrentShape(Graphics& graphics);

	void addSketch()
	{ 
		std::lock_guard<std::mutex> lock(mutex_);
		m_shapes.push_back(std::make_shared<SketchShape>(m_sketch_points, getColor(), m_width));
		m_sketch_points.clear();
	}

	HBRUSH getBrush()
	{ 
		LOGBRUSH lb;
		GetObject(m_brush, sizeof(LOGBRUSH), &lb);
		COLORREF color = lb.lbColor; // color value of current brush

		if (color != RGB(m_r, m_g, m_b))
		{
			DeleteObject(m_brush);

			m_brush = CreateSolidBrush(RGB(m_r, m_g, m_b));
		}

		return m_brush;
	}

	void setSelect() { m_selected = m_shapes.back().get(); }

	Shape* getSelected() const { return m_selected; }

	void unSelect() { m_selected = NULL; m_selected_shapes.clear(); }

	void selectShape(int x, int y)
	{
		for (auto& shape : m_shapes)
		{
			if (shape->hitTest(x, y))
			{
				m_selected = shape.get();
				m_isMoving = true;
				return;
			}
		}
		
		m_selected = NULL;
		m_isSelecting = true;
		m_selected_shapes.clear();
	}

	void selectShapes()
	{
		for (auto& shape : m_shapes)
		{
			if (shape->netTest(m_origin, m_current))
			{
				m_selected_shapes.push_back(shape);
			}
		}
	}

	void selectRect(Graphics& graphics) const
	{
		Pen selectPen(Color(255, 0, 0, 100), 2.0f);
		selectPen.SetDashStyle(DashStyleDash);

		if (m_selected)
		{
			PointF corner{ m_selected->getCorner() };
			REAL length{ static_cast<REAL>(m_selected->getLength()) };
			REAL height{ static_cast<REAL>(m_selected->getHeight()) };

			graphics.DrawRectangle(&selectPen, corner.X, corner.Y, length, height);
		}
		else if (m_isSelecting || m_selected_shapes.size() > 0)
		{
			graphics.DrawRectangle(&selectPen, m_origin.X, m_origin.Y, m_current.X - m_origin.X, m_current.Y - m_origin.Y);
		}
	}

	bool isMoving() const { return m_isMoving; }

	bool isSelecting() const { return m_isSelecting; }

	void stopSelecting() { m_isSelecting = false; }

	void dropShape() { m_isMoving = false; }

	void moveShape(int x, int y)
	{ 
		if (!m_selected->isLocked())
		{
			m_selected->move(x, y);
		}
	}

	void moveShapes(DIRECTION direction)
	{
		bool shapesCanMove{ false };

		for (auto& shape : m_selected_shapes)
		{
			if (!shape->isLocked())
			{
				shapesCanMove = true;

				PointF position{ shape->getPosition() };
				switch (direction)
				{
				case LEFT:
					--position.X;
					break;
				case DOWN:
					++position.Y;
					break;
				case RIGHT:
					++position.X;
					break;
				case UP:
					--position.Y;
					break;
				default:
					return;
				}

				shape->move(position.X, position.Y);
			}
		}

		if (shapesCanMove)
		{
			switch (direction)
			{
			case LEFT:
				--m_origin.X;
				--m_current.X;
				break;
			case DOWN:
				++m_origin.Y;
				++m_current.Y;
				break;
			case RIGHT:
				++m_origin.X;
				++m_current.X;
				break;
			case UP:
				--m_origin.Y;
				--m_current.Y;
				break;
			default:
				return;
			}
		}
	}

	void deleteShapes()
	{
		m_shapes.clear();
		m_selected_shapes.clear();
	}

	void removeShape()
	{
		if (m_selected)
		{
			if (m_shapes.size() == 1)
			{
				if (!m_shapes[0]->isLocked())
				{
					m_shapes.pop_back();
					m_selected = NULL;
				}
			}
			else if (!m_selected->isLocked())
			{

				// Find the shared_ptr that matches m_selected
				auto it = std::find_if(m_shapes.begin(), m_shapes.end(),
					[this](const std::shared_ptr<Shape>& shape) {
						return shape.get() == m_selected; // Compare pointers
					});

				// If found, erase it
				if (it != m_shapes.end())
				{
					m_shapes.erase(it);
					m_selected = NULL;
				}
			}
		}
		else if (m_selected_shapes.size() > 0)
		{
			bool shapesAreUnlocked{ false };

			for (auto& selected_shape : m_selected_shapes)
			{
				if (!selected_shape->isLocked())
				{
					shapesAreUnlocked = true;

					auto it = std::find_if(m_shapes.begin(), m_shapes.end(),
						[selected_shape](const std::shared_ptr<Shape>& shape) {
							return shape.get() == selected_shape.get();
						});

					m_shapes.erase(it);

					selected_shape = NULL;
				}
			}

			if (shapesAreUnlocked)
			{
				m_selected_shapes.clear();
			}
		}
	}

	void lockSelectedShape()
	{
		if (m_selected)
		{
			m_selected->setLock(true);
		}
		else if (m_selected_shapes.size() > 0)
		{
			for (auto& shape : m_selected_shapes)
			{
				shape->setLock(true);
			}
		}
	}

	void unlockSelectedShape()
	{
		if (m_selected)
		{
			m_selected->setLock(false);
		}
		else if (m_selected_shapes.size() > 0)
		{
			for (auto& shape : m_selected_shapes)
			{
				shape->setLock(false);
			}
		}
	}

	void popUp()
	{
		if (!m_selected->isLocked())
		{
			// Find the shared_ptr that matches m_selected
			auto it = std::find_if(m_shapes.begin(), m_shapes.end(),
				[this](const std::shared_ptr<Shape>& shape) {
					return shape.get() == m_selected; // Compare pointers
				});

			// If not at the end, pop up
			if (it != m_shapes.end() - 1)
			{
				std::iter_swap(it, std::next(it));
			}
		}
	}

	void pushDown()
	{
		if (!m_selected->isLocked())
		{
			// Find the shared_ptr that matches m_selected
			auto it = std::find_if(m_shapes.begin(), m_shapes.end(),
				[this](const std::shared_ptr<Shape>& shape) {
					return shape.get() == m_selected; // Compare pointers
				});

			// If not at the begin, push down
			if (it != m_shapes.begin())
			{
				std::iter_swap(it, std::prev(it));
			}
		}
	}

private:
	std::vector<std::shared_ptr<Shape>> m_shapes;
	std::mutex mutex_;
	std::vector<PointF> m_sketch_points;

	SHAPE m_shape{ NONE };
	uint8_t m_alpha{ 255 };
	uint8_t m_r{ 0 };
	uint8_t m_g{ 0 };
	uint8_t m_b{ 0 };
	int m_width{ 2 };

	bool m_drawing{ false };
	bool m_fill{ false };
	PointF m_origin;
	PointF m_current;

	HBRUSH m_brush;

	Shape* m_selected{ nullptr };
	bool m_isMoving{ false };
	bool m_isSelecting{ false };
	std::vector<std::shared_ptr<Shape>> m_selected_shapes;
};

inline void DRAW_SHAPES::drawCurrentShape(Graphics& graphics)
{
	switch (m_shape)
	{
	case LINE:
		{
			LineShape line(m_origin, m_current, getColor(), m_width);
			line.draw(graphics);
			break;
		}
	case RECT:
		{
			RectShape rect(m_origin, m_current.X - m_origin.X, m_current.Y - m_origin.Y, getColor(), m_width, getFillMode());
			rect.draw(graphics);
			break;
		}
	case ELLIPSE:
		{
			EllipseShape ellipse(m_origin, m_current.X - m_origin.X, m_current.Y - m_origin.Y, getColor(), m_width, getFillMode());
			ellipse.draw(graphics);
			break;
		}
	case TRIANGLE:
		{
			TriShape triangle(m_current, m_origin, PointF(m_current.X, m_origin.Y), getColor(), m_width, getFillMode());
			triangle.draw(graphics);
			break;
		}
	case SKETCH:
		{
			m_sketch_points.push_back(m_current);

			if (m_sketch_points.size() > 1)
			{
				SketchShape sketch(m_sketch_points, getColor(), m_width);
				sketch.draw(graphics);
			}

			break;
		}
	}
}

class Measure : public Shape
{
public:
	Measure(PointF tlCorner, int l) : m_tlCorner(tlCorner), m_length(l), Shape(Color(255, 0, 0, 0), 1, 0) {}

	void draw(Graphics& graphics) const
	{
		graphics.DrawLine(getPen(), m_tlCorner, PointF(m_tlCorner.X, m_tlCorner.Y + m_size)); // left edge; could make optional

		graphics.DrawLine(getPen(), m_tlCorner, PointF(m_tlCorner.X + m_length, m_tlCorner.Y));
		graphics.DrawLine(getPen(), PointF(m_tlCorner.X, m_tlCorner.Y + (m_size / 4)), PointF(m_tlCorner.X + m_length, m_tlCorner.Y + (m_size / 4)));
		graphics.DrawLine(getPen(), PointF(m_tlCorner.X, m_tlCorner.Y + (m_size / 2)), PointF(m_tlCorner.X + m_length, m_tlCorner.Y + (m_size / 2)));
		graphics.DrawLine(getPen(), PointF(m_tlCorner.X, m_tlCorner.Y + (m_size * 3 / 4)), PointF(m_tlCorner.X + m_length, m_tlCorner.Y + (m_size * 3 / 4)));
		graphics.DrawLine(getPen(), PointF(m_tlCorner.X, m_tlCorner.Y + m_size), PointF(m_tlCorner.X + m_length, m_tlCorner.Y + m_size));

		graphics.DrawLine(getPen(), PointF(m_tlCorner.X + m_length, m_tlCorner.Y), PointF(m_tlCorner.X + m_length, m_tlCorner.Y + m_size)); // right edge; same as above
	}

	void setWidth(int w)
	{
		m_size = w * 4;
	}

	bool hitTest(int x, int y) override
	{
		if (x > m_tlCorner.X && x < m_tlCorner.X + m_length && y > m_tlCorner.Y && y < m_tlCorner.Y + m_size)
		{
			setSelX(x);
			setSelY(y);

			return true;
		}
		return false;
	}

	bool netTest(PointF tlCorner, PointF brCorner) override
	{
		if (m_tlCorner.X > tlCorner.X && m_tlCorner.Y > tlCorner.Y && m_tlCorner.X + m_length < brCorner.X && m_tlCorner.Y + m_size < brCorner.Y)
		{
			setSelX(m_tlCorner.X);
			setSelY(m_tlCorner.Y);
			return true;
		}
		return false;
	}

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY() };

		m_tlCorner.X += moveX;
		m_tlCorner.Y += moveY;

		setSelX(dx);
		setSelY(dy);
	}

	PointF getPosition() override { return m_tlCorner; }
	PointF getCorner() const override { return m_tlCorner; }
	int getLength() const { return m_length; }
	int getHeight() const { return m_size; }
	~Measure() override = default;

private:
	PointF m_tlCorner;
	int m_length{};
	int m_size{ 28 };
};

class Symbol : public Shape
{
public:
	Symbol(PointF corner, int width, std::wstring symbol) : m_corner(corner), m_width(width), m_symbol(symbol), Shape(Color(255, 0, 0, 0), 1, 1) {}

	void draw(Graphics& graphics) const
	{
		double size{ std::sqrt(std::pow(m_width, 2) + std::pow(m_width, 2)) };
		PointF drawPoint(m_corner.X - size / 10.0f, m_corner.Y - size / 1.5);

		Font font(L"Leland", m_width, FontStyleRegular, UnitPixel);
		graphics.DrawString(m_symbol.c_str(), -1, &font, drawPoint, getBrush());
	}

	void setWidth(int w)
	{
		m_width = w;
	}

	bool hitTest(int x, int y) override
	{
		if (x > m_corner.X && x < m_corner.X + m_width && y > m_corner.Y && y < m_corner.Y + m_width)
		{
			setSelX(x);
			setSelY(y);

			return true;
		}
		return false;
	}

	bool netTest(PointF tlCorner, PointF brCorner) override
	{ 
		if (m_corner.X > tlCorner.X && m_corner.Y > tlCorner.Y && m_corner.X + m_width < brCorner.X && m_corner.Y + m_width < brCorner.Y)
		{
			setSelX(m_corner.X);
			setSelY(m_corner.Y);
			return true;
		}
		return false;
	}

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY() };

		m_corner.X += moveX;
		m_corner.Y += moveY;

		setSelX(dx);
		setSelY(dy);
	}

	PointF getPosition() override { return m_corner; }
	PointF getCorner() const override { return m_corner; }
	int getLength() const { return m_width; }
	int getHeight() const { return m_width; }
	~Symbol() override = default;

private:
	PointF m_corner;
	std::wstring m_symbol;
	int m_width;
};

class Text : public Shape
{
public:
	Text(PointF corner, int width, const std::wstring& font=L"Arial") : m_corner(corner), m_width(width), m_font(font), Shape(Color(255, 0, 0, 0), 1, 1) {}

	void draw(Graphics& graphics) const
	{
		double size{ std::sqrt(std::pow(m_width, 2) + std::pow(m_width, 2)) };
		PointF drawPoint(m_corner.X - size / 10.0f, m_corner.Y - size / 1.5);

		Font font(m_font.c_str(), m_width, FontStyleRegular, UnitPixel);
		graphics.DrawString(m_wstring.c_str(), -1, &font, drawPoint, getBrush());
	}

	void setWidth(int w)
	{
		m_width = w;
	}

	bool hitTest(int x, int y) override
	{
		if (x > m_corner.X && x < m_corner.X + m_width && y > m_corner.Y && y < m_corner.Y + m_width)
		{
			setSelX(x);
			setSelY(y);

			return true;
		}
		return false;
	}

	bool netTest(PointF tlCorner, PointF brCorner) override
	{
		if (m_corner.X > tlCorner.X && m_corner.Y > tlCorner.Y && m_corner.X + m_width < brCorner.X && m_corner.Y + m_width < brCorner.Y)
		{
			setSelX(m_corner.X);
			setSelY(m_corner.Y);
			return true;
		}
		return false;
	}

	void move(float dx, float dy) override
	{
		float moveX{ dx - getSelX() };
		float moveY{ dy - getSelY() };

		m_corner.X += moveX;
		m_corner.Y += moveY;

		setSelX(dx);
		setSelY(dy);
	}

	void setFont(const std::wstring& font) { m_font = font; }

	void queryString(const std::wstring& string) { m_wstring = string; }

	PointF getPosition() override { return m_corner; }
	PointF getCorner() const override { return m_corner; }
	int getLength() const { return m_width; }
	int getHeight() const { return m_width; }
	~Text() override = default;

private:
	PointF m_corner;
	std::wstring m_font;
	std::wstring m_wstring{ L"" };
	int m_width;
};

class SCORE
{
public:
	~SCORE()
	{
		DeleteObject(m_leland);
	}

	enum ELEMENT
	{
		NONE,
		MEASURE,
		SYMBOL,
		TEXT,
		SELECT
	};

	enum DIRECTION
	{
		LEFT,
		DOWN,
		RIGHT,
		UP
	};

	void drawStoredElements(Graphics& graphics)
	{
		for (auto& e : m_elements)
		{
			e->draw(graphics);
		}
	}

	void addElement(std::shared_ptr<Shape> element) { m_elements.push_back(element); }

	void drawCurrentElement(Graphics& graphics);

	void setElement(ELEMENT element) { m_element = element; }
	ELEMENT getElement() const { return m_element; }

	void setOrigin(PointF origin) { m_origin = origin; }
	void setCurrent(PointF current) { m_current = current; }
	PointF getOrigin() const { return m_origin; }
	PointF getCurrent() const { return m_current; }

	void setDrawing(bool b) { m_isDrawing = b; }
	bool isDrawing() const { return m_isDrawing; }

	void setMoving(bool b) { m_isMoving = b; }
	bool isMoving() const { return m_isMoving; }

	void setSelect() { m_selected = m_elements.back().get(); }

	bool isSelecting() const { return m_isSelecting; }

	void setFont()
	{ 
		m_leland = CreateFont(
			288, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, VARIABLE_PITCH, L"Leland");
	}

	void setGlyphs(HWND hWnd)
	{
		HDC hdc = GetDC(hWnd);
		SelectObject(hdc, m_leland);

		for (WCHAR ch{ 0xE000 }; ch < 0xFFFF; ++ch)
		{
			if (isGlyphSupported(hdc, ch))
			{
				WCHAR glyph[2] = { ch, L'\0' };
				SendMessage(hWnd, LB_ADDSTRING, 0, (LPARAM)glyph);
				m_glyphs.push_back(glyph);
			}
		}

		ReleaseDC(hWnd, hdc);
	}

	std::vector<std::wstring> getGlyphs() const { return m_glyphs; }

	void setSymbol(std::size_t index) { m_sym = m_glyphs[index]; }
	std::wstring getSymbol() const { return m_sym; }

	HFONT getLeland() const { return m_leland; }

	void selectElement(int x, int y)
	{
		for (auto& el : m_elements)
		{
			if (el->hitTest(x, y))
			{
				m_selected = el.get();
				m_isMoving = true;
				if (m_selected_elements.size() > 0)
				{
					m_selected_elements.clear();
				}
				return;
			}
		}

		m_isSelecting = true;
		m_selected_elements.clear();
	}

	void moveElement(int x, int y)
	{
		if (!m_selected->isLocked())
		{
			m_selected->move(x, y);
		} 
	}

	void dropElement() { m_isMoving = false; }

	void selectRect(Graphics& graphics) const
	{
		Pen selectPen(Color(255, 0, 0, 100), 2.0f);
		selectPen.SetDashStyle(DashStyleDash);

		graphics.DrawRectangle(&selectPen, m_origin.X, m_origin.Y, m_current.X - m_origin.X, m_current.Y - m_origin.Y);
	}

	void selectElements()
	{
		for (auto& el : m_elements)
		{
			if (el->netTest(m_origin, m_current))
			{
				m_selected_elements.push_back(el);
			}
		}
	}

	void moveElements(DIRECTION direction)
	{
		bool elementsAreUnlocked{ false };

		for (auto& el : m_selected_elements)
		{
			if (!el->isLocked())
			{
				elementsAreUnlocked = true;

				PointF position{ el->getPosition() };
				switch (direction)
				{
				case LEFT:
					--position.X;
					break;
				case DOWN:
					++position.Y;
					break;
				case RIGHT:
					++position.X;
					break;
				case UP:
					--position.Y;
					break;
				default:
					return;
				}

				el->move(position.X, position.Y);
			}
		}

		if (elementsAreUnlocked)
		{
			switch (direction)
			{
			case LEFT:
				--m_origin.X;
				--m_current.X;
				break;
			case DOWN:
				++m_origin.Y;
				++m_current.Y;
				break;
			case RIGHT:
				++m_origin.X;
				++m_current.X;
				break;
			case UP:
				--m_origin.Y;
				--m_current.Y;
				break;
			default:
				return;
			}
		}
	}

	void removeElement()
	{
		if (m_selected)
		{
			if (m_elements.size() == 1)
			{
				if (!m_elements[0]->isLocked())
				{
					m_elements.pop_back();
					m_selected = NULL;
				}
			}
			else if (!m_selected->isLocked())
			{
				// Find the shared_ptr that matches m_selected
				auto it = std::find_if(m_elements.begin(), m_elements.end(),
					[this](const std::shared_ptr<Shape>& shape) {
						return shape.get() == m_selected; // Compare pointers
					});

				// If found, erase it
				if (it != m_elements.end())
				{
					m_elements.erase(it);
					m_selected = NULL;
				}
			}
		}
		else if (m_selected_elements.size() > 0)
		{
			bool elementsAreUnlocked{ false };

			for (auto& selected_el : m_selected_elements)
			{
				if (!selected_el->isLocked())
				{
					elementsAreUnlocked = true;

					auto it = std::find_if(m_elements.begin(), m_elements.end(),
						[selected_el](const std::shared_ptr<Shape>& shape) {
							return shape.get() == selected_el.get();
						});

					m_elements.erase(it);

					selected_el = NULL;
				}
			}

			if (elementsAreUnlocked)
			{
				m_selected_elements.clear();
			}
		}
	}

	void stopSelecting() { m_isSelecting = false; }

	void setSize(int s)
	{
		if (m_selected)
		{
			if (!m_selected->isLocked())
			{
				m_selected->setWidth(s);
			}
		}
		else if (m_selected_elements.size() > 0)
		{
			for (auto& el : m_selected_elements)
			{
				if (!el->isLocked())
				{
					el->setWidth(s);
				}
			}
		}
	}

	void lockElement()
	{
		if (m_selected)
		{
			m_selected->setLock(true);
		}
		else if (m_selected_elements.size() > 0)
		{
			for (auto& el : m_selected_elements)
			{
				el->setLock(true);
			}
		}
	}

	void unlockElement()
	{
		if (m_selected)
		{
			m_selected->setLock(false);
		}
		else if (m_selected_elements.size() > 0)
		{
			for (auto& el : m_selected_elements)
			{
				el->setLock(false);
			}
		}
	}

	void deleteScore()
	{ 
		m_elements.clear();
		m_selected_elements.clear();
	}

private:
	std::vector<std::shared_ptr<Shape>> m_elements;
	std::vector<std::shared_ptr<Shape>> m_selected_elements;
	Shape* m_selected;
	ELEMENT m_element;
	PointF m_origin;
	PointF m_current;

	bool m_isDrawing{ false };
	bool m_isSelecting{ false };
	bool m_isMoving{ false };

	std::vector<std::wstring> m_glyphs;
	std::wstring m_sym;
	HFONT m_leland{ nullptr };

	bool isGlyphSupported(HDC hdc, WCHAR ch)
	{
		WORD glyphIndex;
		GetGlyphIndicesW(hdc, &ch, 1, &glyphIndex, GGI_MARK_NONEXISTING_GLYPHS);
		return glyphIndex != GDI_ERROR && glyphIndex != 0xFFFF;
	}
};

inline void SCORE::drawCurrentElement(Graphics& graphics)
{
	switch (m_element)
	{
	case MEASURE:
		{
			Measure m(m_origin, m_current.X - m_origin.X);
			m.draw(graphics);
			break;
		}
	case SYMBOL:
		{
			Symbol s(m_origin, m_current.Y - m_origin.Y, getSymbol());
			s.draw(graphics);
			break;
		}
	default:
		break;
	}
}