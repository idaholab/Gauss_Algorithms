/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: Algorithms.java
 *
 *  Description: Implements a channel range for searches or fits
 */
package gov.inl.gaussAlgorithms;

import java.util.Iterator;
import java.util.TreeSet;

/**
 * This defines a contiguous range of channels from low to high, inclusive.
 *
 */
public class ChannelRange implements Comparable<Object> {
	
	// member data

	private final int m_firstChannel;
	private final int m_lastChannel;

	// constructor
	
	public ChannelRange(int end1, int end2) {

		m_firstChannel = Math.min(end1, end2);
		m_lastChannel = Math.max(end1, end2);
	}
	
	// public methods

	public int compareTo(Object channelRangeObject) {

		int answer = 0;

		ChannelRange other = (ChannelRange) channelRangeObject;

		answer = m_firstChannel - other.getFirstChannel(); // check order
		if (answer == 0) {
			answer = m_lastChannel - other.getLastChannel();
		}

		return answer;
	}
	
	public boolean contains(double channel) {
		
		boolean answer = true;
		
		if ((channel < m_firstChannel) || (channel > m_lastChannel)) {
			answer = false;
		}
		
		return answer;
	}

	public boolean equals(Object channelRangeObject) {

		if (!channelRangeObject.getClass().equals(getClass())) {
			return false;
		}
		ChannelRange channelRange = (ChannelRange) channelRangeObject;

		boolean answer = false;

		if ((channelRange.m_firstChannel == m_firstChannel) &&
			(channelRange.m_lastChannel == m_lastChannel)) {
			answer = true;
		}

		return answer;
	}

	public TreeSet<Peak> peaksInRange(final TreeSet<Peak> peaks) {

		TreeSet<Peak> workingList = new TreeSet<Peak>();

		for (Iterator<Peak> it = peaks.iterator(); it.hasNext(); ) {
			Peak peak = it.next();
			
			if (peak.inChannelRange(this)) {
				workingList.add(peak);
			}
		}

		return workingList;
	}	

	public int getFirstChannel() {

		return m_firstChannel;
	}

	public int getLastChannel() {

		return m_lastChannel;
	}

	public int hashCode() {

		return new String(m_firstChannel + "-" + m_lastChannel).hashCode();
	}

	// is "this" channel range inside the bounds of the argument channel range
	public boolean inChannelRange(final ChannelRange range) {

		boolean inside = false;

		if (null != range) {
			if ((range.getFirstChannel() <= m_firstChannel) &&
				(range.getLastChannel() >= m_lastChannel)) {
				inside = true;
			}
		}

		return inside;
	}
	
	public String toDisplayString() {
		
		return m_firstChannel + " -> " + m_lastChannel;
	}

	public String toString() {
		return "channelRange[first = " + m_firstChannel + " last = " +
			m_lastChannel + "]";
	}
	
	public int widthChannels() {
		
		return m_lastChannel - m_firstChannel + 1;
	}
}
