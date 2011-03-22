
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iostream>

#include <AOFlagger/msio/date.h>

typedef std::pair<double, std::string> Tick;

class TickSet
{
	public:
		TickSet()
		{
		}
		virtual ~TickSet()
		{
		}
		
		virtual unsigned Size() const = 0;
		virtual Tick GetTick(unsigned i) const = 0;
		
		virtual void DecreaseTicks()
		{
			if(Size() > 1)
			{
				Reset(Size() - 1);
			}
		}
		virtual void Reset(unsigned sizeRequest) = 0;
	protected:
	private:
		
};

class NumericTickSet : public TickSet
{
	public:
		NumericTickSet(double min, double max, unsigned sizeRequest) : _min(min), _max(max)
		{
			set(sizeRequest);
		}
		
		virtual unsigned Size() const
		{
			return _ticks.size();
		}
		
		virtual Tick GetTick(unsigned i) const
		{
			std::stringstream tickStr;
			tickStr << _ticks[i];
			return Tick((_ticks[i] - _min) / (_max - _min), tickStr.str());
		}
		
		virtual void Reset(unsigned sizeRequest)
		{
			_ticks.clear();
			set(sizeRequest);
		}
	private:
		void set(unsigned sizeRequest)
		{
			if(_max == _min)
				_ticks.push_back(_min);
			else
			{
				if(sizeRequest == 0)
					sizeRequest = 1;
				double
					tickWidth = roundUpToNiceNumber((_max - _min) / (double) sizeRequest);
				if(tickWidth == 0.0)
					tickWidth = 1.0;
				double
					pos = roundUpToNiceNumber(_min, tickWidth);
				while(pos <= _max)
				{
					std::cout << tickWidth << ' ' << pos << '\n';
					_ticks.push_back(pos);
					pos += tickWidth;
				}
			}
		}
		
		double roundUpToNiceNumber(double number)
		{
			if(!std::isfinite(number))
				return number;
			double roundedNumber = 1.0;
			if(number <= 0.0)
			{
				if(roundedNumber == 0.0)
					return 0.0;
				else
				{
					roundedNumber = -1.0;
					number *= -1.0;
				}
			}
			while(number > 10)
			{
				number /= 10;
				roundedNumber *= 10;
			}
			while(number <= 1)
			{
				number *= 10;
				roundedNumber /= 10;
			}
			if(number <= 2) return roundedNumber * 2;
			else if(number <= 5) return roundedNumber * 5;
			else return roundedNumber * 10;
		}
		double roundUpToNiceNumber(double number, double roundUnit)
		{
			return roundUnit * ceil(number / roundUnit);
		}
		
		double _min, _max;
		std::vector<double> _ticks;
};

class TimeTickSet : public TickSet
{
	public:
		TimeTickSet(double minTime, double maxTime, unsigned sizeRequest) : _min(minTime), _max(maxTime)
		{
			set(sizeRequest);
		}
		
		virtual unsigned Size() const
		{
			return _ticks.size();
		}
		
		virtual Tick GetTick(unsigned i) const
		{
			double val = _ticks[i];
			return Tick((val - _min) / (_max - _min), Date::AipsMJDToTimeString(val));
		}
		
		virtual void Reset(unsigned sizeRequest)
		{
			_ticks.clear();
			set(sizeRequest);
		}
	private:
		void set(unsigned sizeRequest)
		{
			if(_max == _min)
				_ticks.push_back(_min);
			else
			{
				if(sizeRequest == 0)
					sizeRequest = 1;
			double tickWidth = calculateTickWidth((_max - _min) / (double) sizeRequest);
				if(tickWidth == 0.0)
					tickWidth = 1.0;
				double
					pos = roundUpToNiceNumber(_min, tickWidth);
				while(pos < _max)
				{
					std::cout << tickWidth << ' ' << pos << '\n';
					_ticks.push_back(pos);
					pos += tickWidth;
				}
			}
		}
		
		double calculateTickWidth(double lowerLimit) const
		{
			// number is in units of seconds
			
			// In days?
			if(lowerLimit >= 60.0*60.0*24.0)
			{
				double width = 60.0*60.0*24.0;
				while(width < lowerLimit)
					width *= 2.0;
				return width;
			}
			// in hours?
			else if(lowerLimit > 60.0*30.0)
			{
				if(lowerLimit <= 60.0*60.0)
					return 60.0*60.0; // hours
				else if(lowerLimit <= 60.0*60.0*2.0)
					return 60.0*60.0*2.0; // two hours
				else if(lowerLimit <= 60.0*60.0*3.0)
					return 60.0*60.0*3.0; // three hours
				else if(lowerLimit <= 60.0*60.0*4.0)
					return 60.0*60.0*4.0; // four hours
				else if(lowerLimit <= 60.0*60.0*6.0)
					return 60.0*60.0*6.0; // six hours
				else
					return 60.0*60.0*12.0; // twelve hours
			}
			// in minutes?
			else if(lowerLimit > 30.0)
			{
				if(lowerLimit <= 60.0)
					return 60.0; // in minutes
				else if(lowerLimit <= 60.0*2.0)
					return 60.0*2.0; // two minutes
				else if(lowerLimit <= 60.0*5.0)
					return 60.0*5.0; // five minutes
				else if(lowerLimit <= 60.0*10.0)
					return 60.0*10.0; // ten minutes
				else if(lowerLimit <= 60.0*15.0)
					return 60.0*15.0; // quarter hours
				else
					return 60.0*30.0; // half hours
			}
			// in seconds?
			else if(lowerLimit > 0.5)
			{
				if(lowerLimit <= 1.0)
					return 1.0; // in seconds
				else if(lowerLimit <= 2.0)
					return 2.0; // two seconds
				else if(lowerLimit <= 5.0)
					return 5.0; // five seconds
				else if(lowerLimit <= 10.0)
					return 10.0; // ten seconds
				else if(lowerLimit <= 15.0)
					return 15.0; // quarter minute
				else
					return 30.0; // half a minute
			}
			else if(lowerLimit == 0.0)
				return 0.0;
			// in 10th of seconds or lower?
			else
			{
				double factor = 1.0;
				while(lowerLimit < 0.1)
				{
					factor *= 0.1;
					lowerLimit *= 10.0;
				}
				if(lowerLimit <= 0.1)
					return 0.1 * factor;
				else if(lowerLimit <= 0.2)
					return 0.2 * factor;
				else
					return 0.5 * factor;
			}
		}
		
		double roundUpToNiceNumber(double number, double roundUnit)
		{
			return roundUnit * ceil(number / roundUnit);
		}
		
		double _min, _max;
		std::vector<double> _ticks;
};
