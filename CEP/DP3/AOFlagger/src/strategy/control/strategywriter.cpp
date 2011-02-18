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
#include <AOFlagger/strategy/control/strategywriter.h>

#include <AOFlagger/strategy/actions/adapter.h>
#include <AOFlagger/strategy/actions/addstatisticsaction.h>
#include <AOFlagger/strategy/actions/baselineselectionaction.h>
#include <AOFlagger/strategy/actions/changeresolutionaction.h>
#include <AOFlagger/strategy/actions/combineflagresultsaction.h>
#include <AOFlagger/strategy/actions/cutareaaction.h>
#include <AOFlagger/strategy/actions/directionalcleanaction.h>
#include <AOFlagger/strategy/actions/foreachbaselineaction.h>
#include <AOFlagger/strategy/actions/foreachcomplexcomponentaction.h>
#include <AOFlagger/strategy/actions/foreachmsaction.h>
#include <AOFlagger/strategy/actions/foreachpolarisationaction.h>
#include <AOFlagger/strategy/actions/fouriertransformaction.h>
#include <AOFlagger/strategy/actions/frequencyconvolutionaction.h>
#include <AOFlagger/strategy/actions/frequencyselectionaction.h>
#include <AOFlagger/strategy/actions/fringestopaction.h>
#include <AOFlagger/strategy/actions/imageraction.h>
#include <AOFlagger/strategy/actions/iterationaction.h>
#include <AOFlagger/strategy/actions/plotaction.h>
#include <AOFlagger/strategy/actions/quickcalibrateaction.h>
#include <AOFlagger/strategy/actions/setflaggingaction.h>
#include <AOFlagger/strategy/actions/setimageaction.h>
#include <AOFlagger/strategy/actions/slidingwindowfitaction.h>
#include <AOFlagger/strategy/actions/statisticalflagaction.h>
#include <AOFlagger/strategy/actions/strategyaction.h>
#include <AOFlagger/strategy/actions/svdaction.h>
#include <AOFlagger/strategy/actions/thresholdaction.h>
#include <AOFlagger/strategy/actions/timeconvolutionaction.h>
#include <AOFlagger/strategy/actions/timeselectionaction.h>
#include <AOFlagger/strategy/actions/uvprojectaction.h>
#include <AOFlagger/strategy/actions/writedataaction.h>
#include <AOFlagger/strategy/actions/writeflagsaction.h>

namespace rfiStrategy {

	void StrategyWriter::write(const Strategy &strategy)
	{
		_describedActions.clear();

		std::string commentStr = 
			"This is a Strategy configuration file for the\n"
			"rfi detector by André Offringa (offringa@astro.rug.nl).\n";
		if(_writeDescriptions)
			commentStr += "\nIf you like to take a look at the structure of this file,\n"
				"try opening it in e.g. Firefox.\n";

		Comment(commentStr.c_str());

		Start("rfi-strategy");
		Attribute("format-version", STRATEGY_FILE_FORMAT_VERSION);
		Attribute("reader-version-required", STRATEGY_FILE_READER_VERSION_REQUIRED);
		writeAction(strategy);
		End();

		Close();
	}
	
