/*
 This file is part of DMGBoy.
 
 DMGBoy is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DMGBoy is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with DMGBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "Memory.h"
#include "IGBScreenDrawable.h"
#include "Video.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MEMR(address) (m_mem->memory[(address)])

using namespace std;

Video::Video(IGBScreenDrawable *screen)
{
    m_colorMode = false;
	m_pixel = new VideoPixel();
	SetScreen(screen);
}

Video::~Video(void)
{
	
}

void Video::SetScreen(IGBScreenDrawable *screen)
{
    m_screen = screen;
}

void Video::SetColorMode(bool value)
{
    m_colorMode = value;
}

void Video::SetMem(Memory *mem)
{
	m_mem = mem;
}

void Video::UpdateLine(BYTE y)
{
    if (m_screen)
        m_screen->OnPreDraw();

	OrderOAM(y);
	UpdateBG(y);
	UpdateWin(y);
	UpdateOAM(y);

    if (m_screen)
        m_screen->OnPostDraw();
}

void Video::RefreshScreen()
{
    if (m_screen)
        m_screen->OnRefreshGBScreen();
}

void Video::ClearScreen()
{
    if (m_screen)
        m_screen->OnClear();
}

void Video::UpdateBG(int y)
{
	int x, yScrolled;
	BYTE valueLCDC, valueSCX;
	int display;

	valueLCDC = MEMR(LCDC);
	valueSCX = MEMR(SCX);

	display = BIT7(valueLCDC);
    if ((!m_colorMode) && !BIT0(MEMR(LCDC)))
        display = false;
	
	//Si el LCD desactivado
	//pintamos la linea de blanco o negro
	if (!display && m_screen)
	{
		for (x=0; x<GB_SCREEN_W; x++)
        {
            if (m_colorMode)
                m_screen->OnDrawPixel(0, 0, 0, x, y);
            else
                m_screen->OnDrawPixel(3, x, y);
            
            m_priorityBGWnd[x][y] = false;
        }
		
		return;
	}
	
	//Seleccionar el tile map
    if (m_colorMode)
    {
        m_pixel->mapIni = BIT3(valueLCDC) ? VRAM_OFFSET+0x1C00 : VRAM_OFFSET+0x1800;
    }
    else
    {
        m_pixel->mapIni = BIT3(valueLCDC) ? 0x9C00 : 0x9800;
        GetDMGPalette(m_pixel->palette, BGP);
    }

	yScrolled = (y + MEMR(SCY));
	if (yScrolled < 0)
		yScrolled += 256;
	else if (yScrolled > 255)
		yScrolled -= 256;
	
    m_pixel->y = y;
	m_pixel->yTile = yScrolled % 8;
	m_pixel->rowMap = (yScrolled/8 * 32);
	
	m_pixel->tileDataSelect = BIT4(valueLCDC);
	
	for (x=0; x<GB_SCREEN_W; x++)
	{
		m_pixel->x = x;
		m_pixel->xScrolled = (x + valueSCX);
		if (m_pixel->xScrolled > 255)
			m_pixel->xScrolled -= 256;

		GetColor(m_pixel);

        if (m_screen)
        {
            if (m_colorMode)
                m_screen->OnDrawPixel(m_pixel->r, m_pixel->g, m_pixel->b, x, y);
            else
                m_screen->OnDrawPixel(m_pixel->color, x, y);
        }
	}
}

void Video::UpdateWin(int y)
{
	int x, wndPosX, xIni, yScrolled;
	WORD wndPosY;

	//Si la ventana esta desactivada no hacemos nada
	if (!BIT5(MEMR(LCDC)))
		return;
    if (!m_colorMode && (!BIT0(MEMR(LCDC))))
        return;

	wndPosX = MEMR(WX) - 7;
	wndPosY = MEMR(WY);

	if (y < wndPosY)
		return;

	if (wndPosX < 0) xIni = 0;
	else if (wndPosX > GB_SCREEN_W) xIni = GB_SCREEN_W;
	else xIni = wndPosX;

    if (m_colorMode)
    {
        m_pixel->mapIni = BIT6(MEMR(LCDC)) ? VRAM_OFFSET+0x1C00 : VRAM_OFFSET+0x1800;
    }
    else
    {
        m_pixel->mapIni = BIT6(MEMR(LCDC)) ? 0x9C00 : 0x9800;
        GetDMGPalette(m_pixel->palette, BGP);
    }

		
	yScrolled = y - wndPosY;
	m_pixel->yTile = yScrolled % 8;
	m_pixel->rowMap = yScrolled/8 * 32;
	
	m_pixel->tileDataSelect = BIT4(MEMR(LCDC));
	
	m_pixel->y = y;

	for (x=xIni; x<GB_SCREEN_W; x++)
	{
		m_pixel->x = x;
		m_pixel->xScrolled = x - wndPosX;

		GetColor(m_pixel);

        if (m_screen)
        {
            if (m_colorMode)
                m_screen->OnDrawPixel(m_pixel->r, m_pixel->g, m_pixel->b, x, y);
            else
                m_screen->OnDrawPixel(m_pixel->color, x, y);
        }
	}
}

inline void Video::GetColor(VideoPixel *p)
{
	int xTile, line[2], addressIdTile, addressTile, mapAttributes, yTile, idMapTile, bgPriority;
    BYTE colorPalette[4][3];
	
    idMapTile = p->rowMap + p->xScrolled/8;
	addressIdTile = p->mapIni + idMapTile;
	
	if (!p->tileDataSelect)	//Seleccionar el tile data
	{
		//0x8800 = 0x9000 - (128 * 16)
		addressTile = (WORD)(0x9000 + ((char)MEMR(addressIdTile))*16);	//Se multiplica por 16 porque cada tile ocupa 16 bytes
	}
	else
	{
		addressTile = 0x8000 + MEMR(addressIdTile)*16;
	}
    
    bgPriority = 0;
    xTile = p->xScrolled % 8;
    yTile = p->yTile;
    if (m_colorMode)
    {
        mapAttributes = MEMR(addressIdTile + 0x2000);
        
        addressTile += VRAM_OFFSET - 0x8000;
        if (BIT3(mapAttributes))
            addressTile += 0x2000;
        if (BIT5(mapAttributes))    // Flip x
            xTile = ABS(xTile - 7);
        if (BIT6(mapAttributes))    // Flip y
            yTile = ABS(yTile - 7);
        bgPriority = BIT7(mapAttributes);
        
        int numPalette = mapAttributes & 0x07;
        GetColorPalette(colorPalette, BGP_OFFSET + (numPalette*8));
    }
	
	int addressLineTile = addressTile + (yTile * 2); //yTile * 2 porque cada linea de 1 tile ocupa 2 bytes
	
	line[0] = MEMR(addressLineTile + 0);
	line[1] = MEMR(addressLineTile + 1);
	
	int pixX = ABS(xTile - 7);
	//Un pixel lo componen 2 bits. Seleccionar la posicion del bit en los dos bytes (line[0] y line[1])
	//Esto devolvera un numero de color que junto a la paleta de color nos dara el color requerido
	p->indexColor = (((line[1] & (0x01 << pixX)) >> pixX) << 1) | ((line[0] & (0x01 << pixX)) >> pixX);
    
    if (m_colorMode) {
        p->r = colorPalette[p->indexColor][0];
        p->g = colorPalette[p->indexColor][1];
        p->b = colorPalette[p->indexColor][2];
        
        if (bgPriority && (m_pixel->indexColor > 0))
            m_priorityBGWnd[m_pixel->x][m_pixel->y] = true;
        else
            m_priorityBGWnd[m_pixel->x][m_pixel->y] = false;
    }
    else {
        p->color = p->palette[p->indexColor];
        m_priorityBGWnd[m_pixel->x][m_pixel->y] = (m_pixel->indexColor > 0);
    }
}

void Video::OrderOAM(int y)
{
	int ySprite, hSprite, address, numSprite;

	m_orderedOAM.clear();

	if (!BIT1(MEMR(LCDC)))	//OAM desactivado
		return;

	hSprite = BIT2(MEMR(LCDC)) ? 16 : 8;
    
    numSprite = 40;
    for(address=0xFE9C; address>=0xFE00; address-=0x04)
	{
		ySprite = MEMR(address);

		ySprite -= 16;	//y en pantalla
		if ((ySprite > y-hSprite) && (ySprite <= y))
        {
            if (m_colorMode)
                m_orderedOAM.insert(pair<int, int>(numSprite, address));
            else
                m_orderedOAM.insert(pair<int, int>(MEMR(address+1), address));
        }
        numSprite--;
	}
    
// Habilitar para permitir solo 10 tiles por linea (asi es como funciona en la realidad)
#if (0)
    int size = m_orderedOAM.size();
    if (size > 10)
    {
        numSprite = 0;
        multimap<int, int>::iterator it;
        for (it=m_orderedOAM.begin(); it!=m_orderedOAM.end(); it++)
        {
            numSprite++;
            if (numSprite > 10)
                break;
        }
        m_orderedOAM.erase(it, m_orderedOAM.end());
    }
#endif
}

void Video::UpdateOAM(int y)
{
	int x, xSprite, numSpritesLine, xBeg;
	int attrSprite, xTile, yTile, xFlip, yFlip, countX, countY, spritePriority, mode16, ySprite;
	int addressSprite, tileNumber, addressTile;
	int color;
	int palette0[4], palette1[4];
	BYTE line[2];
    BYTE colorPalette[4][3];

	if (!BIT1(MEMR(LCDC)))	//OAM desactivado
		return;

	mode16 = BIT2(MEMR(LCDC));

	GetDMGPalette(palette0, OBP0);
	GetDMGPalette(palette1, OBP1);

	multimap<int, int>::reverse_iterator it;

	numSpritesLine = 0;

	for (it=m_orderedOAM.rbegin(); it != m_orderedOAM.rend(); it++)
	{
		addressSprite = (*it).second;
		ySprite = MEMR(addressSprite+0) - 16;
		xSprite = MEMR(addressSprite+1) - 8;
		if (xSprite == -8)
			continue;
		tileNumber = MEMR(addressSprite + 2);
		if (mode16)
			tileNumber = tileNumber & 0xFE;
			//!!!!!!!!!Si toca la parte de abajo del tile de 8x16 hay que sumar uno (tileNumber | 0x01)
		attrSprite = MEMR(addressSprite + 3);
		addressTile = 0x8000 + tileNumber*16;
		xFlip = BIT5(attrSprite);
		yFlip = BIT6(attrSprite);
		spritePriority = BIT7(attrSprite);
        
        if (m_colorMode)
        {
            addressTile += VRAM_OFFSET - 0x8000;
            if (BIT3(attrSprite))
                addressTile += 0x2000;
            
            int numPalette = attrSprite & 0x07;
            GetColorPalette(colorPalette, OBP_OFFSET + (numPalette*8));
        }

		xTile = countX = countY = 0;
		yTile = y - ySprite;
		countY = yTile;
		if (yFlip)
			yTile = ABS(yTile - (mode16 ? 15 : 7));

		if (xSprite>0)
		{
			xBeg = xSprite;
			countX = 0;
		}
		else
		{
			xBeg = 0;
			countX = ABS(xSprite);
		}

		for (x=xBeg; ((x<xSprite+8) && (x<GB_SCREEN_W)); x++)
		{
			xTile = xFlip ? ABS(countX - 7) : countX;

			line[0] = MEMR(addressTile + (yTile * 2));	//yTile * 2 porque cada linea de 1 tile ocupa 2 bytes
			line[1] = MEMR(addressTile + (yTile * 2) + 1);

			int pixX = ABS(xTile - 7);
			//Un pixel lo componen 2 bits. Seleccionar la posicion del bit en los dos bytes (line[0] y line[1])
			//Esto devolvera un numero de color que junto a la paleta de color nos dara el color requerido
			BYTE index = (((line[1] & (1 << pixX)) >> pixX) << 1) | ((line[0] & (1 << pixX)) >> pixX);
            
            bool paintSprite = ObjAboveBG(spritePriority, xSprite + countX, ySprite + countY);
            
			//El 0 es transparente (no pintar)
			if (paintSprite && (index > 0))
			{
                if (m_colorMode) {
                    BYTE r = colorPalette[index][0];
                    BYTE g = colorPalette[index][1];
                    BYTE b = colorPalette[index][2];
                    
                    if (m_screen)
                        m_screen->OnDrawPixel(r, g, b, xSprite + countX, ySprite + countY);
                }
                else
                {
                    color = !BIT4(attrSprite) ? palette0[index] : palette1[index];

                    if (m_screen)
                        m_screen->OnDrawPixel(color, xSprite + countX, ySprite + countY);
                }
			}

			countX++;
		}
	}
}

// Decide qué se pinta por encima, si el sprite o el background/window
// Hay 3 sitios que lo deciden (en este orden):
// 1 - LCDC Bit 0
// 2 - BG Map Attribute Bit 7 *
// 3 - OAM Attribute Bit 7
// * El valor de BG Map Attr está almacenado en priorityBGWnd
bool Video::ObjAboveBG(BYTE oamBit7, int x, int y) {
    if (m_colorMode) {
        bool paintSprite = (BIT0(MEMR(LCDC)) == 0);
        if (!paintSprite) {
            if (m_priorityBGWnd[x][y])
                paintSprite = false;
            else
                paintSprite = oamBit7 ? false : true;
        }
        return paintSprite;
    } else
        return !oamBit7 || !m_priorityBGWnd[x][y];
}

void Video::GetDMGPalette(int *palette, int dir)
{
	BYTE paletteData = MEMR(dir);

	palette[0] = ABS((int)(BITS01(paletteData) - 3));
	palette[1] = ABS((int)((BITS23(paletteData) >> 2) - 3));
	palette[2] = ABS((int)((BITS45(paletteData) >> 4) - 3));
	palette[3] = ABS((int)((BITS67(paletteData) >> 6) - 3));
}

void Video::GetColorPalette(BYTE palette[4][3], int address)
{
	for (int i=0; i<4; i++)
    {
        BYTE data1 = MEMR(address+0);
        BYTE data2 = MEMR(address+1);
        
        palette[i][0] = data1 & 0x1F;
        palette[i][1] = ((data2 & 0x03) << 3) | ((data1 & 0xE0) >> 5);
        palette[i][2] = (data2 & 0x7C) >> 2;
        
        // Como el valor va de 0 a 31 (1F), hay que convertirlo de 0 a 255
        // para que sea mas eficiente lo hare de 0 a 248
        palette[i][0] <<= 3;
        palette[i][1] <<= 3;
        palette[i][2] <<= 3;
        
        address += 2;
    }
}

void Video::GetTile(BYTE *buffer, int widthSize, int tile, int bank)
{
    int line[2];
    
    int addressTile = 0x8000;
    if (m_colorMode)
    {
        addressTile += VRAM_OFFSET - 0x8000;
        if (bank == 1)
            addressTile += 0x2000;
    }
    else if (bank == 1)
        return;
    
    addressTile += tile * 16;
    
    for (int y=0; y<8; y++)
    {
        for (int x=0; x<8; x++)
        {
            int addressLineTile = addressTile + (y * 2); //yTile * 2 porque cada linea de 1 tile ocupa 2 bytes
            
            line[0] = MEMR(addressLineTile + 0);
            line[1] = MEMR(addressLineTile + 1);
            
            int pixX = ABS(x - 7);
            //Un pixel lo componen 2 bits. Seleccionar la posicion del bit en los dos bytes (line[0] y line[1])
            //Esto devolvera un numero de color que junto a la paleta de color nos dara el color requerido
            int indexColor = (((line[1] & (1 << pixX)) >> pixX) << 1) | ((line[0] & (1 << pixX)) >> pixX);
            
            int offset = widthSize*y + x*3;
            int gray = ABS(indexColor - 3) * 85;
            buffer[offset + 0] = gray;
            buffer[offset + 1] = gray;
            buffer[offset + 2] = gray;
        }
    }
}
