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

#include "GUI_Grouper.h"
#include "../engine.h"

extern Engine g_engine;

GUI_Grouper::GUI_Grouper(iRect boxRect, Uint32 internalID)
{
	setRect(boxRect);
	m_internalID = internalID;
}

void GUI_Grouper::render()
{
	Surface* renderer = g_engine.getRender();
	renderer->drawPictureRepeat(3, 0, 96, 96, 1, m_tRect.x1, m_tRect.y1, m_tRect.x2, 1);
	renderer->drawPictureRepeat(3, 0, 98, 1, 96, m_tRect.x1, m_tRect.y1+1, 1, m_tRect.y2-1);
	renderer->drawPictureRepeat(3, 2, 197, 96, 1, m_tRect.x1+1, m_tRect.y1+m_tRect.y2-1, m_tRect.x2-1, 1);
	renderer->drawPictureRepeat(3, 276, 0, 1, 96, m_tRect.x1+m_tRect.x2-1, m_tRect.y1+1, 1, m_tRect.y2-2);
}
