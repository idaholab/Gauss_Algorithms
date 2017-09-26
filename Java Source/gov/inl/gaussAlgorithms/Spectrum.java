/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: Spectrum.java
 *
 *  Description: holds a spectrum and its uncertainties
 */
package gov.inl.gaussAlgorithms;

/**
 * histogram of channel vs count (gamma-ray spectrum)
 *
 */
public class Spectrum {

	private final int                   m_firstChannel;
	private final int                   m_lastChannel;
	// expect to have same size array for count as for sigCount
	private final int[]                 m_count;
	private final double[]              m_sigCount;

	public Spectrum (int firstChannel, final int[] count) throws Exception {
		
		m_firstChannel = firstChannel;
		if (null == count) {
			m_count = new int[0];
		} else {
			m_count = count.clone();
		}
		m_lastChannel = m_firstChannel + m_count.length - 1;
		m_sigCount = constructSigCounts(count);
	}
	
	public Spectrum (int firstChannel, final int[] count,
			final double[] sigCount) throws Exception {
		
		m_firstChannel = firstChannel;
		if (null == count) {
			m_count = new int[0];
			m_sigCount = new double[0];
		} else {
			m_count = count.clone();
			m_sigCount = sigCount.clone();
		}
		m_lastChannel = m_firstChannel + m_count.length - 1;
	}

	public int getCountAt(int channel) {
		return m_count[channel - m_firstChannel];
	}

	public int[] getCounts() {
		return m_count;
	}

	public int getFirstChannel() {
		return m_firstChannel;
	}

	public int getLastChannel() {
		return m_lastChannel;
	}
	
	public int[] getRegionCounts(ChannelRange region) {
		
		// TODO added on 7/3/2014. Check it.
		if (region.getFirstChannel() > m_lastChannel) {
			return new int[0];
		}
		if (region.getLastChannel() < m_firstChannel) {
			return new int[0];
		}

		int bottom = Math.max(region.getFirstChannel(), m_firstChannel);
		int top = Math.min(region.getLastChannel(), m_lastChannel);
		top = Math.max(bottom, top);

		int regionWidth = top - bottom + 1;
		int[] regionCounts = new int[regionWidth];
		int pointer = bottom - m_firstChannel;
		
		for (int i = 0; i < regionWidth; i++, pointer++) {
			regionCounts[i] = m_count[pointer];
		}
		
		return regionCounts;
	}
	
	public double getSigCountAt(int channel) {
		return m_sigCount[channel - m_firstChannel];
	}

	public double[] getSigCounts() {
		return m_sigCount;
	}

	private static double[] constructSigCounts(final int[] counts)
			throws Exception {

		int numChannels = counts.length;
		double[] sigCount = new double[numChannels];

		for (int i = 0; i < numChannels; i++) {
			double temp = Math.max(0.0, counts[i]);
			sigCount[i] = Math.sqrt(temp);

			if (sigCount[i] <= 0.0)
				sigCount[i] = .3;
		}
		
		// do small count correction for counts <= 10 based on GW Phillips,
		// NIM 153 (1978), p. 449.

		int i;
		for (i = 0; i < 2; i++) {
			if (counts[i] <= 10) {
				double temp = (counts[i] + counts[i+1] + counts[i+2]) / 3.0;
				sigCount[i] = Math.sqrt(temp);
				
				if (sigCount[i] <= 0.0) {
					sigCount[i] = .5773503;
				}
			}
		}

		for (; i < numChannels - 2; i++) {
			if (counts[i] <= 10) {
				double temp = counts[i-2] + counts[i+2] +
	                          (2 * (counts[i-1] + counts[i+1])) +
	                          (3 * counts[i]);
				temp = Math.max(0, temp / 9.0);
				sigCount[i] = Math.sqrt(temp);

				if (sigCount[i] <= 0.0) {
					sigCount[i] = .3333333;
				}
	        }
		}

		for (; i < numChannels; i++) {
			if (counts[i] <= 10) {
				double temp = (counts[i-2] + counts[i-1] + counts[i]) / 3.0;
				sigCount[i] = Math.sqrt(temp);

				if (sigCount[i] <= 0.0) {
					sigCount[i] = .5773503;
				}
			}
		}
		
		return sigCount;
	}
}
