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
학습용 구현 실제 이렇게 사용 안 됨 
// ++ 전위연산자 
const Vector2D& Vector2D::operator++() 
{
    x++;
    y++;
    return *this;
}

// ++ 후위연산자 
const Vector2D Vector2D::operator++(int) 
{
    Vector2D temp = *this; 
    x++;
    y++;
    return temp; 
} 

// -- 전위연산자 
const Vector2D& Vector2D::operator--() 
{
    x--;
    y--;
    return *this;
}

// -- 후위연산자 
const Vector2D Vector2D::operator--(int)
{
    Vector2D temp = *this;
    x--;
    y--;
    return temp;
} 
*/ 
