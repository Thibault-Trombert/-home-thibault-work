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

#include <fstream>
#include <string>
#include "Registers.h"
#include "Log.h"
using namespace std;

QueueLog::QueueLog(int maxItems)
{
	if (maxItems <= 0)
		maxItems = 100;
	
	m_maxItems = maxItems;
	Empty();
}

QueueLog::~QueueLog()
{
	DestroyAll();
}

void QueueLog::DestroyAll()
{
    ItemLog *item;
	ItemLog *auxItem;
	
	item = m_first;
	while (item) {
		auxItem = item->next;
		if (item->regs)
			delete item->regs;
		delete item;
		item = auxItem;
	}
    
    Empty();
}

void QueueLog::Empty()
{
    m_numItems = 0;
	m_first = NULL;
	m_last = NULL;
}

void QueueLog::Enqueue(string prefix, Registers *regs, string suffix)
{
	ItemLog * newItem = new ItemLog;
	newItem->prefix = prefix;
	newItem->regs = NULL;
	if (regs)
	{
		newItem->regs = new Registers();
		*newItem->regs = *regs;
	}
	newItem->suffix = suffix;
	newItem->next = NULL;
	newItem->prev = NULL;
	
	if (m_numItems >= m_maxItems)
	{
		m_first = m_first->next;
		//Borrar el más viejo
		if (m_first->prev->regs)
			delete m_first->prev->regs;
		delete m_first->prev;
		m_first->prev = NULL;
		m_numItems--;
	}
	
	if (m_first == NULL)
	{
		m_first = newItem;
		m_last = newItem;
	}
	else
	{
		m_last->next = newItem;
		newItem->prev = m_last;
		m_last = newItem;
	}
	
	m_numItems++;

}

void QueueLog::Save(string path, bool empty)
{
	ofstream file(path.c_str(), ios_base::out);
	
	if (file)
	{
		ItemLog *item;
		
		item = m_first;
		while (item) {
			file << item->prefix;
			if (item->regs)
				file << item->regs->ToString();
			file << item->suffix << endl;
			item = item->next;
		}
		file.close();
	}
    
    if (empty)
    {
        DestroyAll();
    }
}
