/***************************************************************************
 *   Copyright (C) 2008 by A.R. Offringa   *
 *   offringa@astro.rug.nl   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef GUI_QUALITY__2DPLOTPAGE_H
#define GUI_QUALITY__2DPLOTPAGE_H

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/window.h>
#include <gtkmm/frame.h>

#include <AOFlagger/quality/qualitytablesformatter.h>

#include <AOFlagger/gui/plot/plot2d.h>
#include <AOFlagger/gui/plot/plotwidget.h>

/**
	@author A.R. Offringa <offringa@astro.rug.nl>
*/
class TwoDimensionalPlotPage : public Gtk::HBox {
	public:
		TwoDimensionalPlotPage();
    virtual ~TwoDimensionalPlotPage()
    {
		}
		
		void SetStatistics(class StatisticsCollection *statCollection, const std::string &filename)
		{
			processStatistics(statCollection, filename);
			
			_statCollection = statCollection;
			updatePlot();
		}
		void CloseStatistics()
		{
			_statCollection = 0;
		}
		bool HasStatistics() const
		{
			return _statCollection != 0;
		}
	protected:
		virtual void processStatistics(class StatisticsCollection *, const std::string &)
		{
		}
		
		virtual const std::map<double, class DefaultStatistics> &GetStatistics() const = 0;
		
		virtual void StartLine(Plot2D &plot, const std::string &name) = 0;
		
		class StatisticsCollection *GetStatCollection() const
		{
			return _statCollection;
		}
	private:
		enum PhaseType { AmplitudePhaseType, PhasePhaseType, RealPhaseType, ImaginaryPhaseType} ;
		
		void updatePlot();
		template<enum PhaseType Phase>
		inline double getValue(const std::complex<long double> val);
		void plotStatistic(QualityTablesFormatter::StatisticKind kind);
		void plotPolarization(QualityTablesFormatter::StatisticKind kind, unsigned polarization);
		void plotPolarization(QualityTablesFormatter::StatisticKind kind, unsigned polarizationA, unsigned polarizationB);
		template<enum PhaseType Phase>
		void plotPhase(QualityTablesFormatter::StatisticKind kind, unsigned polarization);
		template<enum PhaseType Phase>
		void plotPhase(QualityTablesFormatter::StatisticKind kind, unsigned polarizationA, unsigned polarizationB);
		
		void initStatisticKindButtons();
		void initPolarizationButtons();
		void initPhaseButtons();
		
		Gtk::VBox _sideBox;
		
		Gtk::Frame _statisticFrame;
		Gtk::VBox _statisticBox;
		Gtk::CheckButton _countButton, _meanButton, _varianceButton, _dCountButton, _dMeanButton, _dVarianceButton, _rfiRatioButton;
		
		Gtk::Frame _polarizationFrame;
		Gtk::VBox _polarizationBox;
		Gtk::CheckButton _polXXButton, _polXYButton, _polYXButton, _polYYButton, _polXXandYYButton, _polXYandYXButton;
		
		Gtk::Frame _phaseFrame;
		Gtk::VBox _phaseBox;
		Gtk::CheckButton _amplitudeButton, _phaseButton, _realButton, _imaginaryButton;
		
		class StatisticsCollection *_statCollection;
		Plot2D _plot;
		PlotWidget _plotWidget;
};

#endif