	void StrategyWriter::writeAction(const Action &action)
	{
		if(_writeDescriptions)
		{
			if(_describedActions.count(action.Type()) == 0)
			{
				const char *description = ActionFactory::GetDescription(action.Type());
				if(description != 0)
					Comment(wrap(description, 70).c_str());
				_describedActions.insert(action.Type());
			}
		}

		Start("action");
		switch(action.Type())
		{
			case ActionBlockType:
				throw std::runtime_error("Can not store action blocks");
			case AdapterType:
				writeAdapter(static_cast<const Adapter&>(action));
				break;
			case AddStatisticsActionType:
				writeAddStatisticsAction(static_cast<const AddStatisticsAction&>(action));
				break;
			case BaselineSelectionActionType:
				writeBaselineSelectionAction(static_cast<const BaselineSelectionAction&>(action));
				break;
			case ChangeResolutionActionType:
				writeChangeResolutionAction(static_cast<const ChangeResolutionAction&>(action));
				break;
			case CombineFlagResultsType:
				writeCombineFlagResults(static_cast<const CombineFlagResults&>(action));
				break;
			case CutAreaActionType:
				writeCutAreaAction(static_cast<const CutAreaAction&>(action));
				break;
			case DirectionalCleanActionType:
				writeDirectionalCleanAction(static_cast<const DirectionalCleanAction&>(action));
				break;
			case ForEachBaselineActionType:
				writeForEachBaselineAction(static_cast<const ForEachBaselineAction&>(action));
				break;
			case ForEachComplexComponentActionType:
				writeForEachComplexComponentAction(static_cast<const ForEachComplexComponentAction&>(action));
				break;
			case ForEachMSActionType:
				writeForEachMSAction(static_cast<const ForEachMSAction&>(action));
				break;
			case ForEachPolarisationBlockType:
				writeForEachPolarisationBlock(static_cast<const ForEachPolarisationBlock&>(action));
				break;
			case FourierTransformActionType:
				writeFourierTransformAction(static_cast<const FourierTransformAction&>(action));
				break;
			case FrequencyConvolutionActionType:
				writeFrequencyConvolutionAction(static_cast<const FrequencyConvolutionAction&>(action));
				break;
			case FrequencySelectionActionType:
				writeFrequencySelectionAction(static_cast<const FrequencySelectionAction&>(action));
				break;
			case FringeStopActionType:
				writeFringeStopAction(static_cast<const FringeStopAction&>(action));
				break;
			case ImagerActionType:
				writeImagerAction(static_cast<const ImagerAction&>(action));
				break;
			case IterationBlockType:
				writeIterationBlock(static_cast<const IterationBlock&>(action));
				break;
			case PlotActionType:
				writePlotAction(static_cast<const PlotAction&>(action));
				break;
			case QuickCalibrateActionType:
				writeQuickCalibrateAction(static_cast<const QuickCalibrateAction&>(action));
				break;
			case SetFlaggingActionType:
				writeSetFlaggingAction(static_cast<const SetFlaggingAction&>(action));
				break;
			case SetImageActionType:
				writeSetImageAction(static_cast<const SetImageAction&>(action));
				break;
			case SlidingWindowFitActionType:
				writeSlidingWindowFitAction(static_cast<const SlidingWindowFitAction&>(action));
				break;
			case StatisticalFlagActionType:
				writeStatisticalFlagAction(static_cast<const StatisticalFlagAction&>(action));
				break;
			case StrategyType:
				writeStrategy(static_cast<const Strategy&>(action));
				break;
			case SVDActionType:
				writeSVDAction(static_cast<const SVDAction&>(action));
				break;
			case ThresholdActionType:
				writeThresholdAction(static_cast<const ThresholdAction&>(action));
				break;
			case TimeConvolutionActionType:
				writeTimeConvolutionAction(static_cast<const TimeConvolutionAction&>(action));
			break;
			case TimeSelectionActionType:
				writeTimeSelectionAction(static_cast<const TimeSelectionAction&>(action));
				break;
			case UVProjectActionType:
				writeUVProjectAction(static_cast<const UVProjectAction&>(action));
				break;
			case WriteDataActionType:
				writeWriteDataAction(static_cast<const WriteDataAction&>(action));
				break;
			case WriteFlagsActionType:
				writeWriteFlagsAction(static_cast<const WriteFlagsAction&>(action));
				break;
		}
		End();
	}
	
	void StrategyWriter::writeContainerItems(const ActionContainer &actionContainer)
	{
		Start("children");
		for(size_t i=0;i<actionContainer.GetChildCount();++i)
		{
			writeAction(actionContainer.GetChild(i));
		}
		End();
	}
	
	void StrategyWriter::writeAdapter(const Adapter &action)
	{
		Attribute("type", "Adapter");
		Write<bool>("restore-originals", action.RestoreOriginals());
		writeContainerItems(action);
	}

	void StrategyWriter::writeAddStatisticsAction(const AddStatisticsAction &action)
	{
		Attribute("type", "AddStatisticsAction");
		Write("file-prefix", action.FilePrefix().c_str());
		Write("compare-original-and-alternative", action.CompareOriginalAndAlternative());
		Write("separate-baseline-statistics", action.SeparateBaselineStatistics());
	}

	void StrategyWriter::writeBaselineSelectionAction(const class BaselineSelectionAction &action)
	{
		Attribute("type", "BaselineSelectionAction");
		Write<bool>("preparation-step", action.PreparationStep());
		Write<bool>("flag-bad-baselines", action.FlagBadBaselines());
		Write<num_t>("threshold", action.Threshold());
		Write<num_t>("smoothing-sigma", action.SmoothingSigma());
		Write<num_t>("abs-threshold", action.AbsThreshold());
		Write<bool>("make-plot", action.MakePlot());
	}

	void StrategyWriter::writeChangeResolutionAction(const ChangeResolutionAction &action)
	{
		Attribute("type", "ChangeResolutionAction");
		Write<int>("time-decrease-factor", action.TimeDecreaseFactor());
		Write<int>("frequency-decrease-factor", action.FrequencyDecreaseFactor());
		Write<bool>("restore-revised", action.RestoreRevised());
		Write<bool>("restore-masks", action.RestoreMasks());
		writeContainerItems(action);
	}

	void StrategyWriter::writeCombineFlagResults(const CombineFlagResults &action)
	{
		Attribute("type", "CombineFlagResults");
		writeContainerItems(action);
	}

