### parmtable.g: Glish script to add parameters to the MEP table
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

# pragma include once
print 'include parmtable.g   d28jan2004';

include 'table.g'

parmtable := function (name, create=F)
{
    self:=[=];
    public:=[=];

    if (create) {
	# Create the main table.
	d1 := tablecreatescalarcoldesc ('NAME', '');
	d4 := tablecreatescalarcoldesc ('STARTTIME', as_double(0));
	d5 := tablecreatescalarcoldesc ('ENDTIME', as_double(0));
	d6 := tablecreatescalarcoldesc ('STARTFREQ', as_double(0));
	d7 := tablecreatescalarcoldesc ('ENDFREQ', as_double(0));
	d8 := tablecreatearraycoldesc  ('VALUES', as_double(0));
	d11:= tablecreatescalarcoldesc ('TIME0', as_double(0));
	d12:= tablecreatescalarcoldesc ('FREQ0', as_double(0));
	d13:= tablecreatescalarcoldesc ('NORMALIZED', T);
	d14:= tablecreatearraycoldesc  ('SOLVABLE', T);
	d15:= tablecreatescalarcoldesc ('DIFF', as_double(0));
	d16:= tablecreatescalarcoldesc ('DIFF_REL', T);
	td := tablecreatedesc (d1, d4, d5, d6, d7, d8,
			       d11, d12, d13, d14, d15, d16);
	self.tab := table (name, td);
	if (is_fail(self.tab)) {
	    fail;
	}
	info := self.tab.info();
	info.type := 'MEP';
	self.tab.putinfo (info);
	self.tab.addreadmeline ('PSS ME parameter values');
	
	# Create the table with default values.
	dtabname := spaste(name,'/DEFAULTVALUES');
	td := tablecreatedesc (d1, d8, d11, d12, d13, d14, d15, d16);
	self.dtab := table (dtabname, td);
	if (is_fail(self.dtab)) {
	    fail;
	}
	info := self.dtab.info();
	info.type := 'MEPdef';
	self.dtab.putinfo (info);
	self.dtab.addreadmeline ('Default PSS ME parameter values');
	
	# Make it a subtable of the main table.
	self.tab.putkeyword ('DEFAULTVALUES', spaste('Table: ',dtabname));
    } else {
	self.tab := table (name, readonly=F);
	if (is_fail(self.tab)) {
	    fail;
	}
	kws := self.tab.getkeywords();
	if (has_field (kws, 'DEFAULTVALUES')) {
	    self.dtab := table (kws.DEFAULTVALUES, readonly=F);
	    if (is_fail(self.dtab)) {
		fail;
	    }
	} else {
	    self.dtab := F;
	}
    }

    public.done := function()
    {
	wider self, public;
	self.tab.close();
	if (is_record(self.dtab)) self.dtab.close();
	val self := F;
	val public := F;
	return T;
    }

    public.putdef := function (parmname='parmXXX',
			       values=0, solvable=unset, normalize=unset,
			       diff=1e-6, diffrelative=T,
			       time0=0., freq0=0., trace=T)
    {
	#----------------------------------------------------------------
	funcname := paste('** parmtable.putdef(',parmname,'):');
	input := [parmname=parmname, values=values, solvable=solvable,
		  diff=diff, diffrelative=diffrelative,
		  time0=time0, freq0=freq0];
	if (trace) print funcname,' input=',input;
	#----------------------------------------------------------------

	if (!is_record(self.dtab)) {
	    fail "No DEFAULTVALUES subtable";
	}
	t1 := self.dtab.query (spaste('NAME=="',parmname,'"'));
	nr := t1.nrows();
	t1.close();
	if (nr != 0) {
	    fail paste('Parameter',parmname,'already has a default value');
	}
	# Turn a scalar into a matrix.
	if (length(shape(values)) == 1  &&  length(values) == 1) {
	    values := array (values, 1, 1);
	}
	if (length(shape(values)) != 2  ||  !is_numeric(values)) {
	    fail paste('values should be a 2-dim numerical array');
	}
	if (!is_unset(solvable)) {
  	  if (length(shape(solvable)) == 1  &&  length(solvable) == 1) {
	    solvable := array (solvable, 1, 1);
	  }
          if (!is_boolean(solvable) || length(shape(solvable)) != 2) {
	    fail paste('solvable should be unset or a 2-dim boolean array');
          }
	}
	self.dtab.addrows(1);
	rownr := self.dtab.nrows();
	self.dtab.putcell ('NAME', rownr, parmname);
	nm := T;
	if (is_boolean(solvable)) {
	    self.dtab.putcell ('SOLVABLE', rownr, solvable);
	    nm := all(solvable);
	}
	if (is_boolean(normalize)) {
	    nm := normalize;
	}
	self.dtab.putcell ('TIME0', rownr, time0)
	self.dtab.putcell ('FREQ0', rownr, freq0)
	self.dtab.putcell ('NORMALIZED', rownr, nm);
	vals := as_double(values);
	self.dtab.putcell ('VALUES', rownr, vals);
	self.dtab.putcell ('DIFF', rownr, diff);
	self.dtab.putcell ('DIFF_REL', rownr, diffrelative);
	return T;
    }

    public.put := function (parmname='parmYYY',
			    timerange=[1,1e20], freqrange=[1,1e20], 
			    values=0, solvable=unset, normalize=unset,
                            diff=1e-6, diffrelative=T,
			    time0=0., freq0=0., trace=T)
    {
	#----------------------------------------------------------------
	funcname := paste('** parmtable.put(',parmname,'):');
	input := [parmname=parmname, values=values, solvable=solvable,
		  timerange=timerange, freqrange=freqrange,
		  diff=diff, diffrelative=diffrelative,
		  time0=time0, freq0=freq0];
	if (trace) print funcname,' input=',input;
	#----------------------------------------------------------------


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
	    fail paste('Parameter',parmname,
                       'already defined for given domain');
	}
	# Turn a scalar into a matrix.
	if (length(shape(values)) == 1  &&  length(values) == 1) {
	    values := array (values, 1, 1);
	}
	if (length(shape(values)) != 2  ||  !is_numeric(values)) {
	    fail paste('values should be a 2-dim numerical array');
	}
	if (!is_unset(solvable)  &&  (!is_boolean(solvable) || length(shape(solvable)) != 2)) {
	    fail paste('solvable should be unset or a 2-dim boolean array');
	}
	self.tab.addrows(1);
	rownr := self.tab.nrows();
	self.tab.putcell ('NAME', rownr, parmname);
	self.tab.putcell ('STARTTIME', rownr, timerange[1]);
	self.tab.putcell ('ENDTIME', rownr, timerange[2]);
	self.tab.putcell ('STARTFREQ', rownr, freqrange[1]);
	self.tab.putcell ('ENDFREQ', rownr, freqrange[2]);
	nm := T;
	if (is_boolean(solvable)) {
	    self.tab.putcell ('SOLVABLE', rownr, solvable);
	    nm := all(solvable);
	}
	if (is_boolean(normalize)) {
	    nm := normalize;
	}
	self.tab.putcell ('TIME0', rownr, time0)
	self.tab.putcell ('FREQ0', rownr, freq0)
	self.tab.putcell ('NORMALIZED', rownr, nm);
	vals := as_double(values);
	self.tab.putcell ('VALUES', rownr, vals);
	self.tab.putcell ('DIFF', rownr, diff);
	self.tab.putcell ('DIFF_REL', rownr, diffrelative);
	return T;
    }

    public.loadgsm := function(gsmname, where='', time0=0., freq0=0.)
    {
	t := table(gsmname);
	if (is_fail(t)) fail;
	tab := t.query (where, sortlist='NUMBER');
	fnd := tab.nrows() > 0;
	if (fnd) {
	    for (row in [1:tab.nrows()]) {
		src := tab.getcell ('NUMBER', row);
		name := tab.getcell('NAME', row);
		public.putdef (spaste('RA.', name),
			       values=tab.getcell ('RAPARMS', row),
			       diff=1e-7, diffrelative=F,
			       time0=time0, freq0=freq0);
		public.putdef (spaste('DEC.', name),
			       values=tab.getcell ('DECPARMS', row),
			       diff=1e-7, diffrelative=F,
			       time0=time0, freq0=freq0);
		public.putdef (spaste('StokesI.', name),
			       values=tab.getcell ('IPARMS', row),
			       time0=time0, freq0=freq0);
		public.putdef (spaste('StokesQ.', name),
			       values=tab.getcell ('QPARMS', row),
			       time0=time0, freq0=freq0);
		public.putdef (spaste('StokesU.', name),
			       values=tab.getcell ('UPARMS', row),
			       time0=time0, freq0=freq0);
		public.putdef (spaste('StokesV.', name),
			       values=tab.getcell ('VPARMS', row),
			       time0=time0, freq0=freq0);
	    }
	}
	print 'Wrote',tab.nrows(),'sources into MEP';
	tab.done();
	t.done();
	if (!fnd) {
	    fail 'no sources found in the gsm';
	}
	return T;
    }

    self.perturb := function (tab, where, perturbation, pertrelative)
    {
	t1 := ref tab;
	if (where != '') {
	    t1 := tab.query (where);
	}
	if (is_fail(t1)) fail;
	if (t1.nrows() > 0) {
	    for (row in [1:t1.nrows()]) {
		vals := t1.getcell ('VALUES', row);
		if (pertrelative) {
		    valp := vals * perturbation;
		    valp[abs(vals)<1e-10] := vals + perturbation;
		} else {
		    valp := vals + perturbation;
		}
		t1.putcell ('VALUES', row, valp);
	    }
	}
	if (where != '') {
	    t1.close();
	}
	return T;
    }

    public.perturb := function (where='',
				perturbation=1e-6, pertrelative=T,
				trace=F)
    {
	#----------------------------------------------------------------
	funcname := paste('** parmtable.perturb(',where,'):');
	input := [where=where,
		  perturbation=perturbation, pertrelative=pertrelative];
	if (trace) print funcname,' input=',input;
	#----------------------------------------------------------------

	if (is_record(self.dtab)) {
	    self.perturb(self.dtab, where, perturbation, pertrelative);
	}
	self.perturb(self.tab, where, perturbation, pertrelative);
	return T;
    }

    public.table := function()
    {
	return ref self.tab;
    }

    public.deftable := function()
    {
	return ref self.dtab;
    }

    return ref public;
}
