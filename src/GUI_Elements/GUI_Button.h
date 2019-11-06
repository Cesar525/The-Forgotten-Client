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

#ifndef __FILE_GUI_BUTTON_h_
#define __FILE_GUI_BUTTON_h_

#include "GUI_Element.h"

class GUI_Button : public GUI_Element
{
	public:
		GUI_Button(iRect boxRect, const std::string labelName, Uint32 internalID = 0);

		void setRect(iRect& NewRect);
		void setButtonEventCallback(void (*eventHandlerFunction)(Uint32,Sint32), Uint32 mEvent);

		void onMouseMove(Sint32 x, Sint32 y, bool isInsideParent);
		void onLMouseDown(Sint32 x, Sint32 y);
		void onLMouseUp(Sint32 x, Sint32 y);

		void render();

	protected:
		void (*m_eventHandlerFunction)(Uint32,Sint32);
		std::string m_Label;
		Uint32 m_evtParam;
		Uint32 m_startX;
		Sint32 m_sx[2];
		Sint32 m_sy[2];
		Uint8 m_Pressed;
};

#endif /* __FILE_GUI_BUTTON_h_ */
