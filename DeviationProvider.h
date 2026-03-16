/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DEVIATIONPROVIDER_H_
#define DEVIATIONPROVIDER_H_

#include "NonCopyable.h"
#include <cmath>
#include <functional>
#include <map>

template <typename K>
class DeviationProvider
{
	DECLARE_NON_COPYABLE(DeviationProvider)
public:
	DeviationProvider() : m_needUpdate(false), m_meanValue(0.0), m_standardDeviation(0.0) {}

	explicit DeviationProvider(std::function<double(K const&)> const& computeValueByKey)
	:	m_computeValueByKey(computeValueByKey),
		m_needUpdate(false), m_meanValue(0.0), m_standardDeviation(0.0) {}

	bool isDeviant(K const& key, double coefficient = 1.0, double threshold = 0.0, bool defaultVal = false) const;

	double getDeviationValue(K const& key) const;

	void addOrUpdate(K const& key);

	void addOrUpdate(K const& key, double value);

	void remove(K const& key);

	void clear();

	void setComputeValueByKey(std::function<double(K const&)> const& computeValueByKey);

protected:
	void update() const;

private:
	std::function<double(K const&)> m_computeValueByKey;
	std::map<K, double> m_keyValueMap;

	mutable bool m_needUpdate;
	mutable double m_meanValue;
	mutable double m_standardDeviation;
};


template <typename K>
bool
DeviationProvider<K>::isDeviant(K const& key, double coefficient, double threshold, bool defaultVal) const
{
	if (m_keyValueMap.find(key) == m_keyValueMap.end()) {
		return false;
	}
	if (m_keyValueMap.size() < 3) {
		return false;
	}

	double value = m_keyValueMap.at(key);
	if (std::isnan(value)) {
		return defaultVal;
	}

	update();
	return (std::abs(value - m_meanValue)
	        > std::max((coefficient * m_standardDeviation), (threshold / 100) * m_meanValue));
}

template <typename K>
double
DeviationProvider<K>::getDeviationValue(K const& key) const
{
	if (m_keyValueMap.find(key) == m_keyValueMap.end()) {
		return -1.0;
	}
	if (m_keyValueMap.size() < 2) {
		return 0.0;
	}

	double value = m_keyValueMap.at(key);
	if (std::isnan(value)) {
		return -1.0;
	}

	update();
	return std::abs(m_keyValueMap.at(key) - m_meanValue);
}

template <typename K>
void
DeviationProvider<K>::addOrUpdate(K const& key)
{
	m_needUpdate = true;
	m_keyValueMap[key] = m_computeValueByKey(key);
}

template <typename K>
void
DeviationProvider<K>::addOrUpdate(K const& key, double value)
{
	m_needUpdate = true;
	m_keyValueMap[key] = value;
}

template <typename K>
void
DeviationProvider<K>::remove(K const& key)
{
	m_needUpdate = true;
	typename std::map<K, double>::iterator it = m_keyValueMap.find(key);
	if (it != m_keyValueMap.end()) {
		m_keyValueMap.erase(it);
	}
}

template <typename K>
void
DeviationProvider<K>::update() const
{
	if (!m_needUpdate) {
		return;
	}
	if (m_keyValueMap.size() < 2) {
		return;
	}

	int count = 0;
	{
		double sum = 0.0;
		for (typename std::map<K, double>::const_iterator it = m_keyValueMap.begin();
		     it != m_keyValueMap.end(); ++it) {
			if (!std::isnan(it->second)) {
				sum += it->second;
				count++;
			}
		}
		m_meanValue = sum / count;
	}

	{
		double differencesSum = 0.0;
		for (typename std::map<K, double>::const_iterator it = m_keyValueMap.begin();
		     it != m_keyValueMap.end(); ++it) {
			if (!std::isnan(it->second)) {
				differencesSum += std::pow(it->second - m_meanValue, 2);
			}
		}
		m_standardDeviation = std::sqrt(differencesSum / (count - 1));
	}

	m_needUpdate = false;
}

template <typename K>
void
DeviationProvider<K>::setComputeValueByKey(std::function<double(K const&)> const& computeValueByKey)
{
	m_computeValueByKey = computeValueByKey;
}

template <typename K>
void
DeviationProvider<K>::clear()
{
	m_keyValueMap.clear();
	m_needUpdate = false;
	m_meanValue = 0.0;
	m_standardDeviation = 0.0;
}

#endif
