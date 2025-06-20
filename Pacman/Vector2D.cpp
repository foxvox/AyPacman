#include "Vector2D.h"

const Vector2D& Vector2D::operator+=(const Vector2D& rhs)
{
    x += rhs.x;
    y += rhs.y;
    return *this;
} 

const Vector2D& Vector2D::operator-=(const Vector2D& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

const Vector2D Vector2D::operator+(const Vector2D& rhs) const 
{
	return Vector2D(x + rhs.x, y + rhs.y); 
}

const Vector2D Vector2D::operator-(const Vector2D& rhs) const 
{
	return Vector2D(x - rhs.x, y - rhs.y); 
}

/* 
�н��� ���� ���� �̷��� ��� �� �� 
// ++ ���������� 
const Vector2D& Vector2D::operator++() 
{
    x++;
    y++;
    return *this;
}

// ++ ���������� 
const Vector2D Vector2D::operator++(int) 
{
    Vector2D temp = *this; 
    x++;
    y++;
    return temp; 
} 

// -- ���������� 
const Vector2D& Vector2D::operator--() 
{
    x--;
    y--;
    return *this;
}

// -- ���������� 
const Vector2D Vector2D::operator--(int)
{
    Vector2D temp = *this;
    x--;
    y--;
    return temp;
} 
*/ 