	void StrategyWriter::writeCutAreaAction(const CutAreaAction &action)
	{
		Attribute("type", "CutAreaAction");
		Write<int>("start-time-steps", action.StartTimeSteps());
		Write<int>("end-time-steps", action.EndTimeSteps());
		Write<int>("top-channels", action.TopChannels());
		Write<int>("bottom-channels", action.BottomChannels());
		writeContainerItems(action);
	}

	void StrategyWriter::writeDirectionalCleanAction(const DirectionalCleanAction &action)
	{
		Attribute("type", "DirectionalCleanAction");
		Write<double>("limiting-distance", action.LimitingDistance());
		Write<int>("channel-convolution-size", action.ChannelConvolutionSize());
		Write<double>("attenuation-of-center", action.AttenuationOfCenter());
		Write<bool>("make-plot", action.MakePlot());
	}

	void StrategyWriter::writeForEachBaselineAction(const ForEachBaselineAction &action)
	{
		Attribute("type", "ForEachBaselineAction");
		Write<int>("selection", action.Selection());
		Write<int>("thread-count", action.ThreadCount());
		writeContainerItems(action);
	}

	void StrategyWriter::writeForEachComplexComponentAction(const ForEachComplexComponentAction &action)
	{
		Attribute("type", "ForEachComplexComponentAction");
		Write<bool>("on-amplitude", action.OnAmplitude());
		Write<bool>("on-phase", action.OnPhase());
		Write<bool>("on-real", action.OnReal());
		Write<bool>("on-imaginary", action.OnImaginary());
		Write<bool>("restore-from-amplitude", action.RestoreFromAmplitude());
		writeContainerItems(action);
	}

	void StrategyWriter::writeForEachMSAction(const ForEachMSAction &action)
	{
		Attribute("type", "ForEachMSAction");
		Start("filenames");
		Write("data-column-name", action.DataColumnName());
		Write<bool>("subtract-model", action.SubtractModel());
		const std::vector<std::string> &filenames = action.Filenames();
		for(std::vector<std::string>::const_iterator i=filenames.begin();i!=filenames.end();++i)
		{
			Write("filename", i->c_str());
		}
		End();
		writeContainerItems(action);
	}

	void StrategyWriter::writeForEachPolarisationBlock(const ForEachPolarisationBlock &action)
	{
		Attribute("type", "ForEachPolarisationBlock");
		Write<bool>("on-xx", action.OnXX());
		Write<bool>("on-xy", action.OnXY());
		Write<bool>("on-yx", action.OnYX());
		Write<bool>("on-yy", action.OnYY());
		Write<bool>("on-stokes-i", action.OnStokesI());
		Write<bool>("on-stokes-q", action.OnStokesQ());
		Write<bool>("on-stokes-u", action.OnStokesU());
		Write<bool>("on-stokes-v", action.OnStokesV());
		writeContainerItems(action);
	}

	void StrategyWriter::writeFourierTransformAction(const FourierTransformAction &)
	{
		Attribute("type", "FourierTransformAction");
	}

	void StrategyWriter::writeFrequencyConvolutionAction(const FrequencyConvolutionAction &action)
	{
		Attribute("type", "FrequencyConvolutionAction");
		Write<double>("convolution-size", action.ConvolutionSize());
		Write<int>("kernel-kind", (int) action.KernelKind());
	}

	void StrategyWriter::writeFrequencySelectionAction(const FrequencySelectionAction &action)
	{
		Attribute("type", "FrequencySelectionAction");
		Write<double>("threshold", action.Threshold());
	}

	void StrategyWriter::writeFringeStopAction(const FringeStopAction &action)
	{
		Attribute("type", "FringeStopAction");
		Write<bool>("fit-channels-individually", action.FitChannelsIndividually());
		Write<num_t>("fringes-to-consider", action.FringesToConsider());
		Write<bool>("only-fringe-stop", action.OnlyFringeStop());
		Write<int>("min-window-size", action.MinWindowSize());
		Write<int>("max-window-size", action.MaxWindowSize());
	}

	void StrategyWriter::writeImagerAction(const ImagerAction &)
	{
		Attribute("type", "ImagerAction");
	}

	void StrategyWriter::writeIterationBlock(const IterationBlock &action)
	{
		Attribute("type", "IterationBlock");
		Write<int>("iteration-count", action.IterationCount());
		Write<double>("sensitivity-start", action.SensitivityStart());
		writeContainerItems(action);
	}

	void StrategyWriter::writePlotAction(const class PlotAction &action)
	{
		Attribute("type", "PlotAction");
		Write<int>("plot-kind", action.PlotKind());
		Write<bool>("logarithmic-y-axis", action.LogarithmicYAxis());
	}

	void StrategyWriter::writeQuickCalibrateAction(const QuickCalibrateAction &)
	{
		Attribute("type", "QuickCalibrateAction");
	}

