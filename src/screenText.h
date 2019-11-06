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

#ifndef __FILE_SCREENTEXT_h_
#define __FILE_SCREENTEXT_h_

#include "position.h"

struct ScreenTextMessage
{
	std::string m_message;
	Uint32 m_endTime;
	MessageMode m_mode;
};

struct ScreenTextMessageLine
{
	size_t lineStart;
	size_t lineLength;
	Uint32 lineWidth;
};

class ScreenText
{
	public:
		ScreenText(OnscreenMessages position);
		
		void addMessage(MessageMode mode, std::string text);
		void composeText();

		bool update();
		void render(Sint32 boundLeft, Sint32 boundTop, Sint32 boundRight, Sint32 boundBottom);

	protected:
		std::vector<ScreenTextMessage> m_messages;
		std::vector<ScreenTextMessageLine> m_messagesLine;
		std::string m_text;

		Uint32 m_textWidth;
		Uint32 m_allowedWidth;
		OnscreenMessages m_screenPosition;

		MessageMode m_mode;
		Uint8 m_red;
		Uint8 m_green;
		Uint8 m_blue;
		bool m_dirty;
};

#endif /* __FILE_SCREENTEXT_h_ */
