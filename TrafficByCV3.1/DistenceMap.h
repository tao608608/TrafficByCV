#ifndef DISTENCEMAP_H
#define DISTENCEMAP_H

#pragma once

class DistenceMap
{
public:
	int visit_count;
	DistenceMap(int width, int height, int v)
	{
		this->data = new int[width*height];
		for (int i = 0; i < width*height;++i)
		{
			data[i] = v;
		}
		this->width = width;
		this->height = height;
		this->visit_count = 0;
	}
	~DistenceMap()
	{
		delete[] data;
	}
	inline int GetValue(int x, int y)
	{
		visit_count++;
		return data[x + y*width];
	}
	inline void SetVaule(int x, int y, int v)
	{
		visit_count++;
		data[x + y*width] = v;
	}
	inline int Width()
	{
		return width;
	}
	inline int Height()
	{
		return height;
	}
	inline int Length()
	{
		return width*height;
	}
protected:
private:
	int* data;
	int width;
	int height;
};
#endif DISTENCEMAP_H
