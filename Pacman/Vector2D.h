#pragma once 

#include <iostream> 

using namespace std; 

class Vector2D
{
private:
	float x; 
	float y; 
public:
	explicit Vector2D() : x{ 0.0f }, y{ 0.0f } {};
	explicit Vector2D(int _x, int _y) : x{ (float)_x }, y{ (float)_y } {}
	explicit Vector2D(float _x, float _y) : x{ _x }, y{ _y } {}
public: 	
	const Vector2D	operator+(const Vector2D& rhs) const; 
	const Vector2D	operator-(const Vector2D& rhs) const; 	
	const Vector2D& operator+=(const Vector2D& rhs); 
	const Vector2D& operator-=(const Vector2D& rhs); 
public: 
	void SetX(float _x) { x = _x; } 
	void SetY(float _y) { y = _y; } 
	const float GetX() const { return x; } 
	const float GetY() const { return y; } 	
};