	void StrategyWriter::writeSetFlaggingAction(const SetFlaggingAction &action)
	{
		Attribute("type", "SetFlaggingAction");
		Write<int>("new-flagging", action.NewFlagging());
	}

	void StrategyWriter::writeSetImageAction(const SetImageAction &action)
	{
		Attribute("type", "SetImageAction");
		Write<int>("new-image", action.NewImage());
	}

	void StrategyWriter::writeSlidingWindowFitAction(const SlidingWindowFitAction &action)
	{
		Attribute("type", "SlidingWindowFitAction");
		Write<num_t>("frequency-direction-kernel-size", action.Parameters().frequencyDirectionKernelSize);
		Write<int>("frequency-direction-window-size", action.Parameters().frequencyDirectionWindowSize);
		Write<int>("method", action.Parameters().method);
		Write<num_t>("time-direction-kernel-size", action.Parameters().timeDirectionKernelSize);
		Write<int>("time-direction-window-size", action.Parameters().timeDirectionWindowSize);
	}

	void StrategyWriter::writeStatisticalFlagAction(const StatisticalFlagAction &action)
	{
		Attribute("type", "StatisticalFlagAction");
		Write<size_t>("enlarge-frequency-size", action.EnlargeFrequencySize());
		Write<size_t>("enlarge-time-size", action.EnlargeTimeSize());
		Write<num_t>("max-contaminated-frequencies-ratio", action.MaxContaminatedFrequenciesRatio());
		Write<num_t>("max-contaminated-times-ratio", action.MaxContaminatedTimesRatio());
		Write<num_t>("minimum-good-frequency-ratio", action.MinimumGoodFrequencyRatio());
		Write<num_t>("minimum-good-time-ratio", action.MinimumGoodTimeRatio());
	}

	void StrategyWriter::writeStrategy(const class Strategy &action)
	{
		Attribute("type", "Strategy");
		writeContainerItems(action);
	}

	void StrategyWriter::writeSVDAction(const SVDAction &action)
	{
		Attribute("type", "SVDAction");
		Write<int>("singular-value-count", action.SingularValueCount());
	}

	void StrategyWriter::writeThresholdAction(const ThresholdAction &action)
	{
		Attribute("type", "ThresholdAction");
		Write<num_t>("base-sensitivity", action.BaseSensitivity());
		Write<bool>("time-direction-flagging", action.TimeDirectionFlagging());
		Write<bool>("frequency-direction-flagging", action.FrequencyDirectionFlagging());
	}

	void StrategyWriter::writeTimeConvolutionAction(const TimeConvolutionAction &action)
	{
		Attribute("type", "TimeConvolutionAction");
		Write<int>("operation", (int) action.Operation());
		Write<num_t>("sinc-scale", action.SincScale());
		Write<bool>("is-sinc-scale-in-samples", action.IsSincScaleInSamples());
		Write<num_t>("direction-rad", action.DirectionRad());
		Write<num_t>("eta-parameter", action.EtaParameter());
		Write<bool>("auto-angle", action.AutoAngle());
		Write<unsigned>("iterations", action.Iterations());
	}

	void StrategyWriter::writeTimeSelectionAction(const TimeSelectionAction &action)
	{
		Attribute("type", "TimeSelectionAction");
		Write<double>("threshold", action.Threshold());
	}

	void StrategyWriter::writeUVProjectAction(const UVProjectAction &action)
	{
		Attribute("type", "UVProjectAction");
		Write<numl_t>("direction-rad", action.DirectionRad());
		Write<numl_t>("eta-parameter", action.EtaParameter());
		Write<num_t>("dest-resolution-factor", action.DestResolutionFactor());
		Write<bool>("reverse", action.Reverse());
		Write<bool>("on-revised", action.OnRevised());
		Write<bool>("on-contaminated", action.OnContaminated());
	}
	
	void StrategyWriter::writeWriteDataAction(const WriteDataAction &)
	{
		Attribute("type", "WriteDataAction");
	}

	void StrategyWriter::writeWriteFlagsAction(const WriteFlagsAction &)
	{
		Attribute("type", "WriteFlagsAction");
	}

	std::string StrategyWriter::wrap(const std::string &input, size_t max) const
	{
		int start = 0;
		bool first = true;
		std::stringstream s;
		int length = input.size();
		while(start < length)
		{
			int end = start + max;
			if(end > length)
				end = length;
			else {
				do {
					--end;
				} while(end > start && input[end] != ' ');
				if(end <= start)
					end = start + max;
				else
					++end;
			}
			int nextStart = end;
			while(end > start && input[end-1] == ' ') --end;

			if(!first)
				s << "\n";
			for(int i=start;i<end;++i)
				s << input[i];
				
			first = false;
			start = nextStart;
		}
		return s.str();
	}
}
