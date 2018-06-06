#include "collision.h"

using namespace std;
using namespace mini;
using namespace in;
using namespace DirectX;

OrientedBoundingRectangle::OrientedBoundingRectangle(XMFLOAT2 corner,
	float width, float height, float rotation)
{
	auto c = XMLoadFloat2(&corner);
	auto m = 
		XMMatrixAffineTransformation2D(XMVectorSet(width, height, 0, 0),
			XMVectorZero(), rotation, c);

	auto dx = XMVectorSet(1, 0, 0, 0);
	auto dy = XMVectorSet(0, 1, 0, 0);
	dx = XMVector2TransformNormal(dx, m);
	dy = XMVector2TransformNormal(dy, m);
	if (width < 0 && height > 0)
	{
		c += dx;
		dx = -dx;
	}
	else if (width > 0 && height < 0)
	{
		c += dy;
		dy = -dy;
	}
	XMStoreFloat2(&m_dx, dx);
	XMStoreFloat2(&m_dy, dy);
	XMStoreFloat2(&m_corner, c);
}

OrientedBoundingRectangle::OrientedBoundingRectangle(const OrientedBoundingRectangle& right)
	: m_corner(right.m_corner), m_dx(right.m_dx), m_dy(right.m_dy)
{ }

static float clamp(float t, float min, float max)
{
	if (t < min)
		return min;
	if (t > max)
		return max;
	return t;
}

float OrientedBoundingRectangle::Distance(const BoundingCircle& circle) const
{
	auto cntr = circle.getCenter();
	auto center = XMLoadFloat2(&cntr);
	XMVECTOR translate;
	float min_dist;
	auto inside = _Collision(center, translate, min_dist);
	return inside ? 0.0f : min_dist;
}

bool OrientedBoundingRectangle::_Collision(FXMVECTOR center, XMVECTOR& translate, float& min_dist) const
{
	XMVECTOR p[4];
	XMVECTOR v[4];
	float len[4];
	p[0] = XMLoadFloat2(&m_corner);
	v[0] = XMLoadFloat2(&m_dx);
	v[1] = XMLoadFloat2(&m_dy);
	len[0] = len[2] = XMVectorGetX(XMVector2Length(v[0]));
	len[1] = len[3] = XMVectorGetX(XMVector2Length(v[1]));
	v[0] = XMVector2Normalize(v[0]);
	v[1] = XMVector2Normalize(v[1]);
	v[2] = -v[0];
	v[3] = -v[1];
	for (auto i = 1; i < 4; ++i)
		p[i] = p[i - 1] + len[i - 1] * v[i - 1];

	auto inside = true;
	min_dist = FLT_MAX;
	translate = XMVectorZero();
	for (auto i = 0; i < 4; ++i)
	{
		auto q = center - p[i];
		auto t = XMVectorGetX(XMVector2Dot(q, v[i]));
		auto clampt = clamp(t, 0, len[i]);
		if (t != clampt)
			inside = false;
		q = p[i] + clampt*v[i];
		auto trDir = center - q;
		auto dist = XMVectorGetX(XMVector2Length(trDir));
		if (dist < min_dist)
		{
			min_dist = dist;
			translate = dist == 0 ? XMVector2Orthogonal(v[i]) 
				: XMVector2Normalize(trDir);
		}
	}
	return inside;
}

XMFLOAT2 OrientedBoundingRectangle::Collision(const BoundingCircle& circle) const
{
	auto cntr = circle.getCenter();
	auto center = XMLoadFloat2(&cntr);
	XMVECTOR translate;
	float min_dist;
	auto inside = _Collision(center, translate, min_dist);
	auto radius = circle.getRadius();
	if (inside)
		translate = -translate*(min_dist + radius);
	else if (min_dist < radius)
		translate *= radius - min_dist;
	else translate = XMVectorZero();
	XMFLOAT2 tr;
	XMStoreFloat2(&tr, translate);
	return tr;
}

CollisionEngine::CollisionEngine(XMFLOAT2 characterPosition, float characterRadius)
	: m_character(characterPosition, characterRadius)
{ }

XMFLOAT2 CollisionEngine::SetObstacles(vector<OrientedBoundingRectangle>&& obstacles)
{
	m_obstacles = move(obstacles);
	return ResolveCollisions();
}

XMFLOAT2 CollisionEngine::MoveObstacle(unsigned int obstacleIndex, OrientedBoundingRectangle obstacle)
{
	if (obstacleIndex >= m_obstacles.size())
		return XMFLOAT2();
	m_obstacles[obstacleIndex] = obstacle;
	return ResolveCollisions();
}

XMFLOAT2 CollisionEngine::MoveCharacter(XMFLOAT2& v)
{
	auto oldPos = m_character.getCenter();
	auto newPos = XMFLOAT2(oldPos.x + v.x, oldPos.y + v.y);
	m_character.setCenter(newPos);
	ResolveCollisions();
	newPos = m_character.getCenter();
	v.x = newPos.x - oldPos.x;
	v.y = newPos.y - oldPos.y;
	return newPos;
}

XMFLOAT2 CollisionEngine::ResolveCollisions()
{
	auto oldPos = m_character.getCenter();
	auto newPos = oldPos;
	m_character.setCenter(newPos);
	for (auto& rect : m_obstacles )
	{
		auto tr = rect.Collision(m_character);
		newPos.x += tr.x;
		newPos.y += tr.y;
		m_character.setCenter(newPos);
	}
	XMFLOAT2 v;
	v.x = newPos.x - oldPos.x;
	v.y = newPos.y - oldPos.y;
	return v;
}

OrientedBoundingRectangle CollisionEngine::getObstacle(unsigned int obstacleIndex) const
{
	if (obstacleIndex >= m_obstacles.size())
		return OrientedBoundingRectangle();
	return m_obstacles[obstacleIndex];
}

float CollisionEngine::DistanceToObstacle(unsigned int obstacleIndex) const
{
	if (obstacleIndex >= m_obstacles.size())
		return FLT_MAX;
	return m_obstacles[obstacleIndex].Distance(m_character);
}