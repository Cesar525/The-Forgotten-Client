/*
  Tibia CLient
  Copyright (C) 2019 Saiyans King

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "automap.h"
#include "engine.h"

Automap g_automap;

extern Engine g_engine;

AutomapArea::AutomapArea(Uint16 x, Uint16 y, Uint8 z, Uint32 area)
{
	m_basepos.x = (x & 0xFF00);
	m_basepos.y = (y & 0xFF00);
	m_basepos.z = (z & 0xFF);
	m_currentArea = area;
	m_recreate = false;

	SDL_snprintf(g_buffer, sizeof(g_buffer), "%03d%03d%02d.map", (m_basepos.x / 256), (m_basepos.y / 256), m_basepos.z);
	m_fileName.assign(g_mapPath);
	m_fileName.append(g_buffer);

	load();
}

AutomapArea::~AutomapArea()
{
	save();
	m_marks.clear();
}

void AutomapArea::render(Sint32 x, Sint32 y, Sint32 w, Sint32 h, Sint32 sx, Sint32 sy, Sint32 sw, Sint32 sh)
{
	g_engine.getRender()->drawAutomapTile(m_currentArea, m_recreate, m_color, x, y, w, h, sx, sy, sw, sh);
}

void AutomapArea::renderMarks(Sint32 zoom, Sint32 diff, Sint32 x, Sint32 y, Sint32 x1, Sint32 y1, Sint32 x2, Sint32 y2)
{
	Surface* renderer = g_engine.getRender();
	for(std::vector<MapMark>::iterator it = m_marks.begin(), end = m_marks.end(); it != end; ++it)
	{
		MapMark& mark = (*it);
		if(mark.x >= x1 && mark.x <= x2 && mark.y >= y1 && mark.y <= y2)
		{
			Sint32 posX = x+diff-5+(mark.x-x1)*zoom;
			Sint32 posY = y+diff-5+(mark.y-y1)*zoom;
			switch(mark.type)
			{
				case 0: renderer->drawPicture(3, 98, 218, posX, posY, 11, 11); break;
				case 1: renderer->drawPicture(3, 109, 218, posX, posY, 11, 11); break;
				case 2: renderer->drawPicture(3, 120, 218, posX, posY, 11, 11); break;
				case 3: renderer->drawPicture(3, 131, 218, posX, posY, 11, 11); break;
				case 4: renderer->drawPicture(3, 142, 218, posX, posY, 11, 11); break;
				case 5: renderer->drawPicture(3, 153, 218, posX, posY, 11, 11); break;
				case 6: renderer->drawPicture(3, 164, 218, posX, posY, 11, 11); break;
				case 7: renderer->drawPicture(3, 175, 218, posX, posY, 11, 11); break;
				case 8: renderer->drawPicture(3, 186, 218, posX, posY, 11, 11); break;
				case 9: renderer->drawPicture(3, 197, 218, posX, posY, 11, 11); break;
				case 10: renderer->drawPicture(3, 98, 229, posX, posY, 11, 11); break;
				case 11: renderer->drawPicture(3, 109, 229, posX, posY, 11, 11); break;
				case 12: renderer->drawPicture(3, 120, 229, posX, posY, 11, 11); break;
				case 13: renderer->drawPicture(3, 131, 229, posX, posY, 11, 11); break;
				case 14: renderer->drawPicture(3, 142, 229, posX, posY, 11, 11); break;
				case 15: renderer->drawPicture(3, 153, 229, posX, posY, 11, 11); break;
				case 16: renderer->drawPicture(3, 164, 229, posX, posY, 11, 11); break;
				case 17: renderer->drawPicture(3, 175, 229, posX, posY, 11, 11); break;
				case 18: renderer->drawPicture(3, 186, 229, posX, posY, 11, 11); break;
				default: renderer->drawPicture(3, 197, 229, posX, posY, 11, 11); break;
			}
		}
	}
}

void AutomapArea::setTileDetail(Uint16 x, Uint16 y, Uint8 color, Uint8 speed)
{
	x &= 0xFF;
	y &= 0xFF;
	m_speed[x][y] = speed;

	Uint8 c = m_color[x][y];
	if(c != color)
	{
		m_color[x][y] = color;
		m_recreate = true;
	}
}

Uint8 AutomapArea::getColor(Uint16 x, Uint16 y)
{
	x &= 0xFF;
	y &= 0xFF;
	return m_color[x][y];
}

Uint8 AutomapArea::getSpeed(Uint16 x, Uint16 y)
{
	x &= 0xFF;
	y &= 0xFF;
	return m_speed[x][y];
}

bool AutomapArea::load()
{
	SDL_RWops* areafile = SDL_RWFromFile(m_fileName.c_str(), "rb");
	if(!areafile)
	{
		SDL_memset(m_color, 0, 256*256);
		SDL_memset(m_speed, 0, 256*256);
		return false;
	}
	
	SDL_RWseek(areafile, 0, RW_SEEK_END);
	size_t fileSize = SDL_static_cast(size_t, SDL_RWtell(areafile));
	SDL_RWseek(areafile, 0, RW_SEEK_SET);

	SDL_RWread(areafile, m_color, 1, 256*256);
	SDL_RWread(areafile, m_speed, 1, 256*256);
	if(fileSize > SDL_static_cast(size_t, SDL_RWtell(areafile)))
	{
		Uint32 marks = SDL_ReadLE32(areafile);
		for(Uint32 i = 0; i < marks; ++i)
		{
			Uint32 x = SDL_ReadLE32(areafile);
			Uint32 y = SDL_ReadLE32(areafile);
			Uint32 type = SDL_ReadLE32(areafile);
			const std::string text = SDL_ReadLEString(areafile);

			MapMark mark(SDL_static_cast(Uint16, x), SDL_static_cast(Uint16, y), SDL_static_cast(Uint8, type), text);
			m_marks.push_back(mark);
		}
	}

	m_recreate = true;
	SDL_RWclose(areafile);
	return true;
}

bool AutomapArea::save()
{
	SDL_RWops* areafile = SDL_RWFromFile(m_fileName.c_str(), "wb");
	if(!areafile)
		return false;

	SDL_RWwrite(areafile, m_color, 1, 256*256);
	SDL_RWwrite(areafile, m_speed, 1, 256*256);

	SDL_WriteLE32(areafile, SDL_static_cast(Uint32, m_marks.size()));
	for(std::vector<MapMark>::iterator it = m_marks.begin(), end = m_marks.end(); it != end; ++it)
	{
		MapMark& mark = (*it);
		SDL_WriteLE32(areafile, SDL_static_cast(Uint32, mark.x));
		SDL_WriteLE32(areafile, SDL_static_cast(Uint32, mark.y));
		SDL_WriteLE32(areafile, SDL_static_cast(Uint32, mark.type));
		SDL_WriteLEString(areafile, mark.text);
	}

	SDL_RWclose(areafile);
	return true;
}

Automap::Automap()
{
	m_zoom = 4;
	m_diff = m_zoom/2;
	m_currentArea = 0;
	m_centerPosition = Position(0, 0, 7);
	m_position = Position(0, 0, 7);
}

Automap::~Automap()
{
	for(AutomapAreas::iterator it = m_areas.begin(), end = m_areas.end(); it != end; ++it)
		delete it->second;
}

AutomapArea* Automap::getArea(Uint16 x, Uint16 y, Uint8 z)
{
	Uint32 posindex = ((z & 0xFF) | ((y & 0xFF00) << 8) | ((x & 0xFF00) << 16));
	AutomapAreas::iterator it = m_areas.find(posindex);
	if(it != m_areas.end())
		return it->second;
	else
	{
		if(m_areas.size() == AUTOMAP_MAXTILES)//Probably never happen but let's just prevent massive memory usage
		{
			for(AutomapAreas::iterator ait = m_areas.begin(), end = m_areas.end(); ait != end; ++ait)
				delete ait->second;

			m_areas.clear();
		}
		AutomapArea* area = new AutomapArea(x, y, z, m_currentArea++);
		m_areas[posindex] = area;
		return area;
	}
}

void Automap::setCentralPosition(const Position& pos)
{
	m_centerPosition = pos;
	m_position = Position(pos.x, pos.y, pos.z);
}

void Automap::setTileDetail(Uint16 x, Uint16 y, Uint8 z, Uint8 color, Uint8 speed)
{
	AutomapArea* area = getArea(x, y, z);
	if(area)
		area->setTileDetail((x & 0xFF), (y & 0xFF), color, speed);
}

void Automap::render(Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
	Sint32 zoom = m_zoom;
	Surface* renderer = g_engine.getRender();
	renderer->setClipRect(x, y, w, h);

	//Coordinates of the top-left, bottom-right minimap corner
	Sint32 x1 = m_position.x-SDL_static_cast(Sint32, (w/2.0f/zoom));
	Sint32 y1 = m_position.y-SDL_static_cast(Sint32, (h/2.0f/zoom));
	Sint32 x2 = m_position.x+SDL_static_cast(Sint32, (w/2.0f/zoom));
	Sint32 y2 = m_position.y+SDL_static_cast(Sint32, (h/2.0f/zoom));

	//Cache minimap areas to render marks later
	std::vector<AutomapArea*> cachedAreas;
	cachedAreas.reserve(4);

	//(i,j) is the pixel position where we will draw the area
	//(g) is the precached y pixel start position
	Sint32 i, j, g;

	if(y1 < 0) g = (-y1*zoom);
	else g = (-(y1 & 0xFF)*zoom);

	if(x1 < 0) i = (-x1*zoom);
	else i = (-(x1 & 0xFF)*zoom);

	Sint32 tileSize = (256*zoom);
	for(; i < w; i += tileSize)
	{
		Sint32 current_x = x1+(i/zoom);
		Sint32 destx = i, srcx = 0;
		Sint32 srcw = 256, destw = tileSize;
		if(destx < 0)
		{
			srcx = (-destx/zoom);
			srcw -= srcx;
			destx = 0;
			destw = (srcw*zoom);
		}
		if(destx + destw > w)
		{
			destw = w-destx;
			srcw = (destw/zoom);
		}

		j = g;
		for(; j < h; j += tileSize)
		{
			Sint32 current_y = y1+(j/zoom);
			Sint32 desty = j, srcy = 0;
			Sint32 srch = 256, desth = tileSize;
			if(desty < 0)
			{
				srcy = (-desty/zoom);
				srch -= srcy;
				desty = 0;
				desth = (srch*zoom);
			}
			if(desty + desth > h)
			{
				desth = h-desty;
				srch = (desth/zoom);
			}

			if(current_x <= 64768 && current_y <= 64768)
			{
				AutomapArea* area = getArea(SDL_static_cast(Uint16, current_x), SDL_static_cast(Uint16, current_y), m_position.z);
				if(area)
				{
					area->render(x+destx, y+desty, destw, desth, srcx, srcy, srcw, srch);
					cachedAreas.push_back(area);
				}
			}
		}
	}

	//mark where the player is
	if(m_centerPosition.z == m_position.z && m_centerPosition.x >= x1 && m_centerPosition.x <= x2 && m_centerPosition.y >= y1 && m_centerPosition.y <= y2)
	{
		Sint32 posX = x+m_diff-3+(m_centerPosition.x-x1)*zoom;
		Sint32 posY = y+m_diff-3+(m_centerPosition.y-y1)*zoom;
		renderer->fillRectangle(posX+2, posY, 2, 6, 255, 255, 255, 255);
		renderer->fillRectangle(posX, posY+2, 6, 2, 255, 255, 255, 255);
	}
	for(std::vector<AutomapArea*>::iterator it = cachedAreas.begin(), end = cachedAreas.end(); it != end; ++it)
		(*it)->renderMarks(m_zoom, m_diff, x, y, x1, y1, x2, y2);
	renderer->disableClipRect();
}

bool Automap::wasSeen(const Position& pos)
{
	AutomapArea* area = getArea(pos.x, pos.y, pos.z);
	if(area)
		return (area->getColor((pos.x & 0xFF), (pos.y & 0xFF)) != 0);
	return false;
}

bool Automap::isWalkable(const Position& pos)
{
	AutomapArea* area = getArea(pos.x, pos.y, pos.z);
	if(area)
		return (area->getSpeed((pos.x & 0xFF), (pos.y & 0xFF)) != 0);
	return false;
}

Uint8 Automap::getSpeed(const Position& pos)
{
	AutomapArea* area = getArea(pos.x, pos.y, pos.z);
	if(area)
		return area->getSpeed((pos.x & 0xFF), (pos.y & 0xFF));
	return 0;
}

void Automap::setZoom(Sint32 zoom)
{
	m_zoom = zoom;
	if(m_zoom < 1)
		m_zoom = 1;
	else if(m_zoom > 8)
		m_zoom = 8;
	m_diff = m_zoom / 2;
}

void Automap::zoomOut()
{
	if(m_zoom >= 1)
	{
		m_zoom /= 2;
		m_diff = m_zoom/2;
	}
}

void Automap::zoomIn()
{
	if(m_zoom <= 8)
	{
		m_zoom *= 2;
		m_diff = m_zoom/2;
	}
}
