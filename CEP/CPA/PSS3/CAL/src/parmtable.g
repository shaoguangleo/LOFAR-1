### TFStoredParmPolc.cc: Stored parameter with polynomial coefficients
###
### Copyright (C) 2002
### ASTRON (Netherlands Foundation for Research in Astronomy)
### P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
###
### This program is free software; you can redistribute it and/or modify
### it under the terms of the GNU General Public License as published by
### the Free Software Foundation; either version 2 of the License, or
### (at your option) any later version.
###
### This program is distributed in the hope that it will be useful,
### but WITHOUT ANY WARRANTY; without even the implied warranty of
### MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
### GNU General Public License for more details.
###
### You should have received a copy of the GNU General Public License
### along with this program; if not, write to the Free Software
### Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
###
### $Id$

pragma include once

include 'table.g'

parmtable := function (name, create=F)
{
    self:=[=];
    public:=[=];

    if (create) {
	# Create the main table.
	d1 := tablecreatescalarcoldesc ('NAME', '');
	d3 := tablecreatescalarcoldesc ('STARTTIME', as_double(0));
	d4 := tablecreatescalarcoldesc ('ENDTIME', as_double(0));
	d5 := tablecreatescalarcoldesc ('STARTFREQ', as_double(0));
	d6 := tablecreatescalarcoldesc ('ENDFREQ', as_double(0));
	d7 := tablecreatearraycoldesc  ('RVALUES', as_double(0));
	d8 := tablecreatearraycoldesc  ('CVALUES', as_dcomplex(0));
	d9 := tablecreatearraycoldesc  ('MASK', T);
	td := tablecreatedesc (d1, d3, d4, d5, d6, d7, d8, d9);
	self.tab := table (name, td);
	if (is_fail(self.tab)) {
	    fail;
	}
	info := self.tab.info();
	info.type := 'MEP';
	self.tab.putinfo (info);
	self.tab.addreadmeline ('PSS ME parameter values');
	
	# Create the table with initial values.
	itabname := spaste(name,'/INITIALVALUES');
	td := tablecreatedesc (d1, d7, d8, d9);
	self.itab := table (itabname, td);
	if (is_fail(self.itab)) {
	    fail;
	}
	info := self.itab.info();
	info.type := 'MEPinit';
	self.itab.putinfo (info);
	self.itab.addreadmeline ('Initial PSS ME parameter values');
	
	# Make it a subtable of the main table.
	self.tab.putkeyword ('INITIALVALUES', spaste('Table: ',itabname));
    } else {
	self.tab := table (name, readonly=F);
	if (is_fail(self.tab)) {
	    fail;
	}
	self.itab := table (self.tab.getkeyword ('INITIALVALUES'), readonly=F);
	if (is_fail(self.itab)) {
	    fail;
	}
    }

    public.done := function()
    {
	wider self, public;
	self.tab.close();
	self.itab.close();
	val self := F;
	val public := F;
	return T;
    }

    public.putinit := function (parmname, values, mask=unset)
    {
	t1 := self.itab.query (spaste('NAME=="',parmname,'"'));
	nr := t1.nrows();
	t1.close();
	if (nr != 0) {
	    fail paste('Parameter',parmname,'already has an initial value');
	}
	if (length(shape(values)) != 2  ||  !is_numeric(values)) {
	    fail paste('values should be a 2-dim numerical array');
	}
	if (!is_unset(mask)  &&  (!is_boolean(mask) || length(shape(mask)) != 2)) {
	    fail paste('mask should be unset or a 2-dim boolean array');
	}
	self.itab.addrows(1);
	rownr := self.itab.nrows();
	self.itab.putcell ('NAME', rownr, parmname);
	if (! is_unset(mask)) {
	    self.itab.putcell ('MASK', rownr, mask);
	}
	if (is_dcomplex(values) || is_complex(values)) {
	    self.itab.putcell ('CVALUES', rownr, as_dcomplex(values));
	} else {
	    self.itab.putcell ('RVALUES', rownr, as_double(values));
	}
    }

    public.put := function (parmname, timerange, freqrange, values, mask=unset)
    {
	if (length(timerange) != 2) {
	    fail 'timerange should be a vector of 2 elements (start,end)';
	}
	if (length(freqrange) != 2) {
	    fail 'freqrange should be a vector of 2 elements (start,end)';
	}
	t1 := self.tab.query (spaste('NAME=="',parmname,'" ',
				     '&& near(STARTTIME,', timerange[1],') ',
				     '&& near(ENDTIME,'  , timerange[2],') ',
				     '&& near(STARTFREQ,', freqrange[1],') ',
				     '&& near(ENDFREQ,'  , freqrange[2],') '));
	nr := t1.nrows();
	t1.close();
	if (nr != 0) {
	    fail paste('Parameter',parmname,'already defined for given domain');
	}
	if (length(shape(values)) != 2  ||  !is_numeric(values)) {
	    fail paste('values should be a 2-dim numerical array');
	}
	if (!is_unset(mask)  &&  (!is_boolean(mask) || length(shape(mask)) != 2)) {
	    fail paste('mask should be unset or a 2-dim boolean array');
	}
	self.tab.addrows(1);
	rownr := self.tab.nrows();
	self.tab.putcell ('NAME', rownr, parmname);
	self.tab.putcell ('STARTTIME', rownr, timerange[1]);
	self.tab.putcell ('ENDTIME', rownr, timerange[2]);
	self.tab.putcell ('STARTFREQ', rownr, freqrange[1]);
	self.tab.putcell ('ENDFREQ', rownr, freqrange[2]);
	if (! is_unset(mask)) {
	    self.tab.putcell ('MASK', rownr, mask);
	}
	if (is_dcomplex(values) || is_complex(values)) {
	    self.tab.putcell ('CVALUES', rownr, as_dcomplex(values));
	} else {
	    self.tab.putcell ('RVALUES', rownr, as_double(values));
	}
    }

    return ref public;
}
