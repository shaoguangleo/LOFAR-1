//# GTM_Config.h:
//#
//#  Copyright (C) 2002-2003
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//#  This program is free software; you can redistribute it and/or modify
//#  it under the terms of the GNU General Public License as published by
//#  the Free Software Foundation; either version 2 of the License, or
//#  (at your option) any later version.
//#
//#  This program is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#  GNU General Public License for more details.
//#
//#  You should have received a copy of the GNU General Public License
//#  along with this program; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//#  $Id$

#ifndef GTM_CONFIG_H
#define GTM_CONFIG_H
 
#include <Common/lofar_fstream.h>
#include <Common/lofar_string.h>

#define DEF_CONFIG       "config.cfg"

enum ValType
{
  TYPE_EMPTY,
  TYPE_VARIABLE,
  TYPE_KEYWORD,
  TYPE_BLOCK
};

class GTMValue
{
  string _value;
  int _type;
  int _block;
  
 public:
  GTMValue* _pNext;

  GTMValue(void);
  
  void set(string str, int bl, int key = TYPE_VARIABLE);
  string& get(void);
  const char* sget(void);
  
  int is_keyword(void);
  int is_block(void);
  int get_block_id(void);
  int get_type(void);
};


class GTMConfig
{
  ifstream _file;
  char _delimiter;
  int _vcol;

  GTMValue* _pSValue, *_pVStart;

  int open_config_file(const char* name);
  int close_config_file(void);
  int read_config_file(void);
  int read_config_from_string(const char* buf);
  
  void S_trunc(string& str);

  int search_block(const string& block);
  const char* search_value(int col);
  
public:
  GTMConfig(void);
  GTMConfig(const char* name, int col = 1);
  GTMConfig(char delim, int col = 1);
  GTMConfig(const char* name, char delim, int col = 1);
  GTMConfig(char delim, int col, const char* buf);
  int init(const char* name, char delim, int col);
  int init_from_string(const char* buf, char delim, int col);

  ~GTMConfig(void);
  
  const char* value(string& name, int col = 1);
  const char* value(const string& block, string& name, int col = 1);
  const char* operator()(string& name, int col = 1);
  const char* operator()(const string& block, string& name, int col = 1);

  const char* _ivalue(const char* block, const char* name, int index, int col);
  const char* ivalue(int index, int col = 1);
  const char* ivalue(const char* block, int index, int col = 1);
  const char* ivalue(const char* block, const char* name, int index, int col = 1);

  const char* keyword(int index);
  const char* block(int index);
};

#endif
